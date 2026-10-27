#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef ALTSCREEN_BRIDGE_FILE_OUTPUT
#include <devctl.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "altscreen_frame_protocol.h"
#include "cpmap_resources.h"
#include "cpmap_intro_policy.h"

#define MLB_DEVCTL_QUEUE_FLUSH ((int)0x40040506u)
#define MLB_DEVCTL_START ((int)0x80040509u)
#define MLB_DEVCTL_GET_PACKET_SIZE ((int)0x4004050cu)
#define MLB_DEVCTL_GET_BLOCK_COUNT ((int)0x4004050du)
#define MLB_DEVCTL_GET_QUEUE_SIZE ((int)0x40040510u)

#define TS_SIZE 188U
#define TS_BLOCK_PACKETS 64U
#define TS_BLOCK_SIZE (TS_SIZE * TS_BLOCK_PACKETS)
#ifndef TS_QUEUE_PACKETS
#define TS_QUEUE_PACKETS 65536U
#endif
/* 1024 packets are about 125 ms at the measured 32-byte/48-kHz isoTX2
 * allocation (12.288 Mbit/s). Never preserve a larger real-time backlog:
 * stale predictive video is worse than briefly waiting for a fresh IDR. */
#ifndef TS_LOW_LATENCY_LIMIT_PACKETS
#define TS_LOW_LATENCY_LIMIT_PACKETS 1024U
#endif
#define RECOVERY_FAST_WRITE_LIMIT_US 30000ULL
#define RECOVERY_FAST_WRITES_REQUIRED 8U
#define VIDEO_PID 0x0011U
#define PMT_PID 0x0010U
#define PCR_PID 0x1000U
/* 12.288 Mbit/s / (188 * 8) = 8170.212765... TS packets/s.
 * 327 packets are 40.02 ms.  The working 3GD920790 vectors repeat PAT, PMT
 * and a dedicated PCR packet at approximately this cadence. */
#define TRANSPORT_INTERVAL_PACKETS 327ULL
/* At 12.288 Mbit/s one 188-byte packet advances the 90-kHz clock by
 * exactly 705/64 ticks. */
#define TRANSPORT_TICKS_NUMERATOR 705ULL
#define TRANSPORT_TICKS_DENOMINATOR 64ULL
#define TRANSPORT_PCR_ORIGIN_90K 45000ULL
#define PTS_TARGET_LEAD_90K 9000ULL
#define PTS_MIN_LEAD_90K 4500ULL
#define MAX_MESSAGE (8U * 1024U * 1024U)
#define INTRO_MAX_SIZE (16U * 1024U * 1024U)
#define INTRO_DEFAULT_FPS 30U
#ifndef INTRO_ONCE_PATH
#define INTRO_ONCE_PATH "/ramdisk/carplay-v725-intro-played"
#endif

typedef struct PacketQueue {
    uint8_t *data;
    size_t read_index;
    size_t write_index;
    size_t count;
    size_t high_water_total;
    size_t high_water_window;
    int stopping;
    pthread_mutex_t mutex;
} PacketQueue;

typedef struct AvcConfig {
    uint8_t *sps;
    size_t sps_size;
    uint8_t *pps;
    size_t pps_size;
    unsigned int length_size;
    int valid;
} AvcConfig;

typedef struct Reassembly {
    uint8_t *data;
    size_t capacity;
    uint32_t sequence;
    uint32_t total_size;
    uint32_t received;
    uint32_t flags;
    uint64_t media_timestamp_ns;
    uint16_t type;
    int active;
    uint32_t last_completed_sequence;
    uint16_t last_completed_type;
    int have_last_completed;
} Reassembly;

static volatile sig_atomic_t g_stop;
/* Closing the MOST descriptor from the signal handler interrupts a QNX
 * MsgSend/REPLY-blocked write. Without this, SIGTERM can leave the writer
 * permanently holding /dev/mlb/isoTX2 and prevent a clean OEM-map fallback. */
static volatile sig_atomic_t g_most_fd = -1;
static PacketQueue g_queue;
static unsigned int g_cc_pat;
static unsigned int g_cc_pmt;
static unsigned int g_cc_video;
static unsigned int g_cc_null;
/* Updated once per 64-packet MediaLB block by the single writer thread.
 * A 32-bit value is atomic on the target ARM platform and spans more than
 * thirteen hours at 90 kHz, longer than one bridge process session. */
static volatile uint32_t g_transport_clock_90k =
    (uint32_t)TRANSPORT_PCR_ORIGIN_90K;
static unsigned long g_pts_clock_rebases;
static unsigned long g_frames;
static unsigned long g_keyframes;
static unsigned long g_dropped_messages;
static uint64_t g_media_origin_ntp;
static uint64_t g_local_origin_us;
static uint64_t g_pts_origin_value = 90000ULL;
static uint64_t g_last_pts;
static uint32_t g_last_sequence;
static unsigned long long g_input_bytes_total;
static unsigned long long g_ts_packets_total;
static size_t g_max_frame_total;
static size_t g_max_keyframe_total;
static uint64_t g_diag_window_start_us;
static unsigned long long g_diag_window_bytes;
static unsigned long g_diag_window_frames;
static unsigned long long g_diag_ts_packets_origin;
static size_t g_diag_max_frame;
static size_t g_diag_max_keyframe;
static uint64_t g_diag_max_pts_step;
static uint64_t g_max_write_us;
static unsigned long g_slow_writes;
static int g_waiting_for_keyframe;
static int g_discontinuity_pending;
static unsigned long g_latency_resets;
static unsigned long long g_latency_packets_discarded;
static unsigned long g_latency_frames_discarded;
static uint64_t g_latency_wait_start_us;
static volatile unsigned long g_writer_blocks_completed;
static volatile unsigned long g_writer_fast_streak;
static unsigned long g_recovery_writer_generation;
static int g_recovery_idr_requested;
/* Diagnostic histograms: write latency per 12032-byte block, microseconds. */
#define WRITE_HIST_BUCKETS 6U
static unsigned long g_write_hist[WRITE_HIST_BUCKETS];
static const uint64_t g_write_hist_limits[WRITE_HIST_BUCKETS - 1U] = {
    5000ULL, 10000ULL, 30000ULL, 50000ULL, 100000ULL
};
/* Input burst tracking: rolling 100 ms window. */
static uint64_t g_burst_window_start_us;
static unsigned long long g_burst_window_bytes;
static unsigned long g_burst_window_frames;
static unsigned long long g_burst_max_bytes;
static unsigned long g_burst_max_frames;
static size_t g_slow_write_queue_depth;
static unsigned long g_intro_frames;
static int g_intro_waiting_live_idr;

static uint64_t next_transport_pts_origin(void);

static uint64_t monotonic_us(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
    return (uint64_t)value.tv_sec * 1000000ULL +
           (uint64_t)value.tv_nsec / 1000ULL;
}

static void record_write_latency(uint64_t write_us)
{
    unsigned int i;
    for (i = 0; i < WRITE_HIST_BUCKETS - 1U; ++i) {
        if (write_us < g_write_hist_limits[i]) {
            ++g_write_hist[i];
            return;
        }
    }
    ++g_write_hist[WRITE_HIST_BUCKETS - 1U];
}

static void reset_write_histogram(void)
{
    unsigned int i;
    for (i = 0; i < WRITE_HIST_BUCKETS; ++i) g_write_hist[i] = 0;
}

