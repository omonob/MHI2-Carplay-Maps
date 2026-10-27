#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cpvc_authorized_runtime.h"
#define ALT_FRAME_RING_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-alt-ring.bin"
#define ALT_FRAME_FORCE_IDR_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-alt-force-idr.request"
#define CPMAP_CACHE_LOCK CPMAP_ROOT_PREFIX "/ramdisk/cpmap-video-cache.lock"
#include "altscreen_frame_protocol.h"
#include "cpmap_resources.h"
#include "cpvc_video_policy.h"
#ifndef CARPLAY_YUV_SHM_PATH
#define CARPLAY_YUV_SHM_PATH CPMAP_ROOT_PREFIX "/dev/shmem/carplay_cluster_v1_yuv"
#endif
#include "../mirror/carplay_yuv_shm.h"

#define READY_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-altstream.ready"
#define BRIDGE_READY_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-bridge.ready"
#define RING_PATH ALT_FRAME_RING_PATH
#define DISABLE_PATH CPMAP_ROOT_PREFIX "/mnt/app/carplaymaps/stock_first/DISABLED"
#define AUTO_ENABLE_PATH CPMAP_ROOT_PREFIX "/mnt/app/carplaymaps/CLUSTER_AUTO_ENABLE"
#define LOCK_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-cluster-supervisor.pid"
#define DIO_PID_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-dio-manager.pid"
#define LOG_PATH CPMAP_ROOT_PREFIX "/ramdisk/carplay-cluster-supervisor.log"
#define BRIDGE_LOG_PATH CPMAP_ROOT_PREFIX "/ramdisk/altscreen-most-bridge.log"
#define MIRROR_LOG_PATH CPMAP_ROOT_PREFIX "/ramdisk/cpvc-mirror.log"
#define BRIDGE_PATH CPMAP_ROOT_PREFIX "/mnt/app/carplaymaps/altscreen-most-bridge"
#define MIRROR_PATH CPMAP_ROOT_PREFIX "/mnt/app/carplaymaps/cpvc-mirror-canvas"
#define DMDT_PATH CPMAP_ROOT_PREFIX "/eso/bin/apps/dmdt"
#define MLB_DEBUG_PATH CPMAP_ROOT_PREFIX "/dev/mlb/debug/isoTX2"
#define DIAG_DIR CPMAP_ROOT_PREFIX "/ramdisk/carplaymaps-diagnostics"
#define DIAG_BRIDGE_PATH DIAG_DIR "/last-bridge.log"
#define DIAG_SUPERVISOR_PATH DIAG_DIR "/last-supervisor.log"

#define POLL_US 100000U
#define READY_STABLE_US 400000ULL
#define DISCONNECT_GRACE_US 500000ULL
#define RETRY_DELAY_US 2000000ULL

static volatile sig_atomic_t g_stop;
static pid_t g_bridge_pid = -1;
static int g_child_mode;

static uint64_t monotonic_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL +
           (uint64_t)ts.tv_nsec / 1000ULL;
}

static void log_event(const char *event, long value1, long value2)
{
    cpmap_log_fprintf(stderr, CPMAP_SMALL_LOG_LIMIT,
            "t_us=%llu event=%s value1=%ld value2=%ld\n",
            (unsigned long long)monotonic_us(), event, value1, value2);
    fflush(stderr);
}

static void on_signal(int signal_number)
{
    (void)signal_number;
    g_stop = 1;
}

static int path_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

static int create_marker(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}

static pid_t read_pid_file(const char *path)
{
    char text[32];
    ssize_t count;
    long value;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    count = read(fd, text, sizeof(text) - 1U);
    close(fd);
    if (count <= 0) return -1;
    text[count] = '\0';
    value = strtol(text, NULL, 10);
    return value > 1 ? (pid_t)value : -1;
}

static int acquire_lock(void)
{
    int attempt;
    for (attempt = 0; attempt < 2; ++attempt) {
        int fd = open(LOCK_PATH, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd >= 0) {
            char text[32];
            int length = snprintf(text, sizeof(text), "%ld\n", (long)getpid());
            if (length > 0) (void)write(fd, text, (size_t)length);
            close(fd);
            return 0;
        }
        if (errno == EEXIST) {
            char text[32];
            long pid_value = -1;
            ssize_t count;
            fd = open(LOCK_PATH, O_RDONLY);
            if (fd >= 0) {
                count = read(fd, text, sizeof(text) - 1U);
                close(fd);
                if (count > 0) {
                    text[count] = '\0';
                    pid_value = strtol(text, NULL, 10);
                }
            }
            if (pid_value > 1 && kill((pid_t)pid_value, 0) == 0)
                return -1;
            unlink(LOCK_PATH);
            continue;
        }
        break;
    }
    return -1;
}

