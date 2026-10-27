#ifndef CARPLAY_MIRROR_PROTOCOL_H
#define CARPLAY_MIRROR_PROTOCOL_H

#include <stdint.h>

#define CARPLAY_MIRROR_MAGIC 0x31565043u /* "CPV1" on little-endian ARM */
#define CARPLAY_MIRROR_VERSION 1u
#define CARPLAY_MIRROR_DEFAULT_PORT 15557u
#define CARPLAY_MIRROR_MAX_ACCESS_UNIT (2u * 1024u * 1024u)

enum CarPlayMirrorMessageType {
    CARPLAY_MIRROR_CONFIG = 1,
    CARPLAY_MIRROR_FRAME = 2,
    CARPLAY_MIRROR_STOP = 3
};

typedef struct CarPlayMirrorHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint32_t payload_size;
    uint32_t sequence;
} CarPlayMirrorHeader;

typedef struct CarPlayMirrorConfig {
    uint32_t width;
    uint32_t height;
    uint32_t fps_milli;
    uint32_t reserved;
} CarPlayMirrorConfig;

typedef struct CarPlayMirrorFrame {
    uint64_t pts;
    uint32_t data_size;
    uint8_t append_markers;
    uint8_t reserved[3];
} CarPlayMirrorFrame;

/* Both endpoints are ARM/QNX builds, but keep the wire ABI explicit so an
 * accidental packing or field edit becomes a compile error. */
typedef char CarPlayMirrorHeaderSizeMustBe16[
    sizeof(CarPlayMirrorHeader) == 16 ? 1 : -1];
typedef char CarPlayMirrorConfigSizeMustBe16[
    sizeof(CarPlayMirrorConfig) == 16 ? 1 : -1];
typedef char CarPlayMirrorFrameSizeMustBe16[
    sizeof(CarPlayMirrorFrame) == 16 ? 1 : -1];

#endif