static uint64_t bucket_limit_for_display(unsigned int bucket)
{
    if (bucket < WRITE_HIST_BUCKETS - 1U) return g_write_hist_limits[bucket];
    return 0;
}

/* AirPlayScreenHeader.params[0] is a synchronized NTP timestamp in unsigned
 * 32.32 fixed-point seconds, not nanoseconds. Convert only a session-local
 * delta so the result stays well inside 64 bits. */
static uint64_t ntp_delta_to_90khz(uint64_t delta)
{
    uint64_t whole_seconds = delta >> 32;
    uint64_t fraction = delta & 0xffffffffULL;
    return whole_seconds * 90000ULL +
           ((fraction * 90000ULL + 0x80000000ULL) >> 32);
}

static void on_signal(int signal_number)
{
    int fd;
    (void)signal_number;
    g_stop = 1;
    g_queue.stopping = 1;
    fd = (int)g_most_fd;
    g_most_fd = -1;
    if (fd >= 0) close(fd);
}

static uint32_t mpeg_crc32(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xffffffffU;
    size_t i;
    for (i = 0; i < size; ++i) {
        unsigned int bit;
        crc ^= (uint32_t)data[i] << 24;
        for (bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80000000U) ?
                (crc << 1) ^ 0x04c11db7U : crc << 1;
    }
    return crc;
}

static int queue_init(PacketQueue *queue)
{
    memset(queue, 0, sizeof(*queue));
    queue->data = (uint8_t *)malloc(TS_QUEUE_PACKETS * TS_SIZE);
    if (!queue->data) return -1;
    return pthread_mutex_init(&queue->mutex, NULL);
}

static void queue_destroy(PacketQueue *queue)
{
    pthread_mutex_destroy(&queue->mutex);
    free(queue->data);
    memset(queue, 0, sizeof(*queue));
}

