#ifndef CPMAP_MAP_PREFERENCE_H
#define CPMAP_MAP_PREFERENCE_H
/* Offline integration experiment. Same strict wire format as MapSwitchSettings.
 * This preference gates only supervisor takeover, not licenses, keys, media or
 * the main CarPlay session. No setting is written by this reader. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

struct cpmap_map_preference {
    int enabled;
    uint64_t generation;
    int source; /* 0=default, 1=primary, 2=previous */
};

static uint32_t cpmap_preference_crc(const unsigned char *p, size_t n)
{
    uint32_t crc = 0xffffffffU;
    size_t i;
    for (i = 0; i < n; ++i) {
        unsigned int bit;
        crc ^= p[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xedb88320U : 0);
    }
    return crc ^ 0xffffffffU;
}

static int cpmap_preference_file(const char *path,
                                struct cpmap_map_preference *out)
{
    struct stat st;
    char buf[193], canonical[193], *end;
    size_t n, body;
    unsigned long long generation;
    int enabled, length, read_error, close_error;
    FILE *file;
    if (lstat(path, &st) != 0) return errno == ENOENT ? 0 : -1;
    if (!S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 192) return -1;
    file = fopen(path, "rb");
    if (!file) return -1;
    n = fread(buf, 1, sizeof(buf), file);
    read_error = ferror(file);
    close_error = fclose(file);
    if (read_error || close_error != 0 || n != (size_t)st.st_size || n > 192)
        return -1;
    if (memchr(buf, 0, n) || n < 35 || memcmp(buf, "format=1\nenabled=", 17) != 0)
        return -1;
    buf[n] = 0;
    enabled = buf[17] - '0';
    if ((enabled != 0 && enabled != 1) || strncmp(buf + 18, "\ngeneration=", 12))
        return -1;
    if (buf[30] < '1' || buf[30] > '9') return -1;
    errno = 0;
    generation = strtoull(buf + 30, &end, 10);
    if (errno || generation > 0x7fffffffffffffffULL || *end != '\n') return -1;
    length = snprintf(canonical, sizeof(canonical),
                      "format=1\nenabled=%d\ngeneration=%llu\n",
                      enabled, generation);
    if (length <= 0 || length >= (int)sizeof(canonical)) return -1;
    body = (size_t)length;
    length = snprintf(canonical + body, sizeof(canonical) - body, "crc32=%x\n",
                      (unsigned int)cpmap_preference_crc((unsigned char *)canonical, body));
    if (length <= 0 || body + (size_t)length != n || memcmp(buf, canonical, n)) return -1;
    out->enabled = enabled;
    out->generation = (uint64_t)generation;
    return 1;
}

static int cpmap_read_map_preference(const char *directory,
                                    struct cpmap_map_preference *out)
{
    char path[512];
    int first, second;
    if (snprintf(path, sizeof(path), "%s/maps.state", directory) >= (int)sizeof(path)) return -1;
    first = cpmap_preference_file(path, out);
    if (first == 1) { out->source = 1; return 1; }
    if (snprintf(path, sizeof(path), "%s/maps.previous", directory) >= (int)sizeof(path)) return -1;
    second = cpmap_preference_file(path, out);
    if (second == 1) { out->source = 2; return 1; }
    if (first == 0 && second == 0) {
        out->enabled = 1; out->generation = 0; out->source = 0;
        return 0;
    }
    return -1; /* Caller keeps its last valid state, and logs the read error. */
}
#endif
