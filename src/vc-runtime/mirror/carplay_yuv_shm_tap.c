#include "carplay_yuv_shm.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "../runtime/cpmap_resources.h"
#include "../runtime/cpmap_projection_preferences.h"
#include "../runtime/cpvc_authorized_runtime.h"

typedef int32_t NvSSStatus;
typedef int32_t NvMediaStatus;

typedef struct NvSSVideoStreamParams {
    uint32_t width;
    uint32_t height;
    float fps;
} NvSSVideoStreamParams;

typedef struct NvMediaVideoSurfaceMapMu0841 {
    uint32_t luma_width;
    uint32_t luma_height;
    uint8_t *y;
    uint32_t pitch_y;
    uint8_t *u;
    uint32_t pitch_u;
    uint8_t *v;
    uint32_t pitch_v;
} NvMediaVideoSurfaceMapMu0841;

typedef struct NvMediaVideoDescMu0841 {
    uint32_t picture_structure;
    void *next;
    void *current;
    void *previous;
    void *previous2;
    void *src_rect;
    void *dst_rect;
} NvMediaVideoDescMu0841;

typedef NvSSStatus (*ConfigureFn)(void *, const NvSSVideoStreamParams *);
typedef NvSSStatus (*CloseFn)(void *);
typedef NvMediaStatus (*SurfaceLockFn)(void *, void *);
typedef void (*SurfaceUnlockFn)(void *);
typedef NvMediaStatus (*MixerRenderFn)(void *, uint32_t, void *, void *,
                                       void *, void *, void *, void *, void *);

static ConfigureFn g_real_configure;
static CloseFn g_real_close;
static SurfaceLockFn g_lock;
static SurfaceUnlockFn g_unlock;
static MixerRenderFn g_render;
static void *g_nvss;
static void *g_nvmedia;
static void *g_target_handle;
static CarPlayYuvSharedHeader *g_shared;
static pthread_mutex_t g_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_worker_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_worker_cond = PTHREAD_COND_INITIALIZER;
#define CARPLAY_YUV_MAX_WORKERS 2
static pthread_t g_workers[CARPLAY_YUV_MAX_WORKERS];
static uint32_t g_worker_count = 1;
static void *g_pending_surface;
static uint64_t g_pending_timestamp_us;
static uint64_t g_pending_generation;
static uint64_t g_consumed_generation;
static uint64_t g_enqueued;
static uint64_t g_dropped;
static uint32_t g_workers_started;
static int g_worker_stop;
static uint32_t g_workers_busy;
static int g_capture_active;
static pthread_mutex_t g_publish_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_last_published_generation;
static uint64_t g_stale;
static uint64_t g_lock_total_us;
static uint64_t g_lock_max_us;
static uint64_t g_scale_total_us;
static uint64_t g_scale_max_us;
static uint16_t g_x_luma[CARPLAY_YUV_WIDTH];
static uint16_t g_x_chroma[CARPLAY_YUV_WIDTH / 2u];
static uint32_t g_map_width;
static uint64_t g_frames;
static pthread_mutex_t g_policy_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_policy_check_us;
static int g_mirror_requested;

static int mirror_requested(void)
{
    uint64_t now;
    struct timespec ts;
    int allowed;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
    pthread_mutex_lock(&g_policy_mutex);
    if (now >= g_policy_check_us) {
        struct cpmap_projection_preferences p;
        g_policy_check_us = now + 250000ULL;
        g_mirror_requested = cpvc_authorized_runtime() &&
            cpmap_read_projection_preferences(CPVC_PREF_DIR, &p) >= 0 &&
            !(p.map_mask & 1) && (p.mirror_mask & 1);
    }
    allowed = g_mirror_requested;
    pthread_mutex_unlock(&g_policy_mutex);
    return allowed;
}

static const char *log_path(void)
{
    const char *path = getenv("CARPLAY_YUV_LOG");
    return path && *path ? path :
        "/ramdisk/cpvc-mirror-tap.log";
}

static uint64_t monotonic_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL +
           (uint64_t)ts.tv_nsec / 1000ULL;
}