static int queue_push(PacketQueue *queue, const uint8_t packet[TS_SIZE])
{
    for (;;) {
        pthread_mutex_lock(&queue->mutex);
        if (queue->stopping || g_stop) {
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
        if (queue->count < TS_QUEUE_PACKETS) {
            memcpy(queue->data + queue->write_index * TS_SIZE,
                   packet, TS_SIZE);
            queue->write_index =
                (queue->write_index + 1U) % TS_QUEUE_PACKETS;
            ++queue->count;
            if (queue->count > queue->high_water_total)
                queue->high_water_total = queue->count;
            if (queue->count > queue->high_water_window)
                queue->high_water_window = queue->count;
            pthread_mutex_unlock(&queue->mutex);
            return 0;
        }
        pthread_mutex_unlock(&queue->mutex);
        usleep(1000);
    }
}

static int queue_push_batch(PacketQueue *queue, const uint8_t *packets,
                            size_t packet_count)
{
    size_t i;
    if (!packets || packet_count == 0 || packet_count > TS_QUEUE_PACKETS)
        return -1;
    for (;;) {
        pthread_mutex_lock(&queue->mutex);
        if (queue->stopping || g_stop) {
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
        if (packet_count <= TS_QUEUE_PACKETS - queue->count) {
            for (i = 0; i < packet_count; ++i) {
                memcpy(queue->data + queue->write_index * TS_SIZE,
                       packets + i * TS_SIZE, TS_SIZE);
                queue->write_index =
                    (queue->write_index + 1U) % TS_QUEUE_PACKETS;
            }
            queue->count += packet_count;
            if (queue->count > queue->high_water_total)
                queue->high_water_total = queue->count;
            if (queue->count > queue->high_water_window)
                queue->high_water_window = queue->count;
            pthread_mutex_unlock(&queue->mutex);
            return 0;
        }
        pthread_mutex_unlock(&queue->mutex);
        usleep(1000);
    }
}

static int queue_pop(PacketQueue *queue, uint8_t packet[TS_SIZE])
{
    int result = 0;
    pthread_mutex_lock(&queue->mutex);
    if (queue->count > 0) {
        memcpy(packet, queue->data + queue->read_index * TS_SIZE, TS_SIZE);
        queue->read_index = (queue->read_index + 1U) % TS_QUEUE_PACKETS;
        --queue->count;
        result = 1;
    }
    pthread_mutex_unlock(&queue->mutex);
    return result;
}

static size_t queue_count(PacketQueue *queue)
{
    size_t count;
    pthread_mutex_lock(&queue->mutex);
    count = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return count;
}

static size_t queue_discard_all(PacketQueue *queue)
{
    size_t discarded;
    pthread_mutex_lock(&queue->mutex);
    discarded = queue->count;
    queue->read_index = queue->write_index;
    queue->count = 0;
    queue->high_water_window = 0;
    pthread_mutex_unlock(&queue->mutex);
    return discarded;
}

static int request_fresh_idr(void)
{
    int fd = open(ALT_FRAME_FORCE_IDR_PATH,
                  O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}

static void enter_low_latency_recovery(size_t queued_packets,
                                       uint32_t sequence,
                                       uint64_t now_us)
{
    size_t discarded = queue_discard_all(&g_queue);
    g_waiting_for_keyframe = 1;
    g_discontinuity_pending = 1;
    g_latency_wait_start_us = now_us;
    g_recovery_writer_generation = g_writer_blocks_completed;
    g_recovery_idr_requested = 0;
    ++g_latency_resets;
    g_latency_packets_discarded += discarded;
    /* An IDR sent while the MediaLB writer is still blocked is immediately
     * buried behind another backlog. Defer the request until the writer has
     * completed a short run of healthy blocks after this reset. */
    unlink(ALT_FRAME_FORCE_IDR_PATH);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
            "bridge: latency-reset sequence=%u queued=%lu discarded=%lu "
            "limit=%u idr_request_deferred=1 resets=%lu\n",
            sequence, (unsigned long)queued_packets,
            (unsigned long)discarded,
            (unsigned)TS_LOW_LATENCY_LIMIT_PACKETS,
            g_latency_resets);
}

static int writer_recovered_after_reset(void)
{
    unsigned long blocks = g_writer_blocks_completed;
    unsigned long streak = g_writer_fast_streak;
    return (unsigned long)(blocks - g_recovery_writer_generation) >=
               RECOVERY_FAST_WRITES_REQUIRED &&
           streak >= RECOVERY_FAST_WRITES_REQUIRED;
}

static void queue_metric_snapshot(PacketQueue *queue, size_t *current,
                                  size_t *window_high, size_t *total_high)
{
    pthread_mutex_lock(&queue->mutex);
    *current = queue->count;
    *window_high = queue->high_water_window;
    *total_high = queue->high_water_total;
    queue->high_water_window = queue->count;
    pthread_mutex_unlock(&queue->mutex);
}

static void log_input_metrics(uint64_t now_us, size_t frame_size,
                              int keyframe, uint64_t pts_step)
{
    uint64_t elapsed;
    unsigned long long input_bps;
    unsigned long long ts_bps;
    unsigned long long fps_milli;
    unsigned long long ts_packets;
    size_t queue_current;
    size_t queue_high_window;
    size_t queue_high_total;

    g_input_bytes_total += (unsigned long long)frame_size;
    g_diag_window_bytes += (unsigned long long)frame_size;
    ++g_diag_window_frames;
    if (frame_size > g_max_frame_total) g_max_frame_total = frame_size;
    if (frame_size > g_diag_max_frame) g_diag_max_frame = frame_size;
    if (keyframe && frame_size > g_max_keyframe_total)
        g_max_keyframe_total = frame_size;
    if (keyframe && frame_size > g_diag_max_keyframe)
        g_diag_max_keyframe = frame_size;
    if (pts_step > g_diag_max_pts_step) g_diag_max_pts_step = pts_step;
    /* Rolling 100 ms input-burst window. */
    if (g_burst_window_start_us == 0 ||
        now_us - g_burst_window_start_us >= 100000ULL) {
        if (g_burst_window_bytes > g_burst_max_bytes)
            g_burst_max_bytes = g_burst_window_bytes;
        if (g_burst_window_frames > g_burst_max_frames)
            g_burst_max_frames = g_burst_window_frames;
        g_burst_window_start_us = now_us;
        g_burst_window_bytes = 0;
        g_burst_window_frames = 0;
    }
    g_burst_window_bytes += (unsigned long long)frame_size;
    ++g_burst_window_frames;
    if (g_diag_window_start_us == 0) {
        g_diag_window_start_us = now_us;
        g_diag_ts_packets_origin = 0;
        return;
    }
    elapsed = now_us - g_diag_window_start_us;
    if (elapsed < 1000000ULL) return;
    ts_packets = g_ts_packets_total - g_diag_ts_packets_origin;
    input_bps = (g_diag_window_bytes * 8ULL * 1000000ULL) / elapsed;
    ts_bps = (ts_packets * TS_SIZE * 8ULL * 1000000ULL) / elapsed;
    fps_milli = ((unsigned long long)g_diag_window_frames *
                 1000000000ULL) / elapsed;
    queue_metric_snapshot(&g_queue, &queue_current, &queue_high_window,
                          &queue_high_total);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
            "bridge: metrics input_bps=%llu ts_bps=%llu fps_milli=%llu "
            "frame_max=%lu idr_max=%lu queue=%lu queue_hi=%lu "
            "queue_hi_total=%lu pts_step_max=%llu\n",
            input_bps, ts_bps, fps_milli,
            (unsigned long)g_diag_max_frame,
            (unsigned long)g_diag_max_keyframe,
            (unsigned long)queue_current,
            (unsigned long)queue_high_window,
            (unsigned long)queue_high_total,
            (unsigned long long)g_diag_max_pts_step);
    g_diag_window_start_us = now_us;
    g_diag_window_bytes = 0;
    g_diag_window_frames = 0;
    g_diag_ts_packets_origin = g_ts_packets_total;
    g_diag_max_frame = 0;
    g_diag_max_keyframe = 0;
    g_diag_max_pts_step = 0;
}

static void make_null_packet(uint8_t packet[TS_SIZE])
{
    packet[0] = 0x47;
    packet[1] = 0x1f;
    packet[2] = 0xff;
    packet[3] = (uint8_t)(0x10U | (g_cc_null++ & 0x0fU));
    memset(packet + 4, 0xff, TS_SIZE - 4U);
}

static int make_psi_packet(uint8_t packet[TS_SIZE], uint16_t pid,
                           unsigned int *continuity,
                           const uint8_t *section, size_t section_size)
{
    if (section_size + 5U > TS_SIZE) return -1;
    memset(packet, 0xff, TS_SIZE);
    packet[0] = 0x47;
    packet[1] = (uint8_t)(0x40U | ((pid >> 8) & 0x1fU));
    packet[2] = (uint8_t)pid;
    packet[3] = (uint8_t)(0x10U | ((*continuity)++ & 0x0fU));
    packet[4] = 0; /* pointer_field */
    memcpy(packet + 5, section, section_size);
    return 0;
}

static int make_pat_packet(uint8_t packet[TS_SIZE])
{
    uint8_t pat[16] = {
        0x00, 0xb0, 0x0d, 0x00, 0x01, 0xc1, 0x00, 0x00,
        0x00, 0x01, 0xe0, 0x10, 0, 0, 0, 0
    };
    uint32_t crc = mpeg_crc32(pat, 12);
    pat[12] = (uint8_t)(crc >> 24);
    pat[13] = (uint8_t)(crc >> 16);
    pat[14] = (uint8_t)(crc >> 8);
    pat[15] = (uint8_t)crc;
    return make_psi_packet(packet, 0, &g_cc_pat, pat, sizeof(pat));
}

static int make_pmt_packet(uint8_t packet[TS_SIZE])
{
    uint8_t pmt[21] = {
        0x02, 0xb0, 0x12, 0x00, 0x01, 0xc1, 0x00, 0x00,
        0xf0, 0x00, 0xf0, 0x00, 0x1b, 0xe0, 0x11, 0xf0,
        0x00, 0, 0, 0, 0
    };
    uint32_t crc = mpeg_crc32(pmt, 17);
    pmt[17] = (uint8_t)(crc >> 24);
    pmt[18] = (uint8_t)(crc >> 16);
    pmt[19] = (uint8_t)(crc >> 8);
    pmt[20] = (uint8_t)crc;
    return make_psi_packet(packet, PMT_PID, &g_cc_pmt, pmt, sizeof(pmt));
}

static void encode_pts(uint8_t target[5], uint64_t pts)
{
    pts &= 0x1ffffffffULL;
    target[0] = (uint8_t)(0x20U | (((pts >> 30) & 7U) << 1) | 1U);
    target[1] = (uint8_t)(pts >> 22);
    target[2] = (uint8_t)((((pts >> 15) & 0x7fU) << 1) | 1U);
    target[3] = (uint8_t)(pts >> 7);
    target[4] = (uint8_t)(((pts & 0x7fU) << 1) | 1U);
}

static void encode_pcr(uint8_t target[6], uint64_t base)
{
    base &= 0x1ffffffffULL;
    target[0] = (uint8_t)(base >> 25);
    target[1] = (uint8_t)(base >> 17);
    target[2] = (uint8_t)(base >> 9);
    target[3] = (uint8_t)(base >> 1);
    target[4] = (uint8_t)(((base & 1U) << 7) | 0x7eU);
    target[5] = 0;
}

static void make_pcr_packet(uint8_t packet[TS_SIZE], uint64_t base)
{
    memset(packet, 0xff, TS_SIZE);
    packet[0] = 0x47;
    packet[1] = (uint8_t)((PCR_PID >> 8) & 0x1fU);
    packet[2] = (uint8_t)PCR_PID;
    /* Adaptation-only PCR packets in the captured OEM stream keep CC at 0. */
    packet[3] = 0x20;
    packet[4] = 183;
    packet[5] = 0x10;
    encode_pcr(packet + 6, base);
}

static int enqueue_pes(const uint8_t *annexb, size_t annexb_size,
                       uint64_t pts, int keyframe)
{
    uint8_t *pes;
    uint8_t *packets;
    size_t pes_size = 14U + annexb_size;
    size_t packet_capacity = (pes_size + 175U) / 176U;
    size_t packet_count = 0;
    size_t offset = 0;
    int first = 1;
    int result;
    pes = (uint8_t *)malloc(pes_size);
    if (!pes) return -1;
    packets = (uint8_t *)malloc(packet_capacity * TS_SIZE);
    if (!packets) {
        free(pes);
        return -1;
    }
    pes[0] = 0x00; pes[1] = 0x00; pes[2] = 0x01; pes[3] = 0xe0;
    pes[4] = 0x00; pes[5] = 0x00; /* unbounded video PES */
    pes[6] = 0x80; pes[7] = 0x80; pes[8] = 0x05;
    encode_pts(pes + 9, pts);
    memcpy(pes + 14, annexb, annexb_size);

    while (offset < pes_size) {
        uint8_t *packet = packets + packet_count * TS_SIZE;
        size_t remaining = pes_size - offset;
        /* The PCR is carried on its own OEM PID.  The first video packet only
         * needs an adaptation flags byte for random-access/discontinuity. */
        size_t min_adaptation = first ? 2U : 0U;
        size_t max_payload = 184U - min_adaptation;
        size_t payload = remaining < max_payload ? remaining : max_payload;
        size_t adaptation = 184U - payload;
        size_t payload_offset = 4U;
        memset(packet, 0xff, TS_SIZE);
        packet[0] = 0x47;
        packet[1] = (uint8_t)((first ? 0x40U : 0U) |
                              ((VIDEO_PID >> 8) & 0x1fU));
        packet[2] = (uint8_t)VIDEO_PID;
        packet[3] = (uint8_t)(((adaptation ? 3U : 1U) << 4) |
                              (g_cc_video++ & 0x0fU));
        if (adaptation) {
            packet[4] = (uint8_t)(adaptation - 1U);
            payload_offset += adaptation;
            if (adaptation > 1U) {
                packet[5] = first ?
                    (uint8_t)((keyframe ? 0x40U : 0U) |
                              (g_discontinuity_pending ? 0x80U : 0U)) : 0;
            }
        }
        memcpy(packet + payload_offset, pes + offset, payload);
        offset += payload;
        ++packet_count;
        first = 0;
    }
    result = queue_push_batch(&g_queue, packets, packet_count);
    if (result == 0) {
        g_ts_packets_total += packet_count;
        if (g_discontinuity_pending) g_discontinuity_pending = 0;
    }
    free(packets);
    free(pes);
    return result;
}

static size_t annexb_start_code_size(const uint8_t *data, size_t size,
                                     size_t offset)
{
    if (offset + 4U <= size && data[offset] == 0 && data[offset + 1U] == 0 &&
        data[offset + 2U] == 0 && data[offset + 3U] == 1) return 4U;
    if (offset + 3U <= size && data[offset] == 0 && data[offset + 1U] == 0 &&
        data[offset + 2U] == 1) return 3U;
    return 0;
}

static size_t find_annexb_aud(const uint8_t *data, size_t size, size_t from)
{
    size_t i;
    for (i = from; i + 4U <= size; ++i) {
        size_t prefix = annexb_start_code_size(data, size, i);
        if (prefix && i + prefix < size &&
            (data[i + prefix] & 0x1fU) == 9U) return i;
    }
    return size;
}

static int annexb_has_idr(const uint8_t *data, size_t size)
{
    size_t i;
    for (i = 0; i + 4U <= size; ++i) {
        size_t prefix = annexb_start_code_size(data, size, i);
        if (prefix && i + prefix < size &&
            (data[i + prefix] & 0x1fU) == 5U) return 1;
    }
    return 0;
}

/* Play an already instrument-compatible Annex-B stream directly through the
 * existing TS/MOST muxer.  The stream must contain one AUD NAL per access
 * unit.  No decode or re-encode occurs on the head unit. */
static int play_intro_annexb(const char *path, CpmapIntroPolicy policy)
{
    FILE *file;
    int intro_marked = 0;
    uint8_t *data;
    long file_size_long;
    size_t file_size;
    size_t frame_start;
    uint64_t pts;
    uint64_t pts_step;
    unsigned int fps = INTRO_DEFAULT_FPS;
    const char *fps_text = getenv("CARPLAY_CLUSTER_INTRO_FPS");

    if (policy==CPMAP_INTRO_OFF || !path || !path[0]) return 0;
    if (fps_text && fps_text[0]) {
        unsigned long parsed = strtoul(fps_text, NULL, 10);
        if (parsed >= 10U && parsed <= 60U) fps = (unsigned int)parsed;
    }
    file = fopen(path, "rb");
    if (!file) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro unavailable path=%s error=%s\n",
                path, strerror(errno));
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0 ||
        (file_size_long = ftell(file)) <= 0 ||
        (size_t)file_size_long > INTRO_MAX_SIZE ||
        fseek(file, 0, SEEK_SET) != 0) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: invalid intro size path=%s\n", path);
        fclose(file);
        return -1;
    }
    file_size = (size_t)file_size_long;
    data = (uint8_t *)malloc(file_size);
    if (!data || fread(data, 1, file_size, file) != file_size) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro read failed path=%s\n", path);
        free(data);
        fclose(file);
        return -1;
    }
    fclose(file);
    frame_start = find_annexb_aud(data, file_size, 0);
    if (frame_start == file_size) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro contains no AUD path=%s\n", path);
        free(data);
        return -1;
    }

    g_discontinuity_pending = 1;
    pts = next_transport_pts_origin();
    pts_step = 90000ULL / fps;
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro-start path=%s bytes=%lu fps=%u pts=%llu\n",
            path, (unsigned long)file_size, fps, (unsigned long long)pts);
    while (!g_stop && frame_start < file_size) {
        size_t next = find_annexb_aud(data, file_size, frame_start + 4U);
        size_t frame_size = next - frame_start;
        int keyframe = annexb_has_idr(data + frame_start, frame_size);
        if (frame_size == 0 ||
            enqueue_pes(data + frame_start, frame_size, pts, keyframe) != 0) {
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                    "bridge: intro enqueue failed frame=%lu bytes=%lu queue=%lu\n",
                    g_intro_frames + 1U, (unsigned long)frame_size,
                    (unsigned long)queue_count(&g_queue));
            free(data);
            return -1;
        }
        if (!intro_marked && policy == CPMAP_INTRO_ONCE_PER_BOOT) {
            /* The first frame has entered the output queue.  From this point
             * onward the animation has started in this boot, even if a phone
             * disconnect interrupts the rest of this playback. */
            int marker = open(INTRO_ONCE_PATH,
                              O_WRONLY | O_CREAT | O_EXCL, 0600);
            if (marker >= 0) close(marker);
            else cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro-once marker failed: %s\n",
                         strerror(errno));
            intro_marked = 1;
        }
        ++g_intro_frames;
        g_last_pts = pts;
        pts += pts_step;
        frame_start = next;
        usleep(1000000U / fps);
    }
    free(data);
    if (g_stop) return -1;

    /* Ignore every live frame accumulated behind the animation.  The hook
     * will answer this marker with a fresh config+IDR pair, allowing a clean
     * predictive-chain and timestamp boundary. The intro now ends on its
     * full logo frame (no fade tail), then switches directly on that IDR. */
    g_intro_waiting_live_idr = 1;
    g_last_sequence = 0;
    g_media_origin_ntp = 0;
    g_local_origin_us = 0;
    g_pts_origin_value = next_transport_pts_origin();
    g_discontinuity_pending = 1;
    if (request_fresh_idr() != 0)
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro IDR request failed: %s\n",
                strerror(errno));
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro-complete frames=%lu next_pts=%llu\n",
            g_intro_frames, (unsigned long long)g_pts_origin_value);
    return 1;
}