static void set_runtime_environment(void)
{
    setenv("IPL_CONFIG_DIR", "/etc/eso/production", 1);
    setenv("LD_LIBRARY_PATH",
           "/mnt/app/root/lib-target:/eso/lib:/mnt/app/usr/lib:"
           "/mnt/app/armle/lib:/mnt/app/armle/lib/dll:"
           "/mnt/app/armle/usr/lib", 1);
    unsetenv("LD_PRELOAD");
}

static int run_wait(char *const argv[])
{
    pid_t child = fork();
    pid_t result;
    int status;
    if (child < 0) return -1;
    if (child == 0) {
        unsetenv("LD_PRELOAD");
        execv(argv[0], argv);
        _exit(127);
    }
    do {
        result = waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);
    if (result != child) return -1;
    if (!WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}

static int copy_diagnostic_file(const char *source, const char *target)
{
    uint8_t buffer[4096];
    int input = -1;
    int output = -1;
    int result = -1;
    input = open(source, O_RDONLY);
    if (input < 0) goto done;
    output = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output < 0) goto done;
    for (;;) {
        ssize_t count = read(input, buffer, sizeof(buffer));
        size_t offset = 0;
        if (count == 0) break;
        if (count < 0) {
            if (errno == EINTR) continue;
            goto done;
        }
        while (offset < (size_t)count) {
            ssize_t written = write(output, buffer + offset,
                                    (size_t)count - offset);
            if (written > 0) offset += (size_t)written;
            else if (written < 0 && errno == EINTR) continue;
            else goto done;
        }
    }
    (void)fsync(output);
    result = 0;
done:
    if (output >= 0) close(output);
    if (input >= 0) close(input);
    return result;
}

static void archive_diagnostics(void)
{
    int bridge_status;
    int supervisor_status;
    if (mkdir(DIAG_DIR, 0755) != 0 && errno != EEXIST) {
        log_event("diagnostics-mkdir-failed", errno, 0);
        return;
    }
    bridge_status = copy_diagnostic_file(BRIDGE_LOG_PATH, DIAG_BRIDGE_PATH);
    supervisor_status = copy_diagnostic_file(LOG_PATH, DIAG_SUPERVISOR_PATH);
    log_event("diagnostics-archived", bridge_status, supervisor_status);
}

static int request_most_flush(void)
{
    static const char command[] = "flush(0)\n";
    int fd = open(MLB_DEBUG_PATH, O_WRONLY | O_NONBLOCK);
    ssize_t written;
    int saved_errno;
    if (fd < 0) return -1;
    written = write(fd, command, sizeof(command) - 1U);
    saved_errno = errno;
    close(fd);
    if (written == (ssize_t)(sizeof(command) - 1U)) return 0;
    errno = saved_errno;
    return -1;
}

static int release_oem_most(void)
{
    char *dc[] = { (char *)DMDT_PATH, "dc", "72", NULL };
    char *sc[] = { (char *)DMDT_PATH, "sc", "4", "72", NULL };
    int dc_status = run_wait(dc);
    int sc_status = run_wait(sc);
    log_event("oem-most-release", dc_status, sc_status);
    return dc_status == 0 && sc_status == 0 ? 0 : -1;
}

static int restore_oem_most(void)
{
    char *dc[] = { (char *)DMDT_PATH, "dc", "70", "33", NULL };
    char *sc[] = { (char *)DMDT_PATH, "sc", "4", "70", NULL };
    int dc_status = run_wait(dc);
    int sc_status = run_wait(sc);
    log_event("oem-most-restore", dc_status, sc_status);
    return dc_status == 0 && sc_status == 0 ? 0 : -1;
}

