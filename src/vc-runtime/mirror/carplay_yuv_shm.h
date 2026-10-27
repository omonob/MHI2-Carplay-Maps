#ifndef CARPLAY_YUV_SHM_H
#define CARPLAY_YUV_SHM_H

#include <stdint.h>

#ifndef CARPLAY_YUV_SHM_PATH
#define CARPLAY_YUV_SHM_PATH "/dev/shmem/carplay_cluster_v1_yuv"
#endif
#define CARPLAY_YUV_MAGIC 0x31565559u /* "YUV1" */
#define CARPLAY_YUV_VERSION 2u
/* Keep the stock CarPlay mixer resolution in shared memory.  The GLES
 * renderer scales it to the 800x480 cluster viewport in hardware. */
#define CARPLAY_YUV_WIDTH 1264u
#define CARPLAY_YUV_HEIGHT 640u
#define CARPLAY_YUV_Y_SIZE (CARPLAY_YUV_WIDTH * CARPLAY_YUV_HEIGHT)
#define CARPLAY_YUV_UV_SIZE ((CARPLAY_YUV_WIDTH / 2u) * \
                             (CARPLAY_YUV_HEIGHT / 2u))
#define CARPLAY_YUV_FRAME_SIZE (CARPLAY_YUV_Y_SIZE + \
                                2u * CARPLAY_YUV_UV_SIZE)
#define CARPLAY_YUV_SLOT_COUNT 2u

typedef struct CarPlayYuvSharedHeader {
    uint32_t magic;
    uint32_t version;
    volatile uint32_t sequence;
    volatile uint32_t active_slot;
    volatile uint32_t stream_active;
    uint32_t source_width;
    uint32_t source_height;
    uint32_t output_width;
    uint32_t output_height;
    uint32_t reserved_align;
    uint64_t timestamp_us;
    uint64_t published;
} CarPlayYuvSharedHeader;

#define CARPLAY_YUV_SHM_SIZE \
    (sizeof(CarPlayYuvSharedHeader) + \
     CARPLAY_YUV_SLOT_COUNT * CARPLAY_YUV_FRAME_SIZE)

typedef char CarPlayYuvSharedHeaderSizeMustBe56[
    sizeof(CarPlayYuvSharedHeader) == 56 ? 1 : -1];

static uint8_t *CarPlayYuvSlot(CarPlayYuvSharedHeader *header,
                               uint32_t slot)
{
    return (uint8_t *)(header + 1) +
           (slot % CARPLAY_YUV_SLOT_COUNT) * CARPLAY_YUV_FRAME_SIZE;
}

#endif