static void free_config(AvcConfig *config)
{
    free(config->sps);
    free(config->pps);
    memset(config, 0, sizeof(*config));
}

static uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static int copy_unit(uint8_t **target, size_t *target_size,
                     const uint8_t *source, size_t size)
{
    uint8_t *copy = (uint8_t *)malloc(size ? size : 1U);
    if (!copy) return -1;
    memcpy(copy, source, size);
    free(*target);
    *target = copy;
    *target_size = size;
    return 0;
}

static int parse_avcc_config(AvcConfig *config, const uint8_t *data,
                             size_t size)
{
    AvcConfig next;
    size_t offset;
    unsigned int count;
    unsigned int i;
    memset(&next, 0, sizeof(next));
    if (size < 7U || data[0] != 1U) return -1;
    next.length_size = (data[4] & 3U) + 1U;
    count = data[5] & 0x1fU;
    offset = 6;
    for (i = 0; i < count; ++i) {
        uint16_t unit_size;
        if (offset + 2U > size) goto failed;
        unit_size = read_be16(data + offset);
        offset += 2;
        if (unit_size == 0 || offset + unit_size > size) goto failed;
        if (!next.sps && copy_unit(&next.sps, &next.sps_size,
                                   data + offset, unit_size) != 0) goto failed;
        offset += unit_size;
    }
    if (offset + 1U > size) goto failed;
    count = data[offset++];
    for (i = 0; i < count; ++i) {
        uint16_t unit_size;
        if (offset + 2U > size) goto failed;
        unit_size = read_be16(data + offset);
        offset += 2;
        if (unit_size == 0 || offset + unit_size > size) goto failed;
        if (!next.pps && copy_unit(&next.pps, &next.pps_size,
                                   data + offset, unit_size) != 0) goto failed;
        offset += unit_size;
    }
    if (!next.sps || !next.pps || next.length_size < 1U ||
        next.length_size > 4U) goto failed;
    next.valid = 1;
    free_config(config);
    *config = next;
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: avcC ready length=%u sps=%u pps=%u\n",
            config->length_size, (unsigned)config->sps_size,
            (unsigned)config->pps_size);
    return 0;