static int start_bridge(int mode)
{
    pid_t child;
    unsigned int i;
    int status;
    int log_fd;
    if (g_bridge_pid > 0) return 0;
    child = fork();
    if (child < 0) return -1;
    if (child == 0) {
        log_fd = open(mode == CPVC_VIDEO_MIRROR ? MIRROR_LOG_PATH : BRIDGE_LOG_PATH,
                      O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            if (log_fd > STDERR_FILENO) close(log_fd);
        }
        unsetenv("LD_PRELOAD");
        if (mode == CPVC_VIDEO_MIRROR)
            execl(MIRROR_PATH, MIRROR_PATH, "shm", (char *)NULL);
        else
            execl(BRIDGE_PATH, BRIDGE_PATH, (char *)NULL);
        _exit(127);
    }
    g_bridge_pid = child;
    g_child_mode = mode;
    for (i = 0; i < 15U; ++i) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            g_bridge_pid = -1;
            log_event("bridge-start-failed", status, 0);
            return -1;
        }
        usleep(50000);
    }
    log_event("bridge-started", (long)child, 0);
    return 0;
}

static int stop_bridge(void)
{
    unsigned int i;
    unsigned int grace_polls = g_child_mode == CPVC_VIDEO_MIRROR ? 150U : 10U;
    int status;
    pid_t child = g_bridge_pid;
    unlink(BRIDGE_READY_PATH);
    if (child <= 0) return 0;
    log_event("video-owner-stop-request", g_child_mode, (long)child);
    kill(child, SIGTERM);
    for (i = 0; i < grace_polls; ++i) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            log_event("bridge-stopped", (long)child, status);
            g_bridge_pid = -1;
            return 0;
        }
        usleep(100000);
    }
    /* Only the raw TS bridge owns the MOST device. The mirror is a native
     * DisplayManager/EGL producer; give it time to restore 33 and release its
     * own EGL resources, and never flush somebody else's encoder device. */
    if (g_child_mode == CPVC_VIDEO_MAP) {
        status = request_most_flush();
        log_event("bridge-driver-flush", status, status == 0 ? 0 : errno);
    }
    for (i = 0; i < (g_child_mode == CPVC_VIDEO_MAP ? 40U : 0U); ++i) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            log_event("bridge-stopped-after-flush", (long)child, status);
            g_bridge_pid = -1;
            return 0;
        }
        usleep(100000);
    }
    kill(child, SIGKILL);
    for (i = 0; i < 20U; ++i) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            log_event("bridge-killed", (long)child, status);
            g_bridge_pid = -1;
            return 0;
        }
        usleep(100000);
    }
    log_event("bridge-stop-timeout", (long)child, 0);
    return -1;
}

static int takeover(int mode)
{
    if (mode == CPVC_VIDEO_MAP) {
        if (release_oem_most() != 0) return -1;
        usleep(500000);
    }
    /* The mirror owns its own EGL context, and only selects it after drawing
     * its first frame. Do not release the OEM encoder before that point. */
    if (start_bridge(mode) != 0) {
        restore_oem_most();
        return -1;
    }
    if (create_marker(BRIDGE_READY_PATH) != 0)
        log_event("bridge-ready-marker-failed", errno, 0);
    else
        log_event("bridge-ready", (long)g_bridge_pid, 0);
    return 0;
}

