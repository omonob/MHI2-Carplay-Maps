#ifndef ALTSCREEN_FRAME_PROTOCOL_H
#define ALTSCREEN_FRAME_PROTOCOL_H

#include <stdint.h>

#define ALT_FRAME_MAGIC 0x43504d46U /* "CPMF" */
#define ALT_FRAME_VERSION 1U
#define ALT_FRAME_PORT 17201U
/* Keep the UDP payload below the MHI2 QNX loopback MTU so the kernel never
 * has to IP-fragment a CarPlay frame chunk. */
#define ALT_FRAME_CHUNK_MAX 1200U

#define ALT_FRAME_TYPE_CONFIG 1U
#define ALT_FRAME_TYPE_VIDEO 2U

#define ALT_FRAME_FLAG_KEYFRAME 0x00000001U

#ifndef ALT_FRAME_RING_PATH
#define ALT_FRAME_RING_PATH "/ramdisk/carplay-alt-ring.bin"
#endif

/* Consumer-to-producer recovery signal. The MOST bridge creates this marker
 * after discarding a stale TS backlog; the AirPlay hook consumes it and asks
 * the phone for a genuine AltScreen IDR. */
#ifndef ALT_FRAME_FORCE_IDR_PATH
#define ALT_FRAME_FORCE_IDR_PATH "/ramdisk/carplay-alt-force-idr.request"
#endif

#define ALT_FRAME_RING_MAGIC 0x43505247U /* "CPRG" */
#define ALT_FRAME_RING_VERSION 1U
#define ALT_FRAME_RING_SLOT_COUNT 64U
#define ALT_FRAME_RING_SLOT_DATA_MAX (256U * 1024U)

typedef struct AltFrameRingSlot {
    volatile uint32_t committed_sequence;
    uint32_t type;
    uint32_t size;
    uint32_t flags;
    uint32_t source_sequence;
    uint32_t media_timestamp_lo;
    uint32_t media_timestamp_hi;
    uint8_t data[ALT_FRAME_RING_SLOT_DATA_MAX];
} AltFrameRingSlot;

typedef struct AltFrameRing {
    volatile uint32_t magic;
    uint32_t version;
    volatile uint32_t generation;
    volatile uint32_t write_sequence;
    AltFrameRingSlot slots[ALT_FRAME_RING_SLOT_COUNT];
} AltFrameRing;

typedef struct AltFrameDatagramHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint32_t sequence;
    uint32_t total_size;
    uint32_t offset;
    uint32_t chunk_size;
    uint32_t flags;
    uint32_t media_timestamp_lo;
    uint32_t media_timestamp_hi;
} AltFrameDatagramHeader;

#endif