failed:
    free_config(&next);
    return -1;
}

static uint32_t read_nal_size(const uint8_t *data, unsigned int length_size)
{
    uint32_t value = 0;
    unsigned int i;
    for (i = 0; i < length_size; ++i) value = (value << 8) | data[i];
    return value;
}

static int avcc_to_annexb(const AvcConfig *config, const uint8_t *frame,
                          size_t frame_size, int keyframe,
                          uint8_t **out_data, size_t *out_size)
{
    static const uint8_t start_code[4] = {0, 0, 0, 1};
    static const uint8_t aud[2] = {0x09, 0xf0};
    uint8_t *output;
    size_t capacity = frame_size + config->sps_size + config->pps_size + 64U;
    size_t written = 0;
    size_t offset = 0;
    int has_idr = 0;
    if (!config->valid) return -1;
    output = (uint8_t *)malloc(capacity);
    if (!output) return -1;
    memcpy(output + written, start_code, 4); written += 4;
    memcpy(output + written, aud, sizeof(aud)); written += sizeof(aud);
    while (offset + config->length_size <= frame_size) {
        uint32_t nal_size = read_nal_size(frame + offset, config->length_size);
        offset += config->length_size;
        if (nal_size == 0 || nal_size > frame_size - offset) goto failed;
        if ((frame[offset] & 0x1fU) == 5U) has_idr = 1;
        offset += nal_size;
    }
    if (offset != frame_size) goto failed;
    if (keyframe || has_idr) {
        memcpy(output + written, start_code, 4); written += 4;
        memcpy(output + written, config->sps, config->sps_size);
        written += config->sps_size;
        memcpy(output + written, start_code, 4); written += 4;
        memcpy(output + written, config->pps, config->pps_size);
        written += config->pps_size;
    }
    offset = 0;
    while (offset + config->length_size <= frame_size) {
        uint32_t nal_size = read_nal_size(frame + offset, config->length_size);
        offset += config->length_size;
        memcpy(output + written, start_code, 4); written += 4;
        memcpy(output + written, frame + offset, nal_size); written += nal_size;
        offset += nal_size;
    }
    *out_data = output;
    *out_size = written;
    return has_idr ? 1 : 0;
failed:
    free(output);
    return -1;
}

static int write_full(int fd, const uint8_t *data, size_t size)
{
    size_t offset = 0;
    while (offset < size && !g_stop) {
        ssize_t count = write(fd, data + offset, size - offset);
        if (count > 0) offset += (size_t)count;
        else if (count < 0 && errno == EINTR) continue;
        else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            usleep(1000);
            continue;
        }
        else return -1;
    }
    return offset == size ? 0 : -1;
}

static void *most_writer_thread(void *context)
{
    const char *device = (const char *)context;
    uint32_t blocks = TS_BLOCK_PACKETS;
    uint32_t value = 0;
    uint8_t block[TS_BLOCK_SIZE];
    unsigned long written_blocks = 0;
    unsigned long report_blocks = 0;
    uint64_t report_start_us = monotonic_us();
    uint64_t output_packets = 0;
    uint64_t next_psi_packet = 0;
    uint64_t next_pcr_packet = 2;
    int pmt_pending = 0;
    int fd;
    int status;
#ifdef ALTSCREEN_BRIDGE_FILE_OUTPUT
    fd = open(device, O_WRONLY | O_CREAT | O_TRUNC, 0644);
#else
    fd = open64(device, O_WRONLY | O_NONBLOCK);
#endif
    if (fd < 0) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: open %s failed: %s\n", device, strerror(errno));
        g_stop = 1;
        return NULL;
    }
    g_most_fd = fd;
#ifndef ALTSCREEN_BRIDGE_FILE_OUTPUT
    status = devctl(fd, MLB_DEVCTL_GET_PACKET_SIZE, &value, sizeof(value), NULL);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: interface status=%d value=%u\n", status, value);
    status = devctl(fd, MLB_DEVCTL_GET_BLOCK_COUNT, &value, sizeof(value), NULL);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: packet-size status=%d value=%u\n", status, value);
    status = devctl(fd, MLB_DEVCTL_GET_QUEUE_SIZE, &value, sizeof(value), NULL);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: block-count status=%d value=%u\n", status, value);
    status = devctl(fd, MLB_DEVCTL_QUEUE_FLUSH, NULL, 0, NULL);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: queue-flush status=%d\n", status);
    status = devctl(fd, MLB_DEVCTL_START, &blocks, sizeof(blocks), NULL);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: start status=%d packets=%u\n", status, blocks);
    if (status != 0) {
        if ((int)g_most_fd == fd) {
            g_most_fd = -1;
            close(fd);
        }
        g_stop = 1;
        return NULL;
    }
#else
    (void)blocks;
    (void)value;
    (void)status;
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: portable file-output validation mode\n");
#endif
    while (!g_stop) {
        unsigned int i;
        uint64_t write_start_us;
        uint64_t write_elapsed_us;
#ifdef ALTSCREEN_BRIDGE_TEST_STALL_AFTER_BLOCKS
        if (written_blocks == ALTSCREEN_BRIDGE_TEST_STALL_AFTER_BLOCKS) {
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: test-writer-stall blocks=%lu us=%u\n",
                    written_blocks, (unsigned)ALTSCREEN_BRIDGE_TEST_STALL_US);
            usleep(ALTSCREEN_BRIDGE_TEST_STALL_US);
        }