static int mirror_stream_ready(uint64_t now, int require_fresh)
{
    CarPlayYuvSharedHeader h;
    CarPlayYuvSharedHeader *shared;
    struct stat st;
    uint32_t sequence;
    int fd = open(CARPLAY_YUV_SHM_PATH, O_RDONLY);
    if (fd < 0) return 0;
    if (fstat(fd, &st) != 0 || st.st_size < (off_t)sizeof(h)) { close(fd); return 0; }
    /* POSIX shared-memory objects are mmap interfaces on QNX; do not assume
     * that the Linux regular-file read() fixture also works on /dev/shmem. */
    shared = (CarPlayYuvSharedHeader *)mmap(NULL, sizeof(h), PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (shared == MAP_FAILED) return 0;
    sequence = shared->sequence;
    __sync_synchronize();
    memcpy(&h, shared, sizeof(h));
    __sync_synchronize();
    if (sequence != shared->sequence) h.sequence = 0;
    munmap(shared, sizeof(h));
    return h.magic == CARPLAY_YUV_MAGIC &&
        h.version == CARPLAY_YUV_VERSION && h.stream_active && h.sequence &&
        h.source_width == CARPLAY_YUV_WIDTH && h.source_height == CARPLAY_YUV_HEIGHT &&
        (!require_fresh || (now >= h.timestamp_us && now - h.timestamp_us < 2000000ULL));
}

static int fallback(void)
{
    if (stop_bridge() != 0) {
        archive_diagnostics();
        log_event("fallback-blocked-bridge-open", (long)g_bridge_pid, 0);
        return -1;
    }
    archive_diagnostics();
    usleep(250000);
    return restore_oem_most();
}

int main(void)
{
    AltFrameRing *ring = MAP_FAILED;
    int ring_fd = -1;
    int log_fd;
    int active = 0;
    uint32_t last_generation = 0;
    uint32_t last_sequence = 0;
    uint64_t ready_since_us = 0;
    uint64_t missing_since_us = 0;
    uint64_t next_retry_us = 0;
    struct cpmap_projection_preferences map_preference = { 1, 0, 0, 0, 0, 0, 1, 0 };
    uint64_t preference_check_us = 0;
    int preference_error_logged = 0;
    int preference_valid = 0;
    int pending_mode = 0;

    log_fd = open(LOG_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd >= 0) {
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
        if (log_fd > STDERR_FILENO) close(log_fd);
    }
    if (acquire_lock() != 0) {
        log_event("already-running", 0, 0);
        return 0;
    }
    set_runtime_environment();
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGHUP, on_signal);
    unlink(BRIDGE_READY_PATH);
    log_event("supervisor-started", (long)getpid(), 0);

    while (!g_stop) {
        uint64_t now = monotonic_us();
        int marker_ready = path_exists(READY_PATH);
        int authorized = cpvc_authorized_runtime();
        int requested;
        if (now >= preference_check_us) {
            struct cpmap_projection_preferences readback;
            int result = cpmap_read_projection_preferences(CPVC_PREF_DIR, &readback);
            preference_check_us = now + 500000ULL;
            if (result >= 0) {
                if (map_preference.map != readback.map ||
                    map_preference.generation != readback.generation)
                    log_event("menu-map-preference", readback.map, (long)readback.generation);
                map_preference = readback;
                preference_valid = 1;
                preference_error_logged = 0;
            } else {
                preference_valid = 0;
                if (!preference_error_logged) {
                    log_event("menu-preference-invalid-video-off", 0, 0);
                    preference_error_logged = 1;
                }
            }
        }
        /* Reuse the existing safe fallback path; never stop dio_manager or
         * terminate this supervisor simply because the menu was switched off. */
        requested = cpvc_video_requested(&map_preference, preference_valid,
                          authorized, path_exists(AUTO_ENABLE_PATH));
        pid_t dio_pid = read_pid_file(DIO_PID_PATH);
        int dio_alive = dio_pid > 1 &&
                        (kill(dio_pid, 0) == 0 || errno == EPERM);
        int map_ready = marker_ready && dio_alive;
        int stream_ready = dio_alive && (requested == CPVC_VIDEO_MAP ?
                            map_ready : requested == CPVC_VIDEO_MIRROR ?
                            mirror_stream_ready(now, active != CPVC_VIDEO_MIRROR) : 0);

        cpmap_limit_path("/ramdisk/carplay-security-bridge.log", CPMAP_SMALL_LOG_LIMIT);
        cpmap_limit_path("/ramdisk/cpmap-device-auth.log", CPMAP_SMALL_LOG_LIMIT);
        cpmap_limit_path("/ramdisk/cpmap-disconnect.log", CPMAP_SMALL_LOG_LIMIT);
        cpmap_limit_path("/ramdisk/carplaymaps-device-bound.log", CPMAP_VIDEO_LOG_LIMIT);
        cpmap_limit_path(BRIDGE_LOG_PATH, CPMAP_BRIDGE_LOG_LIMIT);
        cpmap_limit_path(MIRROR_LOG_PATH, CPMAP_BRIDGE_LOG_LIMIT);

        /*
         * dio_manager may be terminated before the preload listener gets a
         * chance to remove its ready marker.  Never carry that marker into a
         * later CarPlay session: PID liveness is authoritative here.
         */
        if (marker_ready && !dio_alive) {
            if (unlink(READY_PATH) == 0)
                log_event("stale-ready-cleared", (long)dio_pid, 0);
            marker_ready = 0;
            map_ready = 0;
            stream_ready = 0;
        }

        if (path_exists(DISABLE_PATH)) {
            log_event("disabled", 0, 0);
            break;
        }
        if (!active && !map_ready && g_bridge_pid <= 0 && path_exists(RING_PATH)) {
            int cache_lock = cpmap_cache_cleanup_lock();
            if (cache_lock >= 0) {
                struct stat cache_stat;
                long released = stat(RING_PATH, &cache_stat) == 0 ?
                    (long)cache_stat.st_size : 0;
                if (ring != MAP_FAILED) {
                    munmap(ring, 4096U);
                    ring = MAP_FAILED;
                }
                last_generation = 0;
                last_sequence = 0;
                if (unlink(RING_PATH) == 0) {
                    unlink(ALT_FRAME_FORCE_IDR_PATH);
                    /* The lock also prevents a new producer from creating
                     * these captures during cleanup. Keep intro-once state. */
                    unlink("/ramdisk/alt-video-config.bin");
                    unlink("/ramdisk/alt-first-header.bin");
                    unlink("/ramdisk/alt-first-frame-prefix.bin");
                    unlink("/ramdisk/alt-first-frame-decrypted.bin");
                    unlink("/ramdisk/alt-first-frame-header.bin");
                    log_event("video-cache-released", released, 0);
                } else {
                    log_event("video-cache-release-failed", errno, released);
                }
                close(cache_lock);
            }
        }
        if (ring == MAP_FAILED) {
            ring_fd = open(RING_PATH, O_RDONLY);
            if (ring_fd >= 0) {
                struct stat ring_stat;
                if (fstat(ring_fd, &ring_stat) == 0 && ring_stat.st_size >= 4096)
                    ring = (AltFrameRing *)mmap(NULL, 4096U, PROT_READ,
                                               MAP_SHARED, ring_fd, 0);
                close(ring_fd);
                ring_fd = -1;
                if (ring == MAP_FAILED)
                    log_event("ring-map-failed", errno, 0);
                else
                    log_event("ring-mapped", 0, 0);
            }
        }
        if (ring != MAP_FAILED &&
            ring->magic == ALT_FRAME_RING_MAGIC &&
            ring->version == ALT_FRAME_RING_VERSION) {
            uint32_t generation = ring->generation;
            uint32_t sequence = ring->write_sequence;
            if (generation != last_generation || sequence != last_sequence) {
                last_generation = generation;
                last_sequence = sequence;
            }
        }

        if (!active) {
            if (stream_ready) {
                if (pending_mode != requested) { ready_since_us = 0; pending_mode = requested; }
                if (!ready_since_us) ready_since_us = now;
                if (now >= next_retry_us &&
                    now - ready_since_us >= READY_STABLE_US) {
                    if (takeover(requested) == 0) {
                        active = requested;
                        missing_since_us = 0;
                        log_event("state-carplay-active", active, (long)g_bridge_pid);
                    } else {
                        next_retry_us = now + RETRY_DELAY_US;
                        ready_since_us = 0;
                        log_event("takeover-failed", 0, 0);
                    }
                }
            } else {
                ready_since_us = 0;
            }
        } else {
            int child_status;
            pid_t child_result = waitpid(g_bridge_pid, &child_status, WNOHANG);
            if (child_result == g_bridge_pid) {
                log_event("bridge-exited", (long)g_bridge_pid, child_status);
                g_bridge_pid = -1;
                archive_diagnostics();
                restore_oem_most();
                active = 0;
                next_retry_us = now + RETRY_DELAY_US;
                ready_since_us = 0;
                continue;
            }
            if (requested != active) {
                log_event("menu-or-authorization-mode-change", active, requested);
                if (fallback() == 0) {
                    active = 0;
                    ready_since_us = 0;
                    next_retry_us = now + RETRY_DELAY_US;
                    log_event("state-oem-active", 0, 0);
                }
            } else if (!stream_ready) {
                if (!missing_since_us) missing_since_us = now;
                if (now - missing_since_us >= DISCONNECT_GRACE_US) {
                    log_event("stream-timeout-fallback", last_generation,
                              last_sequence);
                    if (fallback() == 0) {
                        active = 0;
                        ready_since_us = 0;
                        next_retry_us = now + RETRY_DELAY_US;
                        log_event("state-oem-active", 0, 0);
                    }
                }
            } else {
                missing_since_us = 0;
            }
        }
        usleep(POLL_US);
    }

    if (active) (void)fallback();
    unlink(BRIDGE_READY_PATH);
    if (ring != MAP_FAILED) munmap(ring, 4096U);
    unlink(LOCK_PATH);
    log_event("supervisor-stopped", 0, 0);
    return 0;
}