static void append_log(const char *event, uint32_t width, uint32_t height,
                       int status)
{
    FILE *file = cpmap_log_open(log_path(), CPMAP_SMALL_LOG_LIMIT);
    if (!file) return;
    fprintf(file,
            "t_us=%llu pid=%ld event=%s source=%ux%u frames=%llu "
            "active=%u status=0x%08x workers=%u enqueued=%llu dropped=%llu "
            "stale=%llu lock_avg_us=%llu lock_max_us=%llu "
            "scale_avg_us=%llu scale_max_us=%llu\n",
            (unsigned long long)monotonic_us(), (long)getpid(),
            event ? event : "?", width, height,
            (unsigned long long)g_frames,
            g_shared ? g_shared->stream_active : 0u,
            (unsigned int)status, g_worker_count,
            (unsigned long long)g_enqueued,
            (unsigned long long)g_dropped,
            (unsigned long long)g_stale,
            (unsigned long long)(g_frames ? g_lock_total_us / g_frames : 0),
            (unsigned long long)g_lock_max_us,
            (unsigned long long)(g_frames ? g_scale_total_us / g_frames : 0),
            (unsigned long long)g_scale_max_us);
    cpmap_log_close(file, CPMAP_SMALL_LOG_LIMIT);
}

static int ensure_shared(void)
{
    int fd;
    void *mapping;
    if (g_shared) return 0;
    pthread_mutex_lock(&g_init_mutex);
    if (!g_shared) {
        fd = open(CARPLAY_YUV_SHM_PATH, O_RDWR | O_CREAT, 0666);
        if (fd >= 0 && ftruncate(fd, CARPLAY_YUV_SHM_SIZE) == 0) {
            mapping = mmap(NULL, CARPLAY_YUV_SHM_SIZE,
                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (mapping != MAP_FAILED) {
                g_shared = (CarPlayYuvSharedHeader *)mapping;
                memset(g_shared, 0, CARPLAY_YUV_SHM_SIZE);
                g_shared->magic = CARPLAY_YUV_MAGIC;
                g_shared->version = CARPLAY_YUV_VERSION;
                g_shared->output_width = CARPLAY_YUV_WIDTH;
                g_shared->output_height = CARPLAY_YUV_HEIGHT;
            }
        }
        if (fd >= 0) close(fd);
    }
    pthread_mutex_unlock(&g_init_mutex);
    return g_shared ? 0 : -1;
}

static int resolve_nvss(void)
{
    if (!g_real_configure)
        g_real_configure = (ConfigureFn)dlsym(
            RTLD_NEXT, "NvSSVideoStreamConfigure");
    if (!g_real_close)
        g_real_close = (CloseFn)dlsym(RTLD_NEXT, "NvSSVideoClose");
    if (!g_real_configure || !g_real_close) {
        if (!g_nvss) g_nvss = dlopen("libnvss_video.so", RTLD_NOW | RTLD_LOCAL);
        if (g_nvss) {
            if (!g_real_configure)
                g_real_configure = (ConfigureFn)dlsym(
                    g_nvss, "NvSSVideoStreamConfigure");
            if (!g_real_close)
                g_real_close = (CloseFn)dlsym(g_nvss, "NvSSVideoClose");
        }
    }
    return g_real_configure && g_real_close ? 0 : -1;
}

static int resolve_nvmedia(void)
{
    if (!g_render)
        g_render = (MixerRenderFn)dlsym(RTLD_NEXT, "NvMediaVideoMixerRender");
    if (!g_lock)
        g_lock = (SurfaceLockFn)dlsym(RTLD_NEXT, "NvMediaVideoSurfaceLock");
    if (!g_unlock)
        g_unlock = (SurfaceUnlockFn)dlsym(
            RTLD_NEXT, "NvMediaVideoSurfaceUnlock");
    if (!g_render || !g_lock || !g_unlock) {
        if (!g_nvmedia) g_nvmedia = dlopen("libnvmedia.so", RTLD_NOW | RTLD_LOCAL);
        if (g_nvmedia) {
            if (!g_render)
                g_render = (MixerRenderFn)dlsym(
                    g_nvmedia, "NvMediaVideoMixerRender");
            if (!g_lock)
                g_lock = (SurfaceLockFn)dlsym(
                    g_nvmedia, "NvMediaVideoSurfaceLock");
            if (!g_unlock)
                g_unlock = (SurfaceUnlockFn)dlsym(
                    g_nvmedia, "NvMediaVideoSurfaceUnlock");
        }
    }
    return g_render && g_lock && g_unlock ? 0 : -1;
}

static int sane_map(const NvMediaVideoSurfaceMapMu0841 *map)
{
    uint32_t chroma_width;
    if (!map || map->luma_width < CARPLAY_YUV_WIDTH ||
        map->luma_width > 1920 || map->luma_height < CARPLAY_YUV_HEIGHT ||
        map->luma_height > 1080 || !map->y || !map->u || !map->v)
        return 0;
    chroma_width = (map->luma_width + 1u) / 2u;
    return map->pitch_y >= map->luma_width &&
           map->pitch_u >= chroma_width && map->pitch_v >= chroma_width;
}

static void prepare_x_maps(uint32_t source_width)
{
    uint32_t x;
    if (g_map_width == source_width) return;
    for (x = 0; x < CARPLAY_YUV_WIDTH; ++x)
        g_x_luma[x] = (uint16_t)(((uint64_t)x * source_width) /
                                 CARPLAY_YUV_WIDTH);
    for (x = 0; x < CARPLAY_YUV_WIDTH / 2u; ++x)
        g_x_chroma[x] = (uint16_t)(((uint64_t)x *
                                    ((source_width + 1u) / 2u)) /
                                   (CARPLAY_YUV_WIDTH / 2u));
    g_map_width = source_width;
}

static void scale_plane(uint8_t *destination, uint32_t output_width,
                        uint32_t output_height, const uint8_t *source,
                        uint32_t source_width, uint32_t source_height,
                        uint32_t source_pitch, const uint16_t *x_map)
{
    uint32_t y, x;
    if (output_width == source_width && output_height == source_height) {
        for (y = 0; y < output_height; ++y)
            memcpy(destination + (size_t)y * output_width,
                   source + (size_t)y * source_pitch, output_width);
        return;
    }
    for (y = 0; y < output_height; ++y) {
        uint32_t source_y = (uint32_t)(((uint64_t)y * source_height) /
                                      output_height);
        const uint8_t *row = source + (size_t)source_y * source_pitch;
        uint8_t *out = destination + (size_t)y * output_width;
        (void)source_width;
        for (x = 0; x < output_width; ++x) out[x] = row[x_map[x]];
    }
}

static int capture_is_active(void)
{
    int active;
    pthread_mutex_lock(&g_worker_mutex);
    active = g_capture_active && !g_worker_stop;
    pthread_mutex_unlock(&g_worker_mutex);
    return active;
}

static void *capture_worker(void *opaque)
{
    uint32_t worker_index = (uint32_t)(uintptr_t)opaque;
    while (1) {
        void *surface;
        uint64_t generation;
        uint64_t queued_timestamp_us;
        NvMediaVideoSurfaceMapMu0841 map;
        NvMediaStatus lock_status;
        uint64_t lock_started_us;
        uint64_t lock_elapsed_us = 0;
        uint64_t scale_started_us;
        uint64_t scale_elapsed_us = 0;

        pthread_mutex_lock(&g_worker_mutex);
        while (!g_worker_stop &&
               (!g_capture_active || !g_pending_surface ||
                g_pending_generation == g_consumed_generation))
            pthread_cond_wait(&g_worker_cond, &g_worker_mutex);
        if (g_worker_stop) {
            pthread_mutex_unlock(&g_worker_mutex);
            break;
        }
        surface = g_pending_surface;
        generation = g_pending_generation;
        queued_timestamp_us = g_pending_timestamp_us;
        g_consumed_generation = generation;
        ++g_workers_busy;
        pthread_mutex_unlock(&g_worker_mutex);

        if (mirror_requested() && resolve_nvmedia() == 0) {
            memset(&map, 0, sizeof(map));
            lock_started_us = monotonic_us();
            lock_status = g_lock(surface, &map);
            lock_elapsed_us = monotonic_us() - lock_started_us;
            if (lock_status == 0) {
                if (capture_is_active() && sane_map(&map) && g_shared &&
                    map.luma_width == g_shared->source_width &&
                    map.luma_height == g_shared->source_height) {
                    uint32_t slot = worker_index % CARPLAY_YUV_SLOT_COUNT;
                    uint8_t *frame = CarPlayYuvSlot(g_shared, slot);
                    uint32_t source_chroma_width =
                        (map.luma_width + 1u) / 2u;
                    uint32_t source_chroma_height =
                        (map.luma_height + 1u) / 2u;
                    prepare_x_maps(map.luma_width);
                    scale_started_us = monotonic_us();
                    scale_plane(frame, CARPLAY_YUV_WIDTH,
                                CARPLAY_YUV_HEIGHT, map.y,
                                map.luma_width, map.luma_height,
                                map.pitch_y, g_x_luma);
                    scale_plane(frame + CARPLAY_YUV_Y_SIZE,
                                CARPLAY_YUV_WIDTH / 2u,
                                CARPLAY_YUV_HEIGHT / 2u, map.u,
                                source_chroma_width, source_chroma_height,
                                map.pitch_u, g_x_chroma);
                    scale_plane(frame + CARPLAY_YUV_Y_SIZE +
                                      CARPLAY_YUV_UV_SIZE,
                                CARPLAY_YUV_WIDTH / 2u,
                                CARPLAY_YUV_HEIGHT / 2u, map.v,
                                source_chroma_width, source_chroma_height,
                                map.pitch_v, g_x_chroma);
                    scale_elapsed_us = monotonic_us() - scale_started_us;

                    /* Two workers write different slots. Publish only if this
                     * job is newer than the last completed job, so a slow old
                     * surface can never move the display backwards. */
                    pthread_mutex_lock(&g_publish_mutex);
                    if (generation > g_last_published_generation &&
                        capture_is_active()) {
                        g_lock_total_us += lock_elapsed_us;
                        if (lock_elapsed_us > g_lock_max_us)
                            g_lock_max_us = lock_elapsed_us;
                        g_scale_total_us += scale_elapsed_us;
                        if (scale_elapsed_us > g_scale_max_us)
                            g_scale_max_us = scale_elapsed_us;
                        __sync_synchronize();
                        g_shared->active_slot = slot;
                        /* CLOCK_MONOTONIC is system-wide on QNX. Preserve the
                         * mixer enqueue time so the renderer can measure the
                         * complete software-path latency. */
                        g_shared->timestamp_us = queued_timestamp_us;
                        ++g_shared->published;
                        ++g_shared->sequence;
                        g_last_published_generation = generation;
                        ++g_frames;
                        if (g_frames == 1 || g_frames % 300 == 0)
                            append_log(g_frames == 1 ? "first-frame" :
                                       "progress", map.luma_width,
                                       map.luma_height, 0);
                    } else {
                        ++g_stale;
                    }
                    pthread_mutex_unlock(&g_publish_mutex);
                }
                g_unlock(surface);
            }
        }

        pthread_mutex_lock(&g_worker_mutex);
        if (g_workers_busy) --g_workers_busy;
        pthread_cond_broadcast(&g_worker_cond);
        pthread_mutex_unlock(&g_worker_mutex);
    }
    return NULL;
}

static void queue_surface(void *surface)
{
    pthread_mutex_lock(&g_worker_mutex);
    if (g_capture_active && !g_worker_stop) {
        if (g_pending_surface &&
            g_pending_generation != g_consumed_generation)
            ++g_dropped;
        g_pending_surface = surface;
        g_pending_timestamp_us = monotonic_us();
        ++g_pending_generation;
        ++g_enqueued;
        pthread_cond_signal(&g_worker_cond);
    }
    pthread_mutex_unlock(&g_worker_mutex);
}

static void stop_capture_and_drain(void)
{
    pthread_mutex_lock(&g_worker_mutex);
    g_capture_active = 0;
    g_pending_surface = NULL;
    g_consumed_generation = g_pending_generation;
    pthread_cond_broadcast(&g_worker_cond);
    while (g_workers_busy)
        pthread_cond_wait(&g_worker_cond, &g_worker_mutex);
    pthread_mutex_unlock(&g_worker_mutex);
}

NvSSStatus NvSSVideoStreamConfigure(
    void *handle, const NvSSVideoStreamParams *params)
{
    NvSSStatus status;
    if (resolve_nvss() != 0) return -1;
    status = g_real_configure(handle, params);
    if (status == 0 && params && params->width == 1264 &&
        params->height == 640 && ensure_shared() == 0) {
        g_target_handle = handle;
        g_shared->source_width = params->width;
        g_shared->source_height = params->height;
        g_shared->sequence = 0;
        g_shared->timestamp_us = 0;
        g_shared->published = 0;
        /* Build the immutable horizontal maps before waking multiple capture
         * workers; both workers then only read them. */
        prepare_x_maps(params->width);
        g_shared->stream_active = 0; /* Publish only on a requested live frame. */
        __sync_synchronize();
        pthread_mutex_lock(&g_worker_mutex);
        g_capture_active = 1;
        pthread_cond_signal(&g_worker_cond);
        pthread_mutex_unlock(&g_worker_mutex);
        append_log("configure", params->width, params->height, status);
    }
    return status;
}

NvSSStatus NvSSVideoClose(void *handle)
{
    NvSSStatus status;
    if (resolve_nvss() != 0) return -1;
    if (handle == g_target_handle && g_shared) {
        g_shared->stream_active = 0;
        __sync_synchronize();
        stop_capture_and_drain();
        append_log("close", g_shared->source_width,
                   g_shared->source_height, 0);
    }
    status = g_real_close(handle);
    if (handle == g_target_handle) g_target_handle = NULL;
    return status;
}

NvMediaStatus NvMediaVideoMixerRender(void *mixer, uint32_t output,
                                      void *background, void *video_desc,
                                      void *arg4, void *arg5, void *arg6,
                                      void *arg7, void *arg8)
{
    NvMediaStatus status;
    NvMediaVideoDescMu0841 *video =
        (NvMediaVideoDescMu0841 *)video_desc;
    if (resolve_nvmedia() != 0) return -1;
    status = g_render(mixer, output, background, video_desc,
                      arg4, arg5, arg6, arg7, arg8);
    if (status == 0 && video && video->current && g_shared && g_target_handle) {
        int enabled = mirror_requested();
        g_shared->stream_active = enabled ? 1 : 0;
        __sync_synchronize();
        if (enabled) queue_surface(video->current);
    }
    return status;
}

__attribute__((constructor))
static void carplay_yuv_tap_loaded(void)
{
    uint32_t i;
    const char *worker_text = getenv("CARPLAY_YUV_WORKERS");
    if (worker_text && worker_text[0] == '2' && worker_text[1] == '\0')
        g_worker_count = 2;
    (void)ensure_shared();
    for (i = 0; i < g_worker_count; ++i) {
        if (pthread_create(&g_workers[i], NULL, capture_worker,
                           (void *)(uintptr_t)i) != 0)
            break;
        ++g_workers_started;
    }
    if (g_workers_started != g_worker_count)
        g_worker_count = g_workers_started;
    append_log(g_workers_started ? "loaded-async" : "worker-start-failed",
               0, 0, g_workers_started ? 0 : -1);
}

__attribute__((destructor))
static void carplay_yuv_tap_unloaded(void)
{
    uint32_t i;
    stop_capture_and_drain();
    pthread_mutex_lock(&g_worker_mutex);
    g_worker_stop = 1;
    pthread_cond_broadcast(&g_worker_cond);
    pthread_mutex_unlock(&g_worker_mutex);
    for (i = 0; i < g_workers_started; ++i)
        pthread_join(g_workers[i], NULL);
    append_log("unloaded-async", (uint32_t)g_enqueued,
               (uint32_t)g_dropped, 0);
}