#endif
        for (i = 0; i < TS_BLOCK_PACKETS; ++i) {
            uint8_t *packet = block + i * TS_SIZE;
            uint64_t pcr_base = TRANSPORT_PCR_ORIGIN_90K +
                (output_packets * TRANSPORT_TICKS_NUMERATOR) /
                TRANSPORT_TICKS_DENOMINATOR;
            if (pmt_pending) {
                if (make_pmt_packet(packet) != 0) make_null_packet(packet);
                pmt_pending = 0;
            } else if (output_packets >= next_psi_packet) {
                if (make_pat_packet(packet) != 0) make_null_packet(packet);
                pmt_pending = 1;
                next_psi_packet += TRANSPORT_INTERVAL_PACKETS;
            } else if (output_packets >= next_pcr_packet) {
                make_pcr_packet(packet, pcr_base);
                next_pcr_packet += TRANSPORT_INTERVAL_PACKETS;
            } else if (!queue_pop(&g_queue, packet)) {
                make_null_packet(packet);
            }
            ++output_packets;
        }
        g_transport_clock_90k = (uint32_t)(TRANSPORT_PCR_ORIGIN_90K +
            (output_packets * TRANSPORT_TICKS_NUMERATOR) /
            TRANSPORT_TICKS_DENOMINATOR);
        write_start_us = monotonic_us();
        if (write_full(fd, block, sizeof(block)) != 0) {
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: MOST write failed: %s\n", strerror(errno));
            g_stop = 1;
            break;
        }
        write_elapsed_us = monotonic_us() - write_start_us;
        if (write_elapsed_us > g_max_write_us)
            g_max_write_us = write_elapsed_us;
        record_write_latency(write_elapsed_us);
        if (write_elapsed_us >= 50000ULL) {
            ++g_slow_writes;
            g_slow_write_queue_depth = queue_count(&g_queue);
        }
        if (write_elapsed_us <= RECOVERY_FAST_WRITE_LIMIT_US)
            ++g_writer_fast_streak;
        else
            g_writer_fast_streak = 0;
#ifdef ALTSCREEN_BRIDGE_FILE_OUTPUT
        usleep(10000);
#endif
        ++written_blocks;
        g_writer_blocks_completed = written_blocks;
        if ((written_blocks % 100U) == 0U) {
            uint64_t report_now_us = monotonic_us();
            uint64_t report_elapsed_us = report_now_us - report_start_us;
            unsigned long long output_bps = report_elapsed_us ?
                ((unsigned long long)(written_blocks - report_blocks) *
                 TS_BLOCK_SIZE * 8ULL * 1000000ULL) / report_elapsed_us : 0;
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                    "bridge: blocks=%lu output_bps=%llu queue=%u frames=%lu "
                    "keys=%lu drops=%lu write_us=%llu write_us_max=%llu "
                    "slow_writes=%lu\n",
                    written_blocks, output_bps,
                    (unsigned)queue_count(&g_queue), g_frames,
                    g_keyframes, g_dropped_messages,
                    (unsigned long long)write_elapsed_us,
                    (unsigned long long)g_max_write_us, g_slow_writes);
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                    "bridge: hist_write_us="
                    "%lu,%lu,%lu,%lu,%lu,%lu "
                    "burst_100ms_max_bytes=%llu burst_100ms_max_frames=%lu "
                    "slow_write_queue=%lu\n",
                    g_write_hist[0], g_write_hist[1], g_write_hist[2],
                    g_write_hist[3], g_write_hist[4], g_write_hist[5],
                    g_burst_max_bytes, g_burst_max_frames,
                    (unsigned long)g_slow_write_queue_depth);
            reset_write_histogram();
            g_burst_max_bytes = 0;
            g_burst_max_frames = 0;
            g_slow_write_queue_depth = 0;
            report_blocks = written_blocks;
            report_start_us = report_now_us;
        }
    }
    if ((int)g_most_fd == fd) {
        g_most_fd = -1;
        close(fd);
    }
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: writer stopped blocks=%lu\n", written_blocks);
    return NULL;
}

static uint64_t next_transport_pts_origin(void)
{
    uint64_t value = (uint64_t)g_transport_clock_90k + PTS_TARGET_LEAD_90K;
    if (value <= g_last_pts) value = g_last_pts + 1ULL;
    return value;
}

static int process_frame(AvcConfig *config, const uint8_t *data, size_t size,
                         uint32_t flags, uint32_t sequence,
                         uint64_t media_timestamp_ntp)
{
    uint8_t *annexb = NULL;
    size_t annexb_size = 0;
    int result = avcc_to_annexb(config, data, size,
                                (flags & ALT_FRAME_FLAG_KEYFRAME) != 0,
                                &annexb, &annexb_size);
    int keyframe;
    int sequence_restart;
    int sequence_gap;
    uint64_t now_us;
    uint64_t pts;
    uint64_t pts_step;
    size_t queued_packets;
    if (result < 0) return -1;
    keyframe = result > 0 || (flags & ALT_FRAME_FLAG_KEYFRAME) != 0;
    now_us = monotonic_us();
    if (g_intro_waiting_live_idr) {
        if (!keyframe) {
            free(annexb);
            return 1;
        }
        g_intro_waiting_live_idr = 0;
        g_media_origin_ntp = media_timestamp_ntp;
        g_local_origin_us = now_us;
        g_pts_origin_value = next_transport_pts_origin();
        g_last_sequence = 0;
        g_discontinuity_pending = 1;
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                "bridge: intro-live-resume-idr sequence=%u pts_origin=%llu\n",
                sequence, (unsigned long long)g_pts_origin_value);
    }
    queued_packets = queue_count(&g_queue);
    sequence_restart = sequence <= 2U && g_last_sequence > 100U;
    sequence_gap = g_last_sequence != 0U && !sequence_restart &&
                   sequence != g_last_sequence + 1U;
    if (!g_waiting_for_keyframe && (sequence_restart || sequence_gap)) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                "bridge: predictive-chain-gap previous=%u current=%u restart=%d\n",
                g_last_sequence, sequence, sequence_restart);
        enter_low_latency_recovery(queued_packets, sequence, now_us);
        queued_packets = 0;
    }
    if (!g_waiting_for_keyframe &&
        queued_packets > TS_LOW_LATENCY_LIMIT_PACKETS) {
        enter_low_latency_recovery(queued_packets, sequence, now_us);
    }
    if (g_waiting_for_keyframe && !writer_recovered_after_reset()) {
        ++g_latency_frames_discarded;
        g_last_sequence = sequence;
        free(annexb);
        if ((g_latency_frames_discarded % 30U) == 1U)
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                    "bridge: latency-wait-writer sequence=%u "
                    "dropped_frames=%lu wait_us=%llu blocks=%lu streak=%lu\n",
                    sequence, g_latency_frames_discarded,
                    (unsigned long long)(now_us - g_latency_wait_start_us),
                    g_writer_blocks_completed, g_writer_fast_streak);
        return 1;
    }
    if (g_waiting_for_keyframe && !keyframe) {
        int marker_status = 1;
        if (!g_recovery_idr_requested) {
            marker_status = request_fresh_idr();
            if (marker_status == 0) g_recovery_idr_requested = 1;
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                    "bridge: latency-request-idr sequence=%u status=%d "
                    "wait_us=%llu blocks=%lu streak=%lu\n",
                    sequence, marker_status,
                    (unsigned long long)(now_us - g_latency_wait_start_us),
                    g_writer_blocks_completed, g_writer_fast_streak);
        }
        ++g_latency_frames_discarded;
        g_last_sequence = sequence;
        free(annexb);
        return 1;
    }
    if (g_waiting_for_keyframe && keyframe) {
        size_t discarded = queue_discard_all(&g_queue);
        uint64_t waited_us = now_us - g_latency_wait_start_us;
        g_latency_packets_discarded += discarded;
        /* Start a continuous, current PTS epoch instead of exposing the
         * decoder to the wall-clock gap occupied by discarded stale frames. */
        g_media_origin_ntp = media_timestamp_ntp;
        g_local_origin_us = now_us;
        g_pts_origin_value = next_transport_pts_origin();
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                "bridge: latency-resume-idr sequence=%u wait_us=%llu "
                "extra_discarded=%lu blocks=%lu streak=%lu\n",
                sequence, (unsigned long long)waited_us,
                (unsigned long)discarded,
                g_writer_blocks_completed, g_writer_fast_streak);
        g_waiting_for_keyframe = 0;
        g_recovery_idr_requested = 0;
    }
    if ((sequence <= 2U && g_last_sequence > 100U) ||
        (media_timestamp_ntp && g_media_origin_ntp &&
         media_timestamp_ntp < g_media_origin_ntp)) {
        g_media_origin_ntp = 0;
        g_local_origin_us = 0;
        g_pts_origin_value = next_transport_pts_origin();
    }
    if (media_timestamp_ntp) {
        if (g_media_origin_ntp == 0) {
            g_media_origin_ntp = media_timestamp_ntp;
            g_pts_origin_value = next_transport_pts_origin();
        }
        pts = g_pts_origin_value +
              ntp_delta_to_90khz(media_timestamp_ntp - g_media_origin_ntp);
    } else {
        if (g_local_origin_us == 0 || now_us < g_local_origin_us) {
            g_local_origin_us = now_us;
            g_pts_origin_value = next_transport_pts_origin();
        }
        pts = g_pts_origin_value +
              ((now_us - g_local_origin_us) * 90ULL) / 1000ULL;
    }
    /* The MediaLB clock keeps running even while CarPlay is idle.  Rebase a
     * burst or a late first frame so every access unit remains safely ahead
     * of the dedicated PCR instead of arriving with a timestamp in the past. */
    if (pts < (uint64_t)g_transport_clock_90k + PTS_MIN_LEAD_90K) {
        uint64_t target = next_transport_pts_origin();
        uint64_t shift = target > pts ? target - pts : 0;
        g_pts_origin_value += shift;
        pts += shift;
        ++g_pts_clock_rebases;
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                "bridge: pts-rebase sequence=%u shift=%llu pcr=%u pts=%llu "
                "count=%lu\n",
                sequence, (unsigned long long)shift,
                (unsigned)g_transport_clock_90k,
                (unsigned long long)pts, g_pts_clock_rebases);
    }
    if (pts <= g_last_pts) pts = g_last_pts + 1ULL;
    pts_step = g_last_pts ? pts - g_last_pts : 0;
    if (g_frames == 0 || (g_frames % 60U) == 0U)
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                "bridge: clock frame=%lu ntp=%llu pts=%llu step=%llu\n",
                g_frames + 1U,
                (unsigned long long)media_timestamp_ntp,
                (unsigned long long)pts,
                (unsigned long long)(g_last_pts ? pts - g_last_pts : 0));
    result = enqueue_pes(annexb, annexb_size, pts, keyframe);
    free(annexb);
    if (result != 0) return -1;
    ++g_frames;
    if (keyframe) ++g_keyframes;
    log_input_metrics(now_us, size, keyframe, pts_step);
    g_last_pts = pts;
    g_last_sequence = sequence;
    return 0;
}

static int accept_fragment(Reassembly *assembly,
                           const AltFrameDatagramHeader *header,
                           const uint8_t *payload, AvcConfig *config)
{
    if (header->total_size == 0 || header->total_size > MAX_MESSAGE ||
        header->chunk_size == 0 ||
        header->offset + header->chunk_size > header->total_size)
        return -1;
    if (assembly->active && assembly->sequence == header->sequence &&
        assembly->type == header->type &&
        assembly->total_size == header->total_size &&
        header->offset < assembly->received) {
        if (header->offset + header->chunk_size <= assembly->received &&
            memcmp(assembly->data + header->offset, payload,
                   header->chunk_size) == 0)
            return 0; /* duplicate fragment */
        return -1;
    }
    if (!assembly->active && assembly->have_last_completed &&
        assembly->last_completed_sequence == header->sequence &&
        assembly->last_completed_type == header->type)
        return 0; /* duplicate of the final fragment */
    if (!assembly->active || header->offset == 0 ||
        assembly->sequence != header->sequence ||
        assembly->type != header->type ||
        assembly->total_size != header->total_size) {
        if (header->offset != 0) return -1;
        if (header->total_size > assembly->capacity) {
            uint8_t *grown = (uint8_t *)realloc(assembly->data,
                                                header->total_size);
            if (!grown) return -1;
            assembly->data = grown;
            assembly->capacity = header->total_size;
        }
        assembly->active = 1;
        assembly->sequence = header->sequence;
        assembly->type = header->type;
        assembly->total_size = header->total_size;
        assembly->received = 0;
        assembly->flags = header->flags;
        assembly->media_timestamp_ns =
            ((uint64_t)header->media_timestamp_hi << 32) |
            header->media_timestamp_lo;
    }
    if (header->offset != assembly->received) return -1;
    memcpy(assembly->data + header->offset, payload, header->chunk_size);
    assembly->received += header->chunk_size;
    if (assembly->received == assembly->total_size) {
        int result;
        if (assembly->type == ALT_FRAME_TYPE_CONFIG)
            result = parse_avcc_config(config, assembly->data,
                                       assembly->total_size);
        else if (assembly->type == ALT_FRAME_TYPE_VIDEO)
            result = process_frame(config, assembly->data,
                                   assembly->total_size, assembly->flags,
                                   assembly->sequence,
                                   assembly->media_timestamp_ns);
        else result = -1;
        assembly->last_completed_sequence = assembly->sequence;
        assembly->last_completed_type = assembly->type;
        assembly->have_last_completed = 1;
        assembly->active = 0;
        return result;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *device = argc > 1 ? argv[1] : "/dev/mlb/isoTX2";
    const char *intro_path = getenv("CARPLAY_CLUSTER_INTRO");
    CpmapIntroPolicy intro_policy = cpmap_intro_read_policy(INTRO_POLICY_PATH);
    AltFrameRing *ring = MAP_FAILED;
    uint8_t *message = NULL;
    AvcConfig config;
    pthread_t writer;
    int fd = -1;
    int writer_started = 0;
    int cache_leased = 0;
    int result = 1;
    uint32_t generation = 0;
    uint32_t next_sequence = 1;
    unsigned int i;
    memset(&config, 0, sizeof(config));
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGPIPE, SIG_IGN);
    if (queue_init(&g_queue) != 0) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: queue allocation failed\n");
        goto done;
    }
    if (cpmap_cache_acquire() != 0) goto done;
    cache_leased = 1;
    fd = open(ALT_FRAME_RING_PATH, O_RDWR | O_CREAT, 0666);
    if (fd < 0 || ftruncate(fd, (off_t)sizeof(*ring)) != 0) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: open ring %s failed: %s\n",
                ALT_FRAME_RING_PATH, strerror(errno));
        goto done;
    }
    ring = (AltFrameRing *)mmap(NULL, sizeof(*ring), PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    close(fd);
    fd = -1;
    if (ring == MAP_FAILED) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: mmap ring failed: %s\n", strerror(errno));
        goto done;
    }
    generation = (ring->magic == ALT_FRAME_RING_MAGIC &&
                  ring->version == ALT_FRAME_RING_VERSION) ?
                 ring->generation + 1U : 1U;
    ring->magic = 0;
    __sync_synchronize();
    ring->version = ALT_FRAME_RING_VERSION;
    ring->generation = generation ? generation : 1U;
    ring->write_sequence = 0;
    for (i = 0; i < ALT_FRAME_RING_SLOT_COUNT; ++i)
        ring->slots[i].committed_sequence = 0;
    __sync_synchronize();
    ring->magic = ALT_FRAME_RING_MAGIC;
    generation = ring->generation;
    message = (uint8_t *)malloc(ALT_FRAME_RING_SLOT_DATA_MAX);
    if (!message) goto done;
    if (pthread_create(&writer, NULL, most_writer_thread, (void *)device) != 0) {
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: writer thread failed\n");
        goto done;
    }
    writer_started = 1;
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: shared ring %s bytes=%u slots=%u -> %s\n",
            ALT_FRAME_RING_PATH, (unsigned)sizeof(*ring),
            ALT_FRAME_RING_SLOT_COUNT, device);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: intro-policy=%s\n",
            intro_policy == CPMAP_INTRO_OFF ? "off" : intro_policy == CPMAP_INTRO_EVERY_CONNECT ? "every-connect" : "once-per-boot");
    if (!cpmap_intro_should_play(intro_policy, INTRO_ONCE_PATH)) {
        /* A second CarPlay session in the same boot must start at a fresh
         * config/IDR boundary, even though the animation is not replayed. */
        __sync_synchronize();
        next_sequence = ring->write_sequence + 1U;
        if (next_sequence == 0) next_sequence = 1U;
        g_intro_waiting_live_idr = 1;
        g_discontinuity_pending = 1;
        if (request_fresh_idr() != 0)
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: reconnect IDR request failed: %s\n",
                    strerror(errno));
        cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                "bridge: intro skipped this boot; waiting for fresh IDR after sequence=%u\n",
                next_sequence - 1U);
    } else {
        int intro_status = play_intro_annexb(intro_path, intro_policy);
        if (intro_status > 0) {
            __sync_synchronize();
            next_sequence = ring->write_sequence + 1U;
            if (next_sequence == 0) next_sequence = 1U;
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                    "bridge: intro skipped stale ring through sequence=%u\n",
                    next_sequence - 1U);
        }
    }
    while (!g_stop) {
        uint32_t available;
        if (ring->magic != ALT_FRAME_RING_MAGIC ||
            ring->version != ALT_FRAME_RING_VERSION) {
            usleep(1000);
            continue;
        }
        if (ring->generation != generation) {
            generation = ring->generation;
            /* A saved menu change applies on the next phone connection. */
            intro_policy = cpmap_intro_read_policy(INTRO_POLICY_PATH);
            next_sequence = 1;
            /* A fast unplug/replug can create a new AltScreen listener before
             * the supervisor has shut this bridge down.  Do not continue the
             * previous stream's predictive chain or timestamps. */
            queue_discard_all(&g_queue);
            g_intro_waiting_live_idr = 1;
            g_last_sequence = 0;
            g_media_origin_ntp = 0;
            g_local_origin_us = 0;
            g_discontinuity_pending = 1;
            if (request_fresh_idr() != 0)
                cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: reconnect IDR request failed: %s\n",
                        strerror(errno));
            cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT, "bridge: ring generation=%u; waiting for fresh IDR\n",
                    generation);
            if (cpmap_intro_should_play(intro_policy,INTRO_ONCE_PATH)) {
                /* The producer increments generation once per accepted phone
                 * video connection. Replay even if a quick reconnect kept this
                 * bridge alive; never keep video buffered behind the logo. */
                int intro_status = play_intro_annexb(intro_path, intro_policy);
                if (intro_status > 0) {
                    __sync_synchronize();
                    next_sequence = ring->write_sequence + 1U;
                    if (next_sequence == 0) next_sequence = 1U;
                    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                            "bridge: reconnect intro skipped stale ring through sequence=%u\n",
                            next_sequence - 1U);
                }
            }
        }
        __sync_synchronize();
        available = ring->write_sequence;
        while (!g_stop && next_sequence != 0 &&
               next_sequence <= available) {
            AltFrameRingSlot *slot =
                &ring->slots[(next_sequence - 1U) % ALT_FRAME_RING_SLOT_COUNT];
            uint32_t committed = slot->committed_sequence;
            uint32_t size;
            uint32_t type;
            uint32_t flags;
            uint32_t source_sequence;
            uint64_t media_timestamp_ns;
            int status;
            if (committed != next_sequence) {
                if (committed > next_sequence) {
                    g_dropped_messages += committed - next_sequence;
                    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
                            "bridge: ring overrun wanted=%u found=%u drops=%lu\n",
                            next_sequence, committed, g_dropped_messages);
                    next_sequence = committed;
                    continue;
                }
                break;
            }
            __sync_synchronize();
            size = slot->size;
            type = slot->type;
            flags = slot->flags;
            source_sequence = slot->source_sequence;
            media_timestamp_ns =
                ((uint64_t)slot->media_timestamp_hi << 32) |
                slot->media_timestamp_lo;
            if (size == 0 || size > ALT_FRAME_RING_SLOT_DATA_MAX) {
                ++g_dropped_messages;
                ++next_sequence;
                continue;
            }
            memcpy(message, slot->data, size);
            __sync_synchronize();
            if (slot->committed_sequence != next_sequence) continue;
            if (type == ALT_FRAME_TYPE_CONFIG)
                status = parse_avcc_config(&config, message, size);
            else if (type == ALT_FRAME_TYPE_VIDEO)
                status = process_frame(&config, message, size, flags,
                                       source_sequence, media_timestamp_ns);
            else status = -1;
            if (status != 0 && !(type == ALT_FRAME_TYPE_VIDEO && status == 1))
                ++g_dropped_messages;
            ++next_sequence;
        }
        usleep(1000);
    }
    result = 0;
done:
    g_stop = 1;
    g_queue.stopping = 1;
    if (fd >= 0) close(fd);
    if (writer_started) pthread_join(writer, NULL);
    cpmap_log_fprintf(stderr, CPMAP_BRIDGE_LOG_LIMIT,
            "bridge: complete frames=%lu keys=%lu drops=%lu input_bytes=%llu "
            "ts_packets=%llu frame_max=%lu idr_max=%lu queue_hi=%lu "
            "write_us_max=%llu slow_writes=%lu latency_resets=%lu "
            "latency_packets_discarded=%llu latency_frames_discarded=%lu "
            "pts_rebases=%lu intro_frames=%lu\n",
            g_frames, g_keyframes, g_dropped_messages,
            g_input_bytes_total, g_ts_packets_total,
            (unsigned long)g_max_frame_total,
            (unsigned long)g_max_keyframe_total,
            (unsigned long)g_queue.high_water_total,
            (unsigned long long)g_max_write_us, g_slow_writes,
            g_latency_resets, g_latency_packets_discarded,
            g_latency_frames_discarded, g_pts_clock_rebases,
            g_intro_frames);
    free(message);
    if (ring != MAP_FAILED) munmap(ring, sizeof(*ring));
    if (cache_leased) cpmap_cache_release();
    free_config(&config);
    if (g_queue.data) queue_destroy(&g_queue);
    return result;
}
