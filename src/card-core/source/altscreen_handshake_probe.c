#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "cpmap_resources.h"

#ifdef ALTSCREEN_FORWARD_SHM_RING
#include "altscreen_frame_protocol.h"
#endif

/*
 * MHI2 CarPlay AltScreen handshake probe.
 *
 * Stage 1 exports AirPlayCopyServerInfo and AirPlayReceiverSessionSetup.
 * The passive-capture build additionally exports AirPlayReceiverSessionStart
 * so it can activate the advertised display after RECORD. It does not
 * create/register a QNX Screen, touch NvSS, decode video, or change the stock
 * type-110 stream.
 *
 * Stage-1 objective:
 *   1. Advertise a complete type-111 cluster display in /info.
 *   2. Return enabledFeatures=[iAPChannel, viewAreas, altScreen] for session
 *      SETUP, matching LIVI's non-HEVC response.
 *   3. Log whether the phone subsequently requests a type-111 stream.
 *
 * The stock 210.81 library does not implement type 111, so a successful proof
 * may end with an expected unsupported SETUP and CarPlay reconnect request.
 */

typedef int32_t OSStatus;

typedef OSStatus (*SessionSetupFn)(void *, const void *, void **);
typedef OSStatus (*SessionStartFn)(void *, void *);
typedef void (*SessionTearDownFn)(void *, const void *, OSStatus,
                                  unsigned char *);
typedef void (*ScreenUpdateStateFn)(void *, int);
typedef void (*SessionCommandCompletionFn)(OSStatus, const void *, void *);
typedef OSStatus (*SessionSendCommandFn)(void *, const void *,
                                         SessionCommandCompletionFn, void *);
typedef void *(*CopyServerInfoFn)(void *, const void *, const void *, OSStatus *);
typedef int (*FPrintFFn)(FILE *, const char *, ...);
typedef void *(*CFDictionaryCreateMutableFn)(void *, long, const void *, const void *);
typedef void *(*CFArrayCreateMutableFn)(void *, long, const void *);
typedef void *(*CFArrayCreateMutableCopyFn)(void *, long, const void *);
typedef const void *(*CFDictionaryGetValueFn)(const void *, const void *);
typedef long (*CFArrayGetCountFn)(const void *);
typedef const void *(*CFArrayGetValueAtIndexFn)(const void *, long);
typedef void (*CFArrayAppendValueFn)(void *, const void *);
typedef void (*CFDictionarySetValueFn)(void *, const void *, const void *);
typedef OSStatus (*CFDictionarySetInt64Fn)(void *, const void *, int64_t);
typedef int64_t (*CFDictionaryGetInt64Fn)(const void *, const void *, OSStatus *);
typedef void *(*CFNumberCreateInt64Fn)(int64_t);
typedef OSStatus (*CFDictionarySetCStringFn)(void *, const void *, const char *, size_t);
typedef void *(*CFStringCreateWithCStringFn)(void *, const char *, uint32_t);
typedef void *(*CFDataCreateFn)(void *, const uint8_t *, long);
typedef const void *(*CFRetainFn)(const void *);
typedef void (*CFReleaseFn)(const void *);
typedef void (*DeriveScreenAESFn)(const void *, size_t, uint64_t,
                                  uint8_t[16], uint8_t[16]);
typedef OSStatus (*AESCTRInitFn)(void *, const uint8_t[16],
                                 const uint8_t[16]);
typedef OSStatus (*AESCTRUpdateFn)(void *, const void *, size_t, void *);
typedef void (*AESCTRFinalFn)(void *);

#define UTF8_ENCODING 0x08000100U
#define ALT_UUID "b7e6c5a0-2222-4000-8000-000000000002"
#include "navigation_card_scene.h"
#ifndef ALTSCREEN_MAP_CONTROL_PORT
#define ALTSCREEN_MAP_CONTROL_PORT 7032
#endif
#define AIRPLAY_FEATURE_CARPLAY_CONTROL (INT64_C(1) << 37)
#define AIRPLAY_FEATURE_HK_PAIRING_AND_ENCRYPT (INT64_C(1) << 38)

/* Alternate-display geometry. Build scripts may override these values for a
 * specific instrument cluster without changing the protocol implementation. */
#ifndef ALTSCREEN_DISPLAY_WIDTH
#define ALTSCREEN_DISPLAY_WIDTH 800
#endif
#ifndef ALTSCREEN_DISPLAY_HEIGHT
#define ALTSCREEN_DISPLAY_HEIGHT 480
#endif
#ifndef ALTSCREEN_DISPLAY_MAX_FPS
#define ALTSCREEN_DISPLAY_MAX_FPS 25
#endif
#ifndef ALTSCREEN_DISPLAY_PHYSICAL_WIDTH
#define ALTSCREEN_DISPLAY_PHYSICAL_WIDTH 160
#endif
#ifndef ALTSCREEN_DISPLAY_PHYSICAL_HEIGHT
#define ALTSCREEN_DISPLAY_PHYSICAL_HEIGHT 96
#endif

/* The view area describes the full drawable region. The nested safe area
 * tells the iPhone where it must keep navigation-critical content. Keeping
 * these as build-time values lets 790 and 791 use independent geometry while
 * preserving the same H.264 transport and decoder path. */
#ifndef ALTSCREEN_VIEW_ORIGIN_X
#define ALTSCREEN_VIEW_ORIGIN_X 0
#endif
#ifndef ALTSCREEN_VIEW_ORIGIN_Y
#define ALTSCREEN_VIEW_ORIGIN_Y 0
#endif
#ifndef ALTSCREEN_VIEW_WIDTH
#define ALTSCREEN_VIEW_WIDTH ALTSCREEN_DISPLAY_WIDTH
#endif
#ifndef ALTSCREEN_VIEW_HEIGHT
#define ALTSCREEN_VIEW_HEIGHT ALTSCREEN_DISPLAY_HEIGHT
#endif
#ifndef ALTSCREEN_SAFE_ORIGIN_X
#define ALTSCREEN_SAFE_ORIGIN_X ALTSCREEN_VIEW_ORIGIN_X
#endif
#ifndef ALTSCREEN_SAFE_ORIGIN_Y
#define ALTSCREEN_SAFE_ORIGIN_Y ALTSCREEN_VIEW_ORIGIN_Y
#endif
#ifndef ALTSCREEN_SAFE_WIDTH
#define ALTSCREEN_SAFE_WIDTH ALTSCREEN_VIEW_WIDTH
#endif
#ifndef ALTSCREEN_SAFE_HEIGHT
#define ALTSCREEN_SAFE_HEIGHT ALTSCREEN_VIEW_HEIGHT
#endif

/* R15 and later may advertise more than one view area for a display.  Keep
 * this disabled by default so the proven MHI2/R14 single-area build remains
 * byte-for-byte equivalent at the protocol level.  An experimental build can
 * enable a second layout and select its initial index during the /info
 * handshake.  Runtime selection still requires the R15 view-area transition
 * command; merely advertising two entries does not switch them automatically. */
#ifndef ALTSCREEN_SECOND_VIEW_ENABLE
#define ALTSCREEN_SECOND_VIEW_ENABLE 0
#endif
#if ALTSCREEN_SECOND_VIEW_ENABLE
#ifndef ALTSCREEN_SECOND_VIEW_ORIGIN_X
#define ALTSCREEN_SECOND_VIEW_ORIGIN_X ALTSCREEN_VIEW_ORIGIN_X
#endif
#ifndef ALTSCREEN_SECOND_VIEW_ORIGIN_Y
#define ALTSCREEN_SECOND_VIEW_ORIGIN_Y ALTSCREEN_VIEW_ORIGIN_Y
#endif
#ifndef ALTSCREEN_SECOND_VIEW_WIDTH
#define ALTSCREEN_SECOND_VIEW_WIDTH ALTSCREEN_VIEW_WIDTH
#endif
#ifndef ALTSCREEN_SECOND_VIEW_HEIGHT
#define ALTSCREEN_SECOND_VIEW_HEIGHT ALTSCREEN_VIEW_HEIGHT
#endif
#ifndef ALTSCREEN_SECOND_SAFE_ORIGIN_X
#define ALTSCREEN_SECOND_SAFE_ORIGIN_X ALTSCREEN_SECOND_VIEW_ORIGIN_X
#endif
#ifndef ALTSCREEN_SECOND_SAFE_ORIGIN_Y
#define ALTSCREEN_SECOND_SAFE_ORIGIN_Y ALTSCREEN_SECOND_VIEW_ORIGIN_Y
#endif
#ifndef ALTSCREEN_SECOND_SAFE_WIDTH
#define ALTSCREEN_SECOND_SAFE_WIDTH ALTSCREEN_SECOND_VIEW_WIDTH
#endif
#ifndef ALTSCREEN_SECOND_SAFE_HEIGHT
#define ALTSCREEN_SECOND_SAFE_HEIGHT ALTSCREEN_SECOND_VIEW_HEIGHT
#endif
#ifndef ALTSCREEN_INITIAL_VIEW_AREA
#define ALTSCREEN_INITIAL_VIEW_AREA 0
#endif
#endif

#if ALTSCREEN_VIEW_ORIGIN_X < 0 || ALTSCREEN_VIEW_ORIGIN_Y < 0 || \
    ALTSCREEN_VIEW_WIDTH <= 0 || ALTSCREEN_VIEW_HEIGHT <= 0 || \
    ALTSCREEN_VIEW_ORIGIN_X + ALTSCREEN_VIEW_WIDTH > ALTSCREEN_DISPLAY_WIDTH || \
    ALTSCREEN_VIEW_ORIGIN_Y + ALTSCREEN_VIEW_HEIGHT > ALTSCREEN_DISPLAY_HEIGHT
#error "AltScreen view area lies outside the advertised display"
#endif

#if ALTSCREEN_SAFE_ORIGIN_X < ALTSCREEN_VIEW_ORIGIN_X || \
    ALTSCREEN_SAFE_ORIGIN_Y < ALTSCREEN_VIEW_ORIGIN_Y || \
    ALTSCREEN_SAFE_WIDTH <= 0 || ALTSCREEN_SAFE_HEIGHT <= 0 || \
    ALTSCREEN_SAFE_ORIGIN_X + ALTSCREEN_SAFE_WIDTH > \
        ALTSCREEN_VIEW_ORIGIN_X + ALTSCREEN_VIEW_WIDTH || \
    ALTSCREEN_SAFE_ORIGIN_Y + ALTSCREEN_SAFE_HEIGHT > \
        ALTSCREEN_VIEW_ORIGIN_Y + ALTSCREEN_VIEW_HEIGHT
#error "AltScreen safe area lies outside the view area"
#endif

#if ALTSCREEN_SECOND_VIEW_ENABLE
#if ALTSCREEN_SECOND_VIEW_ORIGIN_X < 0 || \
    ALTSCREEN_SECOND_VIEW_ORIGIN_Y < 0 || \
    ALTSCREEN_SECOND_VIEW_WIDTH <= 0 || \
    ALTSCREEN_SECOND_VIEW_HEIGHT <= 0 || \
    ALTSCREEN_SECOND_VIEW_ORIGIN_X + ALTSCREEN_SECOND_VIEW_WIDTH > \
        ALTSCREEN_DISPLAY_WIDTH || \
    ALTSCREEN_SECOND_VIEW_ORIGIN_Y + ALTSCREEN_SECOND_VIEW_HEIGHT > \
        ALTSCREEN_DISPLAY_HEIGHT
#error "AltScreen second view area lies outside the advertised display"
#endif

#if ALTSCREEN_SECOND_SAFE_ORIGIN_X < ALTSCREEN_SECOND_VIEW_ORIGIN_X || \
    ALTSCREEN_SECOND_SAFE_ORIGIN_Y < ALTSCREEN_SECOND_VIEW_ORIGIN_Y || \
    ALTSCREEN_SECOND_SAFE_WIDTH <= 0 || \
    ALTSCREEN_SECOND_SAFE_HEIGHT <= 0 || \
    ALTSCREEN_SECOND_SAFE_ORIGIN_X + ALTSCREEN_SECOND_SAFE_WIDTH > \
        ALTSCREEN_SECOND_VIEW_ORIGIN_X + ALTSCREEN_SECOND_VIEW_WIDTH || \
    ALTSCREEN_SECOND_SAFE_ORIGIN_Y + ALTSCREEN_SECOND_SAFE_HEIGHT > \
        ALTSCREEN_SECOND_VIEW_ORIGIN_Y + ALTSCREEN_SECOND_VIEW_HEIGHT
#error "AltScreen second safe area lies outside the second view area"
#endif

#if ALTSCREEN_INITIAL_VIEW_AREA < 0 || ALTSCREEN_INITIAL_VIEW_AREA > 1
#error "AltScreen initial view-area index must be 0 or 1"
#endif
#endif

/* Ask the sender for a genuine alternate-display IDR every 120 received
 * frames. The cluster is less tolerant of a long prediction chain than a
 * normal AirPlay display; never relabel or alter a P frame locally. */
#ifndef ALTSCREEN_FORCE_KEYFRAME_INTERVAL
#define ALTSCREEN_FORCE_KEYFRAME_INTERVAL 120ULL
#endif

/* Session layout is firmware-specific.  The established MU profiles store the
 * screen AES key at session+0x1c8, while exact P0627 (JXE 52754105) stores it
 * at session+0x1c0.  Keep 0x1c8 as the default and override only for the
 * isolated MU0627 build. */
#ifndef MHI2_SESSION_AES_KEY_OFFSET
#define MHI2_SESSION_AES_KEY_OFFSET 456U
#endif

static int g_in_log;

static const char *log_path(void)
{
    const char *value = getenv("CARPLAY_ALTSCREEN_LOG");
    return value && value[0] ? value :
        "/ramdisk/carplay_altscreen_handshake/handshake.log";
}

static uint64_t monotonic_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL +
           (uint64_t)ts.tv_nsec / 1000ULL;
}

static void log_status(const char *event, const void *pointer, OSStatus status)
{
    FILE *file;
    if (g_in_log) return;
    g_in_log = 1;
    file = cpmap_log_open(log_path(), CPMAP_VIDEO_LOG_LIMIT);
    if (file) {
        fprintf(file,
                "t_us=%llu pid=%ld event=%s ptr=%p status=%d (0x%08x)\n",
                (unsigned long long)monotonic_us(), (long)getpid(),
                event ? event : "?", pointer, (int)status,
                (unsigned int)status);
        cpmap_log_close(file, CPMAP_VIDEO_LOG_LIMIT);
    }
    g_in_log = 0;
}

static void log_object(const char *event, const void *object)
{
    FILE *file;
    FPrintFFn print_object;
    if (g_in_log) return;
    g_in_log = 1;
    file = cpmap_log_open(log_path(), CPMAP_VIDEO_LOG_LIMIT);
    if (!file) {
        g_in_log = 0;
        return;
    }
    fprintf(file, "t_us=%llu pid=%ld event=%s ptr=%p",
            (unsigned long long)monotonic_us(), (long)getpid(),
            event ? event : "?", object);
    print_object = (FPrintFFn)dlsym(RTLD_NEXT, "FPrintF");
    if (print_object && object) {
        fputs(" value=", file);
        print_object(file, "%@", object);
    }
    fputc('\n', file);
    cpmap_log_close(file, CPMAP_VIDEO_LOG_LIMIT);
    g_in_log = 0;
}

static int enabled(void)
{
    const char *value = getenv("CARPLAY_ALTSCREEN_ENABLE");
    return value && value[0] && strcmp(value, "0") != 0;
}

static void *resolve(const char *name)
{
    return dlsym(RTLD_NEXT, name);
}

static void *make_string(const char *value)
{
    CFStringCreateWithCStringFn create_string =
        (CFStringCreateWithCStringFn)resolve("CFStringCreateWithCString");
    return create_string ? create_string(NULL, value, UTF8_ENCODING) : NULL;
}

static void release_cf(const void *object)
{
    CFReleaseFn release;
    if (!object) return;
    release = (CFReleaseFn)resolve("CFRelease");
    if (release) release(object);
}

static const void *retain_cf(const void *object)
{
    CFRetainFn retain;
    if (!object) return NULL;
    retain = (CFRetainFn)resolve("CFRetain");
    return retain ? retain(object) : NULL;
}

static void *create_dictionary(void)
{
    CFDictionaryCreateMutableFn create =
        (CFDictionaryCreateMutableFn)resolve("CFDictionaryCreateMutable");
    const void *keys = resolve("kCFLDictionaryKeyCallBacksCFLTypes");
    const void *values = resolve("kCFLDictionaryValueCallBacksCFLTypes");
    if (!create || !keys || !values) return NULL;
    return create(NULL, 0, keys, values);
}

static void *create_array(void)
{
    CFArrayCreateMutableFn create =
        (CFArrayCreateMutableFn)resolve("CFArrayCreateMutable");
    const void *callbacks = resolve("kCFLArrayCallBacksCFLTypes");
    if (!create || !callbacks) return NULL;
    return create(NULL, 0, callbacks);
}

static OSStatus set_int(void *dict, const char *name, int64_t value)
{
    CFDictionarySetInt64Fn set_value =
        (CFDictionarySetInt64Fn)resolve("CFDictionarySetInt64");
    void *key = make_string(name);
    OSStatus status = -1;
    if (set_value && key) status = set_value(dict, key, value);
    release_cf(key);
    return status;
}

static OSStatus set_cstring(void *dict, const char *name, const char *value)
{
    CFDictionarySetCStringFn set_value =
        (CFDictionarySetCStringFn)resolve("CFDictionarySetCString");
    void *key = make_string(name);
    OSStatus status = -1;
    if (set_value && key)
        status = set_value(dict, key, value, (size_t)-1);
    release_cf(key);
    return status;
}

static OSStatus set_object(void *dict, const char *name, const void *value)
{
    CFDictionarySetValueFn set_value =
        (CFDictionarySetValueFn)resolve("CFDictionarySetValue");
    void *key = make_string(name);
    OSStatus status = -1;
    if (set_value && key && value) {
        set_value(dict, key, value);
        status = 0;
    }
    release_cf(key);
    return status;
}

static const void *get_object(const void *dict, const char *name)
{
    CFDictionaryGetValueFn get_value =
        (CFDictionaryGetValueFn)resolve("CFDictionaryGetValue");
    void *key = make_string(name);
    const void *value = NULL;
    if (get_value && key) value = get_value(dict, key);
    release_cf(key);
    return value;
}

static OSStatus append_string(void *array, const char *value)
{
    CFArrayAppendValueFn append =
        (CFArrayAppendValueFn)resolve("CFArrayAppendValue");
    void *string = make_string(value);
    if (!append || !string) {
        release_cf(string);
        return -1;
    }
    append(array, string);
    release_cf(string);
    return 0;
}

static OSStatus append_int(void *array, int64_t value)
{
    CFArrayAppendValueFn append =
        (CFArrayAppendValueFn)resolve("CFArrayAppendValue");
    CFNumberCreateInt64Fn create =
        (CFNumberCreateInt64Fn)resolve("CFNumberCreateInt64");
    void *number = create ? create(value) : NULL;
    if (!append || !number) {
        release_cf(number);
        return -1;
    }
    append(array, number);
    release_cf(number);
    return 0;
}

static int64_t get_int(const void *dict, const char *name, OSStatus *out_status)
{
    CFDictionaryGetInt64Fn get_value =
        (CFDictionaryGetInt64Fn)resolve("CFDictionaryGetInt64");
    void *key = make_string(name);
    OSStatus status = -1;
    int64_t value = 0;
    if (get_value && key) value = get_value(dict, key, &status);
    release_cf(key);
    if (out_status) *out_status = status;
    return value;
}

static void *create_area(int x, int y, int width, int height)
{
    void *dict = create_dictionary();
    OSStatus status = dict ? 0 : -1;
    if (!status) status = set_int(dict, "widthPixels", width);
    if (!status) status = set_int(dict, "heightPixels", height);
    if (!status) status = set_int(dict, "originXPixels", x);
    if (!status) status = set_int(dict, "originYPixels", y);
    if (status) {
        release_cf(dict);
        return NULL;
    }
    return dict;
}

static void *create_alt_display(void)
{
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
    void *display=create_dictionary(),*views=create_array();
    CFArrayAppendValueFn append=(CFArrayAppendValueFn)resolve("CFArrayAppendValue");
    OSStatus status=display&&views&&append?0:-1;
    int i,j,position;
    card_refresh_preference();position=card_snapshot().position;
    if(!status)status=set_cstring(display,"uuid",ALT_UUID);
    if(!status)status=set_int(display,"type",111);
    if(!status)status=set_int(display,"maxFPS",ALTSCREEN_DISPLAY_MAX_FPS);
    if(!status)status=set_int(display,"widthPixels",ALTSCREEN_DISPLAY_WIDTH);
    if(!status)status=set_int(display,"heightPixels",ALTSCREEN_DISPLAY_HEIGHT);
    if(!status)status=set_int(display,"widthPhysical",ALTSCREEN_DISPLAY_PHYSICAL_WIDTH);
    if(!status)status=set_int(display,"heightPhysical",ALTSCREEN_DISPLAY_PHYSICAL_HEIGHT);
    if(!status)status=set_int(display,"features",10);
    if(!status)status=set_int(display,"primaryInputDevice",3);
    if(!status)status=set_int(display,"initialViewArea",cpmap_card_view_for(position,0));
    if(!status)status=set_cstring(display,"initialURL",cpmap_card_url_for(position));
    for(i=0;!status&&i<CPMAP_CARD_VIEW_COUNT;i++){
        struct cpmap_card_rect r=cpmap_card_rect_for_view(i);
        void *view=create_area(0,0,ALTSCREEN_DISPLAY_WIDTH,ALTSCREEN_DISPLAY_HEIGHT),*safe=create_area(r.x,r.y,r.width,r.height),*adjacent=create_array();
        if(!view||!safe||!adjacent)status=-1;
        for(j=0;!status&&j<CPMAP_CARD_VIEW_COUNT;j++)if(j!=i)status=append_int(adjacent,j);
        if(!status)status=set_object(view,"safeArea",safe);
        if(!status&&CPMAP_CARD_VIEW_COUNT>1)status=set_object(view,"adjacentViewAreas",adjacent);
        if(!status)append(views,view);
        release_cf(adjacent);release_cf(safe);release_cf(view);
    }
    if(!status)status=set_object(display,"viewAreas",views);
    release_cf(views);
    if(status){release_cf(display);return NULL;}return display;
#else
    void *display = create_dictionary();
    void *view = NULL;
    void *safe = NULL;
#if ALTSCREEN_SECOND_VIEW_ENABLE
    void *second_view = NULL;
    void *second_safe = NULL;
    void *view_adjacent = NULL;
    void *second_adjacent = NULL;
#endif
    void *view_areas = NULL;
    CFArrayAppendValueFn append =
        (CFArrayAppendValueFn)resolve("CFArrayAppendValue");
    OSStatus status = display ? 0 : -1;

    if (!status) status = set_cstring(display, "uuid", ALT_UUID);
    if (!status) status = set_int(display, "type", 111);
    if (!status) status = set_int(display, "maxFPS",
                                  ALTSCREEN_DISPLAY_MAX_FPS);
    if (!status) status = set_int(display, "widthPixels",
                                  ALTSCREEN_DISPLAY_WIDTH);
    if (!status) status = set_int(display, "heightPixels",
                                  ALTSCREEN_DISPLAY_HEIGHT);
    if (!status) status = set_int(display, "widthPhysical",
                                  ALTSCREEN_DISPLAY_PHYSICAL_WIDTH);
    if (!status) status = set_int(display, "heightPhysical",
                                  ALTSCREEN_DISPLAY_PHYSICAL_HEIGHT);
    if (!status) status = set_int(display, "features", 10);
    if (!status) status = set_int(display, "primaryInputDevice", 3);
    if (!status) status = set_int(display, "initialViewArea",
#if ALTSCREEN_SECOND_VIEW_ENABLE
                                  ALTSCREEN_INITIAL_VIEW_AREA);
#else
                                  0);
#endif
    if (!status) status = set_cstring(display, "initialURL", ALT_MAP_URL);

    if (!status) view = create_area(ALTSCREEN_VIEW_ORIGIN_X,
                                    ALTSCREEN_VIEW_ORIGIN_Y,
                                    ALTSCREEN_VIEW_WIDTH,
                                    ALTSCREEN_VIEW_HEIGHT);
    if (!status) safe = create_area(ALTSCREEN_SAFE_ORIGIN_X,
                                    ALTSCREEN_SAFE_ORIGIN_Y,
                                    ALTSCREEN_SAFE_WIDTH,
                                    ALTSCREEN_SAFE_HEIGHT);
    if (!status && (!view || !safe)) status = -1;
    if (!status) status = set_object(view, "safeArea", safe);
#if ALTSCREEN_SECOND_VIEW_ENABLE
    if (!status) view_adjacent = create_array();
    if (!status && !view_adjacent) status = -1;
    if (!status) status = append_int(view_adjacent, 1);
    if (!status) status = set_object(view, "adjacentViewAreas",
                                     view_adjacent);
#endif
    if (!status) view_areas = create_array();
    if (!status && (!view_areas || !append)) status = -1;
    if (!status) append(view_areas, view);
#if ALTSCREEN_SECOND_VIEW_ENABLE
    if (!status) second_view = create_area(ALTSCREEN_SECOND_VIEW_ORIGIN_X,
                                           ALTSCREEN_SECOND_VIEW_ORIGIN_Y,
                                           ALTSCREEN_SECOND_VIEW_WIDTH,
                                           ALTSCREEN_SECOND_VIEW_HEIGHT);
    if (!status) second_safe = create_area(ALTSCREEN_SECOND_SAFE_ORIGIN_X,
                                           ALTSCREEN_SECOND_SAFE_ORIGIN_Y,
                                           ALTSCREEN_SECOND_SAFE_WIDTH,
                                           ALTSCREEN_SECOND_SAFE_HEIGHT);
    if (!status && (!second_view || !second_safe)) status = -1;
    if (!status) status = set_object(second_view, "safeArea", second_safe);
    if (!status) second_adjacent = create_array();
    if (!status && !second_adjacent) status = -1;
    if (!status) status = append_int(second_adjacent, 0);
    if (!status) status = set_object(second_view, "adjacentViewAreas",
                                     second_adjacent);
    if (!status) append(view_areas, second_view);
#endif
    if (!status) status = set_object(display, "viewAreas", view_areas);

    release_cf(safe);
    release_cf(view);
#if ALTSCREEN_SECOND_VIEW_ENABLE
    release_cf(second_adjacent);
    release_cf(view_adjacent);
    release_cf(second_safe);
    release_cf(second_view);
#endif
    release_cf(view_areas);
    if (status) {
        log_status("alt-display-build-failed", display, status);
        release_cf(display);
        return NULL;
    }
    return display;
#endif
}

static OSStatus append_alt_display(void *server_info)
{
    CFArrayCreateMutableCopyFn copy_array =
        (CFArrayCreateMutableCopyFn)resolve("CFArrayCreateMutableCopy");
    CFArrayGetCountFn get_count =
        (CFArrayGetCountFn)resolve("CFArrayGetCount");
    CFArrayAppendValueFn append =
        (CFArrayAppendValueFn)resolve("CFArrayAppendValue");
    const void *old_displays;
    void *new_displays = NULL;
    void *alt_display = NULL;
    long count;
    OSStatus status = -1;

    old_displays = get_object(server_info, "displays");
    if (!old_displays || !copy_array || !get_count || !append) {
        log_status("alt-display-missing-array-api", server_info, -1);
        return -1;
    }
    count = get_count(old_displays);
    if (count >= 2) {
        log_status("alt-display-already-present", old_displays, 0);
        return 0;
    }
    alt_display = create_alt_display();
    new_displays = copy_array(NULL, 0, old_displays);
    if (alt_display && new_displays) {
        append(new_displays, alt_display);
        status = set_object(server_info, "displays", new_displays);
    }
    release_cf(alt_display);
    release_cf(new_displays);
    log_status("alt-display-append", server_info, status);
    return status;
}

#ifdef ALTSCREEN_ALT_KNOB_HID
/*
 * The MHI2 R14 dio_manager already has a knob implementation, but Volkswagen's
 * START_CARPLAY parameters do not enable it and the stock /info therefore has
 * no knob associated with the type-111 display. Register a second, virtual
 * knob against ALT_UUID before AirPlayCopyServerInfo snapshots the HID list.
 *
 * The command-file worker is intentionally a proof harness, not the final
 * steering-wheel integration. Writing +1 or -1 to the RAM-disk file posts one
 * relative wheel report. Once both directions are verified on the phone, DSI
 * key 38/39 can feed the same post_alt_knob_report() function.
 */
typedef OSStatus (*HIDDeviceCreateVirtualFn)(void **, const void *);
typedef OSStatus (*HIDRegisterDeviceFn)(void *);
typedef OSStatus (*HIDDevicePostReportFn)(void *, const void *, size_t);

static pthread_mutex_t g_alt_knob_lock = PTHREAD_MUTEX_INITIALIZER;
static void *g_alt_knob_device;
static int g_alt_knob_worker_started;

/*
 * R14G17 Platform/HIDKnob.c descriptor. The MHI2-private
 * HIDKnobCreateDescriptor symbol takes an additional platform config pointer,
 * so calling it through the public two-argument prototype returns
 * kBadReferenceErr. Keeping the standard descriptor here avoids that private
 * ABI and still uses MHI2's native HIDDeviceCreateVirtual/Register/Post path.
 */
static const uint8_t k_alt_knob_descriptor[] = {
    0x05, 0x01, 0x09, 0x08, 0xA1, 0x01,
    0x05, 0x09, 0x09, 0x01, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x01, 0x81, 0x02,
    0x05, 0x0C, 0x0A, 0x23, 0x02, 0x0A, 0x24, 0x02,
    0x95, 0x02, 0x81, 0x02, 0x95, 0x05, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x01, 0xA1, 0x00,
    0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7F,
    0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0xC0,
    0x09, 0x38, 0x15, 0x81, 0x25, 0x7F,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x06,
    0xC0
};

static const char *alt_knob_command_path(void)
{
    const char *value = getenv("CARPLAY_ALTSCREEN_KNOB_COMMAND");
    return value && value[0] ? value :
        "/mnt/app/carplaymaps/alt-knob.command";
}

static OSStatus post_alt_knob_report(int ticks)
{
    HIDDevicePostReportFn post =
        (HIDDevicePostReportFn)resolve("HIDDevicePostReport");
    uint8_t report[4] = { 0, 0, 0, 0 };
    OSStatus status;

    if (!post || !g_alt_knob_device || ticks == 0 ||
        ticks < -127 || ticks > 127) return -1;

    /*
     * Do not call MHI2's exported HIDKnobFillReport here. Unlike the public
     * R14 seven-argument API, MU0841 adds an eighth KnobConfig pointer and
     * dereferences it unconditionally. Calling it through the public ABI
     * crashes dio_manager. For a wheel-only report the format is invariant:
     * buttons, X, Y, signed relative wheel.
     */
    report[3] = (uint8_t)(int8_t)ticks;
    log_status("alt-knob-post-begin", g_alt_knob_device, ticks);
    status = post(g_alt_knob_device, report, sizeof(report));
    log_status(ticks > 0 ? "alt-knob-wheel-positive" :
               "alt-knob-wheel-negative", g_alt_knob_device, status);
    return status;
}

static void *alt_knob_command_worker(void *unused)
{
    (void)unused;
    for (;;) {
        FILE *file = fopen(alt_knob_command_path(), "r");
        if (file) {
            char command[32];
            if (fgets(command, sizeof(command), file)) {
                int ticks = (int)strtol(command, NULL, 10);
                fclose(file);
                unlink(alt_knob_command_path());
                post_alt_knob_report(ticks);
            } else {
                fclose(file);
            }
        }
        {
            struct timeval delay;
            delay.tv_sec = 0;
            delay.tv_usec = 50000;
            select(0, NULL, NULL, NULL, &delay);
        }
    }
    return NULL;
}

static OSStatus ensure_alt_knob(void)
{
    HIDDeviceCreateVirtualFn create_device;
    HIDRegisterDeviceFn register_device;
    CFDataCreateFn create_data;
    void *descriptor_data = NULL;
    void *properties = NULL;
    void *device = NULL;
    OSStatus status = -1;

    pthread_mutex_lock(&g_alt_knob_lock);
    if (g_alt_knob_device) {
        pthread_mutex_unlock(&g_alt_knob_lock);
        return 0;
    }

    create_device = (HIDDeviceCreateVirtualFn)resolve(
        "HIDDeviceCreateVirtual");
    register_device = (HIDRegisterDeviceFn)resolve("HIDRegisterDevice");
    create_data = (CFDataCreateFn)resolve("CFDataCreate");
    if (!create_device || !register_device || !create_data) {
        log_status("alt-knob-api-missing", NULL, -1);
        goto exit;
    }

    descriptor_data = create_data(NULL, k_alt_knob_descriptor,
                                  (long)sizeof(k_alt_knob_descriptor));
    log_status("alt-knob-descriptor-data", descriptor_data,
               descriptor_data ? 0 : -1);
    properties = create_dictionary();
    if (!descriptor_data || !properties) {
        status = -1;
        goto exit;
    }

    status = set_cstring(properties, "name", "Eso_AltScreen_Knob");
    if (!status) status = set_cstring(properties, "displayUUID", ALT_UUID);
    if (!status) status = set_int(properties, "vendorID", 0);
    if (!status) status = set_int(properties, "productID", 0x201A);
    if (!status) status = set_int(properties, "countryCode", 9);
    if (!status)
        status = set_object(properties, "hidDescriptor", descriptor_data);
    if (!status) {
        status = create_device(&device, properties);
        log_status("alt-knob-device-create", device, status);
    }
    if (!status) {
        status = register_device(device);
        log_status("alt-knob-device-register", device, status);
    }
    if (!status) {
        pthread_t thread;
        g_alt_knob_device = device;
        device = NULL;
        if (!g_alt_knob_worker_started &&
            pthread_create(&thread, NULL, alt_knob_command_worker, NULL) == 0) {
            pthread_detach(thread);
            g_alt_knob_worker_started = 1;
        }
        log_status("alt-knob-registered", g_alt_knob_device, 0);
    }

exit:
    if (status) log_status("alt-knob-register-failed", device, status);
    release_cf(descriptor_data);
    release_cf(properties);
    release_cf(device);
    pthread_mutex_unlock(&g_alt_knob_lock);
    return status;
}
#endif

static OSStatus set_enabled_features(void *response)
{
    void *features = create_array();
    OSStatus status = features ? 0 : -1;
    if (!status) status = append_string(features, "iAPChannel");
    if (!status) status = append_string(features, "viewAreas");
    if (!status) status = append_string(features, "altScreen");
    if (!status) status = set_object(response, "enabledFeatures", features);
    release_cf(features);
    log_status("enabled-features-set", response, status);
    return status;
}

static OSStatus add_carplay_control_feature(void *server_info)
{
    OSStatus status = -1;
    int64_t features = get_int(server_info, "features", &status);
    int64_t updated;
    if (status) {
        log_status("carplay-control-feature-read-failed", server_info, status);
        return status;
    }
    updated = features | AIRPLAY_FEATURE_CARPLAY_CONTROL;
#ifdef ALTSCREEN_ADVERTISE_HK_PAIRING_AND_ENCRYPT
    updated |= AIRPLAY_FEATURE_HK_PAIRING_AND_ENCRYPT;
#endif
    status = set_int(server_info, "features", updated);
#ifdef ALTSCREEN_ADVERTISE_HK_PAIRING_AND_ENCRYPT
    log_status("carplay-control-hk-pairing-features-set", server_info, status);
#else
    log_status("carplay-control-feature-set", server_info, status);
#endif
    return status;
}

#ifdef ALTSCREEN_ACTIVATE_DISPLAY
static OSStatus send_alt_command(void *session, const char *type,
                                 const char *url);
#endif

#ifdef ALTSCREEN_PASSIVE_CAPTURE

#define SCREEN_HEADER_SIZE 128U
#define SCREEN_MAX_BODY (8U * 1024U * 1024U)
#define SCREEN_OPCODE_VIDEO_FRAME 0U
#define SCREEN_OPCODE_VIDEO_CONFIG 1U
#define ALTSCREEN_ACCEPT_TIMEOUT_SECONDS 180
#define ALTSCREEN_READY_PATH "/ramdisk/carplay-altstream.ready"
#define ALTSCREEN_BRIDGE_READY_PATH "/ramdisk/carplay-bridge.ready"

typedef struct PassiveListener {
    int server_fd;
    uint16_t port;
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
    void *session;
#endif
#ifdef ALTSCREEN_AES_CTR_DECRYPT
    uint8_t aes_key[16];
    uint8_t aes_iv[16];
    int aes_valid;
#endif
} PassiveListener;

#ifdef ALTSCREEN_AES_CTR_DECRYPT
/* AES_CTR_Context is private to libairplay. 512 bytes is deliberately larger
 * than the Gladman AES context used by this ARM build and remains naturally
 * aligned through the union. The API only receives an opaque pointer. */
typedef union AltAESCTRStorage {
    uint64_t align64;
    void *align_ptr;
    uint8_t bytes[512];
} AltAESCTRStorage;

static int inspect_h264_avcc_frame(const uint8_t *frame, size_t size,
                                   int *out_first_type,
                                   int *out_has_idr)
{
    size_t offset = 0;
    int count = 0;
    int first_type = -1;
    int has_idr = 0;
    while (offset + 4 <= size) {
        uint32_t nal_size = ((uint32_t)frame[offset] << 24) |
                            ((uint32_t)frame[offset + 1] << 16) |
                            ((uint32_t)frame[offset + 2] << 8) |
                            (uint32_t)frame[offset + 3];
        offset += 4;
        if (nal_size == 0 || nal_size > size - offset) return 0;
        if (first_type < 0) first_type = frame[offset] & 0x1f;
        if ((frame[offset] & 0x1f) == 5) has_idr = 1;
        offset += nal_size;
        ++count;
    }
    if (out_first_type) *out_first_type = first_type;
    if (out_has_idr) *out_has_idr = has_idr;
    return offset == size ? count : 0;
}
#endif

#ifdef ALTSCREEN_FORWARD_SHM_RING
static AltFrameRing *create_forward_ring(void)
{
    AltFrameRing *ring;
    uint32_t generation = 1;
    unsigned int i;
    int fd = open(ALT_FRAME_RING_PATH, O_RDWR | O_CREAT, 0666);
    if (fd < 0) return NULL;
    if (ftruncate(fd, (off_t)sizeof(*ring)) != 0) {
        close(fd);
        return NULL;
    }
    ring = (AltFrameRing *)mmap(NULL, sizeof(*ring), PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    close(fd);
    if (ring == MAP_FAILED) return NULL;
    if (ring->magic == ALT_FRAME_RING_MAGIC &&
        ring->version == ALT_FRAME_RING_VERSION)
        generation = ring->generation + 1U;
    ring->magic = 0;
    __sync_synchronize();
    ring->version = ALT_FRAME_RING_VERSION;
    ring->generation = generation ? generation : 1U;
    ring->write_sequence = 0;
    for (i = 0; i < ALT_FRAME_RING_SLOT_COUNT; ++i)
        ring->slots[i].committed_sequence = 0;
    __sync_synchronize();
    ring->magic = ALT_FRAME_RING_MAGIC;
    return ring;
}

static int forward_ring_message(AltFrameRing *ring, uint16_t type,
                                uint32_t source_sequence, uint32_t flags,
                                uint64_t media_timestamp_ns,
                                const uint8_t *data, size_t size)
{
    AltFrameRingSlot *slot;
    uint32_t sequence;
    if (!ring || !data || size == 0 || size > ALT_FRAME_RING_SLOT_DATA_MAX)
        return -1;
    sequence = ring->write_sequence + 1U;
    if (sequence == 0) sequence = 1U;
    slot = &ring->slots[(sequence - 1U) % ALT_FRAME_RING_SLOT_COUNT];
    slot->committed_sequence = 0;
    __sync_synchronize();
    slot->type = type;
    slot->size = (uint32_t)size;
    slot->flags = flags;
    slot->source_sequence = source_sequence;
    slot->media_timestamp_lo = (uint32_t)media_timestamp_ns;
    slot->media_timestamp_hi = (uint32_t)(media_timestamp_ns >> 32);
    memcpy(slot->data, data, size);
    __sync_synchronize();
    slot->committed_sequence = sequence;
    __sync_synchronize();
    ring->write_sequence = sequence;
    return 1;
}
#endif

static const char *capture_directory(void)
{
    const char *value = getenv("CARPLAY_ALTSCREEN_CAPTURE_DIR");
    return value && value[0] ? value :
        "/ramdisk/carplay_altscreen_handshake";
}

static void log_capture(const char *event, int value1, int value2,
                        unsigned long long value3)
{
    FILE *file = cpmap_log_open(log_path(), CPMAP_VIDEO_LOG_LIMIT);
    if (!file) return;
    fprintf(file,
            "t_us=%llu pid=%ld event=%s value1=%d value2=%d value3=%llu\n",
            (unsigned long long)monotonic_us(), (long)getpid(),
            event ? event : "?", value1, value2, value3);
    cpmap_log_close(file, CPMAP_VIDEO_LOG_LIMIT);
}

static int create_marker(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}

static int consume_marker(const char *path)
{
    if (access(path, F_OK) != 0) return 0;
    return unlink(path) == 0 ? 1 : 0;
}

static void write_capture_file(const char *name, const void *data, size_t size)
{
    char path[512];
    FILE *file;
    snprintf(path, sizeof(path), "%s/%s", capture_directory(), name);
    file = fopen(path, "wb");
    if (!file) {
        log_capture("capture-file-open-failed", errno, 0, 0);
        return;
    }
    if (size > 0) fwrite(data, 1, size, file);
    fclose(file);
}

static int wait_readable(int fd, int timeout_seconds)
{
    fd_set read_set;
    struct timeval timeout;
    int result;
    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    do {
        result = select(fd + 1, &read_set, NULL, NULL, &timeout);
    } while (result < 0 && errno == EINTR);
    return result;
}

static int read_exact(int fd, uint8_t *buffer, size_t size)
{
    size_t offset = 0;
    while (offset < size) {
        ssize_t count;
        if (wait_readable(fd, 10) <= 0) return -1;
        count = recv(fd, buffer + offset, size - offset, 0);
        if (count <= 0) return -1;
        offset += (size_t)count;
    }
    return 0;
}

static uint32_t read_u32_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void *passive_listener_thread(void *context)
{
    PassiveListener *listener = (PassiveListener *)context;
    int client_fd = -1;
    int one = 1;
    uint8_t header[SCREEN_HEADER_SIZE];
    uint8_t *body = NULL;
    size_t body_capacity = 0;
    unsigned long long frames = 0;
    unsigned long long messages = 0;
    int saved_first_header = 0;
    int have_video_config = 0;
    int stream_ready_signaled = 0;
#ifdef ALTSCREEN_FORWARD_SHM_RING
    AltFrameRing *forward_ring = NULL;
    uint8_t *cached_config = NULL;
    size_t cached_config_size = 0;
    unsigned long long forwarded_frames = 0;
    int cache_leased = 0;
#endif
#ifdef ALTSCREEN_AES_CTR_DECRYPT
    AltAESCTRStorage aes_storage;
    AESCTRInitFn aes_init = (AESCTRInitFn)resolve("AES_CTR_Init");
    AESCTRUpdateFn aes_update = (AESCTRUpdateFn)resolve("AES_CTR_Update");
    AESCTRFinalFn aes_final = (AESCTRFinalFn)resolve("AES_CTR_Final");
    int aes_ready = 0;

    memset(&aes_storage, 0, sizeof(aes_storage));
    if (listener->aes_valid && aes_init && aes_update && aes_final &&
        aes_init(aes_storage.bytes, listener->aes_key, listener->aes_iv) == 0) {
        aes_ready = 1;
        log_capture("alt-aes-ctr-ready", listener->port, 16, 0);
    } else {
        log_capture("alt-aes-ctr-unavailable", listener->aes_valid,
                    aes_init && aes_update && aes_final, 0);
    }
#endif

    unlink(ALTSCREEN_READY_PATH);
    unlink(ALTSCREEN_BRIDGE_READY_PATH);
    log_capture("alt-listener-waiting", listener->port, listener->server_fd, 0);
    if (wait_readable(listener->server_fd, ALTSCREEN_ACCEPT_TIMEOUT_SECONDS) > 0)
        client_fd = accept(listener->server_fd, NULL, NULL);
    close(listener->server_fd);
    listener->server_fd = -1;
    if (client_fd < 0) {
        log_capture("alt-listener-accept-timeout", listener->port, errno, 0);
#ifdef ALTSCREEN_AES_CTR_DECRYPT
        if (aes_ready && aes_final) aes_final(aes_storage.bytes);
#endif
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
        release_cf(listener->session);
#endif
        free(listener);
        return NULL;
    }
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    log_capture("alt-video-connected", listener->port, client_fd, 0);
#ifdef ALTSCREEN_FORWARD_SHM_RING
    /* Do not reserve 16 MiB merely for a pending/failed connection. */
    cache_leased = cpmap_cache_acquire() == 0;
    if (cache_leased) forward_ring = create_forward_ring();
    log_capture("alt-forward-ring-ready", forward_ring != NULL,
                (int)ALT_FRAME_RING_SLOT_COUNT, sizeof(AltFrameRing));
#endif

    for (;;) {
        uint32_t body_size;
        uint8_t opcode;
        uint64_t media_timestamp_ns = 0;
        if (read_exact(client_fd, header, sizeof(header)) != 0) break;
        body_size = read_u32_le(header);
        opcode = header[4];
        memcpy(&media_timestamp_ns, header + 8, sizeof(media_timestamp_ns));
        if (body_size > SCREEN_MAX_BODY) {
            log_capture("alt-invalid-body-size", (int)body_size, opcode, messages);
            break;
        }
        if (body_size > body_capacity) {
            uint8_t *grown = (uint8_t *)realloc(body, body_size ? body_size : 1);
            if (!grown) break;
            body = grown;
            body_capacity = body_size;
        }
        if (body_size && read_exact(client_fd, body, body_size) != 0) break;
        ++messages;
        if (!saved_first_header) {
            write_capture_file("alt-first-header.bin", header, sizeof(header));
            saved_first_header = 1;
        }
        if (opcode == SCREEN_OPCODE_VIDEO_CONFIG) {
            float width = 0.0f;
            float height = 0.0f;
            memcpy(&width, header + 16, sizeof(width));
            memcpy(&height, header + 20, sizeof(height));
            write_capture_file("alt-video-config.bin", body, body_size);
            log_capture("alt-video-config", (int)width, (int)height, body_size);
            if ((int)width == ALTSCREEN_DISPLAY_WIDTH &&
                (int)height == ALTSCREEN_DISPLAY_HEIGHT && body_size > 0)
                have_video_config = 1;
#ifdef ALTSCREEN_FORWARD_SHM_RING
            free(cached_config);
            cached_config = NULL;
            cached_config_size = 0;
            if (body_size > 0) {
                cached_config = (uint8_t *)malloc(body_size);
                if (cached_config) {
                    memcpy(cached_config, body, body_size);
                    cached_config_size = body_size;
                }
            }
            if (forward_ring && body_size > 0) {
                int count = forward_ring_message(
                    forward_ring, ALT_FRAME_TYPE_CONFIG, 0, 0,
                    media_timestamp_ns, body, body_size);
                log_capture("alt-forward-config", count, (int)body_size, 0);
            }
#endif
        } else if (opcode == SCREEN_OPCODE_VIDEO_FRAME) {
            ++frames;
#ifdef ALTSCREEN_AES_CTR_DECRYPT
            if (aes_ready && body_size > 0) {
                int first_nal_type = -1;
                int has_idr = 0;
                int nal_count;
                OSStatus decrypt_status = aes_update(aes_storage.bytes, body,
                                                      body_size, body);
                if (decrypt_status != 0) {
                    log_capture("alt-aes-decrypt-failed", decrypt_status,
                                (int)body_size, frames);
                    aes_ready = 0;
                } else {
                    nal_count = inspect_h264_avcc_frame(body, body_size,
                                                       &first_nal_type,
                                                       &has_idr);
                    if (nal_count > 0 && has_idr) {
                        log_capture("alt-idr-frame", (int)body_size,
                                    nal_count, frames);
                    }
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
                    if (nal_count > 0 && have_video_config &&
                        !stream_ready_signaled) {
                        int marker_status = create_marker(ALTSCREEN_READY_PATH);
                        log_capture("alt-stream-ready", marker_status,
                                    (int)body_size, frames);
                        if (marker_status == 0) stream_ready_signaled = 1;
                    }
                    if (nal_count > 0 && listener->session &&
                        consume_marker(ALTSCREEN_BRIDGE_READY_PATH)) {
                        OSStatus force_status = send_alt_command(
                            listener->session, "forceKeyFrame", NULL);
                        log_capture("alt-bridge-ready-force-keyframe",
                                    force_status, 0, frames);
                    }
                    if (nal_count > 0 && listener->session &&
                        consume_marker(ALT_FRAME_FORCE_IDR_PATH)) {
                        OSStatus force_status = send_alt_command(
                            listener->session, "forceKeyFrame", NULL);
                        log_capture("alt-latency-force-keyframe",
                                    force_status, 0, frames);
                    }
#endif
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
                    if (nal_count > 0 && listener->session &&
                        (frames % ALTSCREEN_FORCE_KEYFRAME_INTERVAL) == 0) {
                        OSStatus force_status = send_alt_command(
                            listener->session, "forceKeyFrame", NULL);
                        log_capture("alt-periodic-force-keyframe",
                                    force_status,
                                    (int)ALTSCREEN_FORCE_KEYFRAME_INTERVAL,
                                    frames);
                    }
#endif
                    if (frames == 1) {
                        write_capture_file("alt-first-frame-decrypted.bin",
                                           body, body_size);
                    }
                    if (frames == 1 || (frames % 30ULL) == 0) {
                        log_capture(nal_count > 0 ?
                                    "alt-aes-decrypt-valid-avcc" :
                                    "alt-aes-decrypt-invalid-avcc",
                                    (int)body_size, first_nal_type, frames);
                    }
#ifdef ALTSCREEN_FORWARD_SHM_RING
                    if (nal_count > 0 && forward_ring) {
                        int count;
                        if ((has_idr || (frames % 30ULL) == 1) &&
                            cached_config && cached_config_size > 0) {
                            forward_ring_message(
                                forward_ring, ALT_FRAME_TYPE_CONFIG,
                                (uint32_t)frames, 0, media_timestamp_ns,
                                cached_config, cached_config_size);
                        }
                        count = forward_ring_message(
                            forward_ring, ALT_FRAME_TYPE_VIDEO,
                            (uint32_t)frames,
                            has_idr ? ALT_FRAME_FLAG_KEYFRAME : 0,
                            media_timestamp_ns, body, body_size);
                        if (count > 0) ++forwarded_frames;
                        if (frames == 1 || (frames % 30ULL) == 0) {
                            log_capture(count > 0 ? "alt-forward-frame" :
                                        "alt-forward-drop", count,
                                        (int)body_size, frames);
                        }
                    }
#endif
                }
            }
#endif
            if (frames == 1) {
                size_t sample = body_size < 256 ? body_size : 256;
                write_capture_file("alt-first-frame-prefix.bin", body, sample);
                write_capture_file("alt-first-frame-header.bin", header, sizeof(header));
            }
            if (frames == 1 || (frames % 30ULL) == 0)
                log_capture("alt-video-frame", (int)body_size, opcode, frames);
        } else {
            log_capture("alt-screen-message", (int)body_size, opcode, messages);
        }
    }
    log_capture("alt-video-disconnected", listener->port, errno, frames);
    unlink(ALTSCREEN_READY_PATH);
    unlink(ALTSCREEN_BRIDGE_READY_PATH);
#ifdef ALTSCREEN_AES_CTR_DECRYPT
    if (aes_ready && aes_final) aes_final(aes_storage.bytes);
#endif
#ifdef ALTSCREEN_FORWARD_SHM_RING
    log_capture("alt-forward-complete", (int)forwarded_frames,
                forward_ring != NULL, frames);
    if (forward_ring) munmap(forward_ring, sizeof(*forward_ring));
    free(cached_config);
    if (cache_leased) cpmap_cache_release();
#endif
    if (client_fd >= 0) close(client_fd);
    free(body);
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
    release_cf(listener->session);
#endif
    free(listener);
    return NULL;
}

static OSStatus start_passive_listener(uint16_t *out_port
#ifdef ALTSCREEN_AES_CTR_DECRYPT
                                       , const uint8_t aes_key[16],
                                       const uint8_t aes_iv[16], int aes_valid
#endif
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
                                       , void *session
#endif
                                       )
{
    PassiveListener *listener;
#ifdef ALTSCREEN_LISTEN_IPV6
    struct sockaddr_in6 address;
#else
    struct sockaddr_in address;
#endif
    socklen_t address_size;
    pthread_t thread;
    int fd;
    int one = 1;
    int status;

#ifdef ALTSCREEN_LISTEN_IPV6
    /* Wired CarPlay on MHI2 is routed over the carplay0 IPv6 link-local
     * interface. The stock main screen data connection is tcp6 as well, so
     * the alternate screen receiver must expose an IPv6 port. */
    fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(one));
    memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
#ifdef ALTSCREEN_FIXED_PORT
    address.sin6_port = htons(ALTSCREEN_FIXED_PORT);
#else
    address.sin6_port = 0;
#endif
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(fd, 1) != 0) {
        close(fd);
        return -1;
    }
    address_size = sizeof(address);
    if (getsockname(fd, (struct sockaddr *)&address, &address_size) != 0) {
        close(fd);
        return -1;
    }
#else
    /* The iPhone reaches the MHI2 control/data ports over IPv4. QNX 6.5 does
     * not reliably provide an IPv4-mapped dual-stack AF_INET6 listener even
     * when IPV6_V6ONLY is cleared, so bind an explicit IPv4 wildcard socket. */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef ALTSCREEN_FIXED_PORT
    address.sin_port = htons(ALTSCREEN_FIXED_PORT);
#else
    address.sin_port = 0;
#endif
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(fd, 1) != 0) {
        close(fd);
        return -1;
    }
    address_size = sizeof(address);
    if (getsockname(fd, (struct sockaddr *)&address, &address_size) != 0) {
        close(fd);
        return -1;
    }
#endif
    listener = (PassiveListener *)calloc(1, sizeof(*listener));
    if (!listener) {
        close(fd);
        return -1;
    }
    listener->server_fd = fd;
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
    listener->session = (void *)retain_cf(session);
    if (!listener->session) {
        close(fd);
        free(listener);
        return -1;
    }
#endif
#ifdef ALTSCREEN_AES_CTR_DECRYPT
    if (aes_valid) {
        memcpy(listener->aes_key, aes_key, sizeof(listener->aes_key));
        memcpy(listener->aes_iv, aes_iv, sizeof(listener->aes_iv));
        listener->aes_valid = 1;
    }
#endif
#ifdef ALTSCREEN_LISTEN_IPV6
    listener->port = ntohs(address.sin6_port);
#else
    listener->port = ntohs(address.sin_port);
#endif
    status = pthread_create(&thread, NULL, passive_listener_thread, listener);
    if (status != 0) {
        close(fd);
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
        release_cf(listener->session);
#endif
        free(listener);
        return -1;
    }
    pthread_detach(thread);
    *out_port = listener->port;
    return 0;
}

static int first_stream_type(const void *request)
{
    const void *streams = get_object(request, "streams");
    CFArrayGetCountFn get_count =
        (CFArrayGetCountFn)resolve("CFArrayGetCount");
    CFArrayGetValueAtIndexFn get_value =
        (CFArrayGetValueAtIndexFn)resolve("CFArrayGetValueAtIndex");
    const void *stream;
    OSStatus status = -1;
    int64_t type;
    if (!streams || !get_count || !get_value || get_count(streams) < 1)
        return -1;
    stream = get_value(streams, 0);
    if (!stream) return -1;
    type = get_int(stream, "type", &status);
    return status == 0 ? (int)type : -1;
}

static int64_t first_stream_int64(const void *request, const char *key,
                                  OSStatus *out_status)
{
    const void *streams = get_object(request, "streams");
    CFArrayGetCountFn get_count =
        (CFArrayGetCountFn)resolve("CFArrayGetCount");
    CFArrayGetValueAtIndexFn get_value =
        (CFArrayGetValueAtIndexFn)resolve("CFArrayGetValueAtIndex");
    const void *stream;
    OSStatus status = -1;
    int64_t value = 0;
    if (streams && get_count && get_value && get_count(streams) >= 1) {
        stream = get_value(streams, 0);
        if (stream) value = get_int(stream, key, &status);
    }
    if (out_status) *out_status = status;
    return value;
}

static void *create_alt_stream_response(uint16_t port)
{
    void *response = create_dictionary();
    void *streams = create_array();
    void *stream = create_dictionary();
    CFArrayAppendValueFn append =
        (CFArrayAppendValueFn)resolve("CFArrayAppendValue");
    OSStatus status = response && streams && stream && append ? 0 : -1;
    if (!status) status = set_int(stream, "type", 111);
    if (!status) status = set_int(stream, "dataPort", port);
    if (!status) append(streams, stream);
    if (!status) status = set_object(response, "streams", streams);
    release_cf(stream);
    release_cf(streams);
    if (status) {
        release_cf(response);
        return NULL;
    }
    return response;
}

#ifdef ALTSCREEN_ACTIVATE_DISPLAY

#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
#include "card_transition_policy.h"
#endif
typedef struct AltCommandContext {
    const char *type;
    void *session;
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
    uint64_t wire_id,wire_epoch;
    int card_owned;
    struct card_command card;
#endif
} AltCommandContext;

#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
/* One completion-bearing command on this legacy event channel at a time.
 * A lost ACK stays busy until disconnect, rather than issuing uncorrelatable
 * follow-up commands. Stale callbacks cannot release a new session's gate. */
static pthread_mutex_t g_card_wire_lock=PTHREAD_MUTEX_INITIALIZER;
static void *g_card_wire_session;
static uint64_t g_card_wire_epoch,g_card_wire_serial,g_card_wire_active;
static void card_operation_completed(const struct card_command *,int);
static void card_signal_ready(void *session);
static void alt_command_completion(OSStatus,const void *,void *);
static void card_wire_reset(void *session){
    pthread_mutex_lock(&g_card_wire_lock);g_card_wire_session=session;
    ++g_card_wire_epoch;g_card_wire_active=0;pthread_mutex_unlock(&g_card_wire_lock);
}
static int card_wire_finish(const AltCommandContext *context){
    int current=0;pthread_mutex_lock(&g_card_wire_lock);
    if(context->wire_id&&context->wire_id==g_card_wire_active&&context->wire_epoch==g_card_wire_epoch&&context->session==g_card_wire_session){g_card_wire_active=0;current=1;}
    pthread_mutex_unlock(&g_card_wire_lock);return current;
}
static OSStatus card_gated_send(SessionSendCommandFn send,void *session,const void *request,AltCommandContext *context){
    OSStatus status=0;
    pthread_mutex_lock(&g_card_wire_lock);
    if(session!=g_card_wire_session)status=-ENOTCONN;
    else if(g_card_wire_active)status=-EBUSY;
    else{if(++g_card_wire_serial==0)++g_card_wire_serial;g_card_wire_active=g_card_wire_serial;
        context->wire_id=g_card_wire_serial;context->wire_epoch=g_card_wire_epoch;}
    pthread_mutex_unlock(&g_card_wire_lock);
    if(status)return status;
    /* The success callback may run synchronously and free context. */
    status=send(session,request,alt_command_completion,context);
    if(status)card_wire_finish(context);
    return status;
}
#endif

typedef struct AltForceTask {
    void *session;
} AltForceTask;

static OSStatus send_alt_command(void *session, const char *type,
                                 const char *url);
static void *force_keyframe_thread(void *context);

static void alt_command_completion(OSStatus status, const void *response,
                                   void *context)
{
    AltCommandContext *command = (AltCommandContext *)context;
    const char *type = command && command->type ? command->type : "unknown";
    char event[96];
    snprintf(event, sizeof(event), "alt-command-completion-%s", type);
    log_status(event, response, status);
    if (response) log_object("alt-command-completion-response", response);
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
    if(command){
        int current=card_wire_finish(command);
        if(command->card_owned&&current)card_operation_completed(&command->card,status);
    }
#endif

    /* libairplay 210.81 cannot reliably correlate two completion-bearing
     * commands queued together.  Chain forceKeyFrame only after the phone has
     * acknowledged showUI, and dispatch it from a separate thread so the next
     * command is not written recursively from the event-channel callback. */
    if (status == 0 && command && command->session &&
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
        !command->card_owned &&
#endif
        strcmp(type, "showUI") == 0) {
        AltForceTask *task = (AltForceTask *)calloc(1, sizeof(*task));
        pthread_t thread;
        int thread_status = -1;
        if (task) task->session = (void *)retain_cf(command->session);
        if (task && task->session)
            thread_status = pthread_create(&thread, NULL,
                                           force_keyframe_thread, task);
        if (thread_status == 0) {
            pthread_detach(thread);
            log_status("alt-force-after-showui-scheduled",
                       command->session, 0);
        } else {
            if (task) {
                release_cf(task->session);
                free(task);
            }
            log_status("alt-force-after-showui-schedule-failed",
                       command->session, thread_status);
        }
    }

    if (command) {
        release_cf(command->session);
        free(command);
    }
}

/*
 * An advertised alternate display is not activated by initialURL alone on
 * the tested iPhone. LIVI waits until the main screen receiver is ready, then
 * sends these commands. The caller below invokes this after the stock type-110
 * SETUP succeeds, when both the event channel and main receiver are ready.
 */
static OSStatus send_alt_command(void *session, const char *type,
                                 const char *url)
{
    SessionSendCommandFn send_command = (SessionSendCommandFn)resolve(
        "AirPlayReceiverSessionSendCommand");
    void *request = create_dictionary();
    void *params = create_dictionary();
    AltCommandContext *context = NULL;
    OSStatus status = send_command && request && params ? 0 : -1;

    if (!status) status = set_cstring(request, "type", type);
    if (!status) status = set_cstring(params, "uuid", ALT_UUID);
    if (!status && url) status = set_cstring(params, "url", url);
    if (!status) status = set_object(request, "params", params);
    if (!status) {
        context = (AltCommandContext *)calloc(1, sizeof(*context));
        if (!context) status = -1;
    }
    if (!status) {
        context->type = type;
        context->session = (void *)retain_cf(session);
        if (!context->session) status = -1;
    }
    if (!status) {
        log_object("alt-command-request", request);
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
        status=card_gated_send(send_command,session,request,context);
#else
        status = send_command(session, request, alt_command_completion,
                              context);
#endif
        if (!status) context = NULL; /* completion callback owns it */
    }
    log_status(type, session, status);
    if (context) {
        release_cf(context->session);
        free(context);
    }
    release_cf(params);
    release_cf(request);
    return status;
}

#ifdef ALTSCREEN_RUNTIME_MAP_CONTROL

/*
 * DSIKeyPanel runs in the Java HMI while AirPlayReceiverSessionSendCommand is
 * private to dio_manager.  A loopback-only UDP socket is the smallest safe
 * bridge between the two processes: it exposes no network service and keeps
 * the stock DSI destination untouched.
 *
 * The four command bodies match the LIVI proof validated on the same iPhone:
 *   changeMapZoomLevel: zoomDirection 0=in, 1=out
 *   requestUI: Amap -> Baidu -> Apple Maps -> Google Maps
 */
static pthread_mutex_t g_map_control_lock = PTHREAD_MUTEX_INITIALIZER;
static void *g_map_control_session;
static int g_map_control_worker_started;
static int g_map_cycle_index;
static const char *g_map_cycle_urls[] = {
    "iosamap://",
    "baidumap://map?src=ios.livi.carplay",
    "http://maps.apple.com/",
    "comgooglemaps://"
};

static OSStatus send_map_semantic_command(void *session, const char *type,
                                          const char *url,
                                          int zoom_direction)
{
    SessionSendCommandFn send_command = (SessionSendCommandFn)resolve(
        "AirPlayReceiverSessionSendCommand");
    void *request = create_dictionary();
    void *params = create_dictionary();
    AltCommandContext *context = NULL;
    OSStatus status = send_command && request && params ? 0 : -1;

    if (!status) status = set_cstring(request, "type", type);
    if (!status && strcmp(type, "changeMapZoomLevel") == 0) {
        status = set_cstring(params, "uuid", ALT_UUID);
        if (!status) status = set_int(params, "zoomDirection",
                                      zoom_direction);
    } else if (!status && strcmp(type, "requestUI") == 0) {
        status = url ? set_cstring(params, "url", url) : -1;
    }
    if (!status) status = set_object(request, "params", params);
    if (!status) {
        context = (AltCommandContext *)calloc(1, sizeof(*context));
        if (!context) status = -1;
    }
    if (!status) {
        context->type = type;
        context->session = (void *)retain_cf(session);
        if (!context->session) status = -1;
    }
    if (!status) {
        log_object("map-control-command-request", request);
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
        {
            int retry;
            /* Only this separate loopback-input thread may wait briefly.
             * Never block HMI, a video/encoder thread or a callback. */
            for(retry=0;retry<40;retry++){
                status=card_gated_send(send_command,session,request,context);
                if(status!=-EBUSY)break;
                usleep(10000);
            }
        }
#else
        status = send_command(session, request, alt_command_completion,
                              context);
#endif
        if (!status) context = NULL;
    }
    log_status(type, session, status);
    if (context) {
        release_cf(context->session);
        free(context);
    }
    release_cf(params);
    release_cf(request);
    return status;
}

static void dispatch_map_control_command(const char *command)
{
    void *session;
    const char *url = NULL;
    int map_delta = 0;

    if (strcmp(command, "map-next") == 0) map_delta = 1;
    else if (strcmp(command, "map-previous") == 0) map_delta = -1;

    pthread_mutex_lock(&g_map_control_lock);
    session = (void *)retain_cf(g_map_control_session);
    if (session && map_delta != 0) {
        int count = (int)(sizeof(g_map_cycle_urls) /
                          sizeof(g_map_cycle_urls[0]));
        g_map_cycle_index = (g_map_cycle_index + map_delta + count) % count;
        url = g_map_cycle_urls[g_map_cycle_index];
    }
    pthread_mutex_unlock(&g_map_control_lock);

    if (!session) {
        log_status("map-control-no-session", NULL, -1);
        return;
    }
    if (strcmp(command, "zoom-in") == 0) {
        send_map_semantic_command(session, "changeMapZoomLevel", NULL, 0);
    } else if (strcmp(command, "zoom-out") == 0) {
        send_map_semantic_command(session, "changeMapZoomLevel", NULL, 1);
    } else if (url) {
        send_map_semantic_command(session, "requestUI", url, -1);
    } else {
        log_status("map-control-unknown-command", session, -1);
    }
    release_cf(session);
}

static void *map_control_command_worker(void *unused)
{
    (void)unused;
    for (;;) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in address;
        int reuse = 1;

        if (fd < 0) {
            log_status("map-control-socket-failed", NULL, errno);
            usleep(1000U * 1000U);
            continue;
        }
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(ALTSCREEN_MAP_CONTROL_PORT);
        if (bind(fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
            log_status("map-control-bind-failed", NULL, errno);
            close(fd);
            usleep(1000U * 1000U);
            continue;
        }
        log_capture("map-control-loopback-ready",
                    ALTSCREEN_MAP_CONTROL_PORT, 0, 0);

        for (;;) {
            char command[32];
            ssize_t length = recv(fd, command, sizeof(command) - 1U, 0);
            if (length < 0) {
                if (errno == EINTR) continue;
                log_status("map-control-recv-failed", NULL, errno);
                break;
            }
            command[length] = '\0';
            while (length > 0 &&
                   (command[length - 1] == '\r' ||
                    command[length - 1] == '\n' ||
                    command[length - 1] == ' ' ||
                    command[length - 1] == '\t')) {
                command[--length] = '\0';
            }
            dispatch_map_control_command(command);
        }
        close(fd);
        usleep(250U * 1000U);
    }
    return NULL;
}

static void remember_map_control_session(void *session)
{
    void *retained = (void *)retain_cf(session);
    void *old;
    pthread_t thread;
    int thread_status = 0;

    pthread_mutex_lock(&g_map_control_lock);
    old = g_map_control_session;
    g_map_control_session = retained;
    g_map_cycle_index = 0;
    if (!g_map_control_worker_started) {
        thread_status = pthread_create(&thread, NULL,
                                       map_control_command_worker, NULL);
        if (thread_status == 0) {
            pthread_detach(thread);
            g_map_control_worker_started = 1;
        }
    }
    pthread_mutex_unlock(&g_map_control_lock);
    release_cf(old);
    log_status("map-control-session-remembered", retained,
               retained && thread_status == 0 ? 0 : -1);
}

static void forget_map_control_session(void *session)
{
    void *old = NULL;
    pthread_mutex_lock(&g_map_control_lock);
    if (g_map_control_session == session) {
        old = g_map_control_session;
        g_map_control_session = NULL;
        g_map_cycle_index = 0;
    }
    pthread_mutex_unlock(&g_map_control_lock);
    release_cf(old);
    if (old) log_status("map-control-session-forgotten", session, 0);
}

#endif /* ALTSCREEN_RUNTIME_MAP_CONTROL */

#ifdef ALTSCREEN_RUNTIME_VIEWAREA_CONTROL

/*
 * CarPlay Communication Plug-in R15 added dynamic view-area transitions.
 * MHI2's R14 library does not export the convenience wrapper, but it already
 * exports AirPlayReceiverSessionSendCommand and the CoreFoundation-lite
 * dictionary helpers used by that wrapper.  The command body below was
 * recovered from a production MHI3 libairplay implementation:
 *
 *   type = updateViewArea
 *   params.uuid = <screen UUID>
 *   params.animationDurationMillis = <milliseconds>
 *   params.viewAreaIndex = <declared viewAreas[] index>
 *   params.adjacentViewAreas = [<declared adjacent view-area index>]
 *
 * Keep this proof behind a compile-time switch.  Stable single-view builds
 * therefore remain protocol-identical and cannot start the command worker.
 */
static pthread_mutex_t g_viewarea_lock = PTHREAD_MUTEX_INITIALIZER;
static void *g_viewarea_session;
static int g_viewarea_worker_started;
static unsigned long g_viewarea_generation;
static int g_viewarea_last_sent;

#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
static struct card_transition_policy g_card_policy;
static int g_card_route_candidate=-1,g_card_route_stable;
static uint64_t g_card_route_since,g_card_next_preference_read;
static void card_operation_completed(const struct card_command *command,int status){
    int accepted;char event[128];
    pthread_mutex_lock(&g_viewarea_lock);
    accepted=card_policy_completed(&g_card_policy,command,status);
    pthread_mutex_unlock(&g_viewarea_lock);
    if(accepted){
        snprintf(event,sizeof(event),"card-ack-op-%d-position-%d-view-%d",command->operation,command->position,command->view);
        log_status(event,NULL,status);
    }
}
static void card_signal_ready(void *session){
    pthread_mutex_lock(&g_viewarea_lock);
    if(session==g_viewarea_session)g_card_policy.ready=1;
    pthread_mutex_unlock(&g_viewarea_lock);
}
#endif

static const char *viewarea_command_path(void)
{
    const char *value = getenv("CARPLAY_ALTSCREEN_VIEWAREA_COMMAND");
    return value && value[0] ? value :
        "/mnt/app/carplaymaps/alt-viewarea.command";
}

static const char *viewarea_navigation_state_path(void)
{
    const char *value = getenv("CARPLAY_NAVIGATION_STATE_PATH");
    return value && value[0] ? value :
        "/mnt/media/cpvc-runtime/v532-rgd-cache.pps";
}

static int read_navigation_route_state(int *route_state)
{
    FILE *file;
    char body[2048];
    char *marker;
    char *end;
    long value;
    size_t length;

    if (!route_state) return 0;
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
#ifndef CPMAP_CARD_PROFILE_791
    if(!getenv("CARPLAY_NAVIGATION_STATE_PATH")||!getenv("CARPLAY_NAVIGATION_STATE_PATH")[0])
        return cpmap_read_navigation_ownership("/ramdisk/cpmap-navigation-owner.state",(long)getpid(),route_state);
#endif
#endif
    file = fopen(viewarea_navigation_state_path(), "r");
    if (!file) return 0;
    length = fread(body, 1, sizeof(body) - 1, file);
    fclose(file);
    if (!length || length >= sizeof(body) - 1) return 0;
    body[length] = '\0';
    marker = strstr(body, "state::");
    if (!marker) return 0;
    marker += 7;
    errno = 0;
    value = strtol(marker, &end, 10);
    if (errno || end == marker || (*end != '\n' && *end != '\r' && *end))
        return 0;
    *route_state = (int)value;
    return 1;
}

static OSStatus send_viewarea_update(void *session, int index,
                                     int animation_duration_ms,
                                     int adjacent_index)
{
    SessionSendCommandFn send_command = (SessionSendCommandFn)resolve(
        "AirPlayReceiverSessionSendCommand");
    void *request = create_dictionary();
    void *params = create_dictionary();
    void *adjacent = create_array();
    AltCommandContext *context = NULL;
    OSStatus status = send_command && request && params && adjacent ? 0 : -1;

    if (index < 0 || index > 1 || animation_duration_ms < 0 ||
        adjacent_index < -1 || adjacent_index > 1) status = -1;
    if (!status) status = set_cstring(request, "type", "updateViewArea");
    if (!status) status = set_cstring(params, "uuid", ALT_UUID);
    if (!status) status = set_int(params, "animationDurationMillis",
                                  animation_duration_ms);
    if (!status) status = set_int(params, "viewAreaIndex", index);
    if (!status && adjacent_index >= 0)
        status = append_int(adjacent, adjacent_index);
    if (!status) status = set_object(params, "adjacentViewAreas", adjacent);
    if (!status) status = set_object(request, "params", params);
    if (!status) {
        context = (AltCommandContext *)calloc(1, sizeof(*context));
        if (!context) status = -1;
    }
    if (!status) {
        context->type = "updateViewArea";
        context->session = (void *)retain_cf(session);
        if (!context->session) status = -1;
    }
    if (!status) {
        log_object("viewarea-command-request", request);
        status = send_command(session, request, alt_command_completion,
                              context);
        if (!status) context = NULL;
    }
    log_status(index == 0 ? "viewarea-command-index-0" :
               "viewarea-command-index-1", session, status);
    if (context) {
        release_cf(context->session);
        free(context);
    }
    release_cf(adjacent);
    release_cf(params);
    release_cf(request);
    return status;
}

#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
static OSStatus send_card_operation(void *session,const struct card_command *command){
    SessionSendCommandFn send=(SessionSendCommandFn)resolve("AirPlayReceiverSessionSendCommand");
    void *request=create_dictionary(),*params=create_dictionary(),*adjacent=NULL;
    AltCommandContext *context=NULL;OSStatus status=send&&request&&params?0:-1;
    const char *type=command->operation==CARD_COMMAND_SCENE?"showUI":command->operation==CARD_COMMAND_VIEW?"updateViewArea":"forceKeyFrame";
    int j;
    if(command->operation<1||command->operation>3||command->position<0||command->position>3||command->view<0||command->view>=CPMAP_CARD_VIEW_COUNT)status=-1;
    if(!status)status=set_cstring(request,"type",type);
    if(!status)status=set_cstring(params,"uuid",ALT_UUID);
    if(!status&&command->operation==CARD_COMMAND_SCENE)status=set_cstring(params,"url",cpmap_card_url_for(command->position));
    if(!status&&command->operation==CARD_COMMAND_VIEW){
        adjacent=create_array();if(!adjacent)status=-1;
        if(!status)status=set_int(params,"animationDurationMillis",0);
        if(!status)status=set_int(params,"viewAreaIndex",command->view);
        for(j=0;!status&&j<CPMAP_CARD_VIEW_COUNT;j++)if(j!=command->view)status=append_int(adjacent,j);
        if(!status)status=set_object(params,"adjacentViewAreas",adjacent);
    }
    if(!status)status=set_object(request,"params",params);
    if(!status){context=(AltCommandContext *)calloc(1,sizeof(*context));if(!context)status=-1;}
    if(!status){
        context->type=type;context->card_owned=1;context->card=*command;
        context->session=(void *)retain_cf(session);if(!context->session)status=-1;
    }
    if(!status){
        log_object("card-scene-command",request);
        status=card_gated_send(send,session,request,context);
        if(!status)context=NULL;
    }
    if(context){release_cf(context->session);free(context);}
    release_cf(adjacent);release_cf(params);release_cf(request);return status;
}
/* One tick of the real native worker; also exercised directly by the CF tests.
 * No frame/size/decoder polling. Preferences are read at most twice a second;
 * the existing navigation-state debounce remains 400ms. */
static void card_runtime_poll(void){
    uint64_t now=monotonic_us();int route=0,has_route,send=0;
    struct cpmap_card_preference preference;struct card_command command;
    void *session=NULL;OSStatus status;
    if(now>=g_card_next_preference_read){g_card_next_preference_read=now+500000;card_refresh_preference();}
    preference=card_snapshot();has_route=read_navigation_route_state(&route);
    pthread_mutex_lock(&g_viewarea_lock);
    if(has_route){
        int candidate=route>0;
        if(g_card_route_candidate!=candidate){g_card_route_candidate=candidate;g_card_route_since=now;}
        else if(now>=g_card_route_since+400000)g_card_route_stable=candidate;
    }
    if(g_viewarea_session){
        card_policy_desire(&g_card_policy,preference.position,cpmap_card_view_for(preference.position,g_card_route_stable),preference.generation);
        if(card_policy_take(&g_card_policy,&command)){
            session=(void *)retain_cf(g_viewarea_session);
            if(session)send=1;else card_policy_defer_unsent(&g_card_policy,&command);
        }
    }
    pthread_mutex_unlock(&g_viewarea_lock);
    if(!send)return;
    status=send_card_operation(session,&command);
    if(status){
        pthread_mutex_lock(&g_viewarea_lock);
        if(status==-EBUSY)card_policy_defer_unsent(&g_card_policy,&command);
        else card_policy_completed(&g_card_policy,&command,status);
        pthread_mutex_unlock(&g_viewarea_lock);
        if(status!=-EBUSY)log_status("card-command-send-failed-no-auto-retry",session,status);
    }
    release_cf(session);
}
#endif

static void *viewarea_command_worker(void *unused)
{
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
    (void)unused;
    for(;;){card_runtime_poll();usleep(50000);}
    return NULL;
#else
    int candidate_index = -1;
    uint64_t candidate_since_us = 0;
    unsigned long observed_generation = 0;

    (void)unused;
    for (;;) {
        FILE *file = fopen(viewarea_command_path(), "r");
        if (file) {
            int index = -1;
            int duration_ms = 0;
            int adjacent_index = -1;
            void *session = NULL;
            int fields = fscanf(file, "%d %d %d", &index, &duration_ms,
                                &adjacent_index);
            fclose(file);
            unlink(viewarea_command_path());
            if (fields >= 1) {
                pthread_mutex_lock(&g_viewarea_lock);
                session = (void *)retain_cf(g_viewarea_session);
                if (session && index >= 0 && index <= 1)
                    g_viewarea_last_sent = index;
                pthread_mutex_unlock(&g_viewarea_lock);
                if (session) {
                    send_viewarea_update(session, index,
                                         fields >= 2 ? duration_ms : 0,
                                         fields >= 3 ? adjacent_index : -1);
                    release_cf(session);
                } else {
                    log_status("viewarea-command-no-session", NULL, -1);
                }
            }
        }
        {
            int route_state;
            if (read_navigation_route_state(&route_state)) {
                int desired_index = route_state > 0 ? 1 : 0;
                uint64_t now_us = monotonic_us();
                void *session = NULL;

                pthread_mutex_lock(&g_viewarea_lock);
                if (observed_generation != g_viewarea_generation) {
                    observed_generation = g_viewarea_generation;
                    candidate_index = -1;
                    candidate_since_us = now_us;
                }
                if (candidate_index != desired_index) {
                    candidate_index = desired_index;
                    candidate_since_us = now_us;
                } else if (g_viewarea_session &&
                           desired_index != g_viewarea_last_sent &&
                           now_us >= candidate_since_us + 400000ULL) {
                    session = (void *)retain_cf(g_viewarea_session);
                    /* Match the Audi implementation: dispatch once per
                     * stable transition instead of retrying stale commands. */
                    g_viewarea_last_sent = desired_index;
                }
                pthread_mutex_unlock(&g_viewarea_lock);

                if (session) {
                    send_viewarea_update(session, desired_index, 0,
                                         1 - desired_index);
                    release_cf(session);
                }
            }
        }
        {
            struct timeval delay;
            delay.tv_sec = 0;
            delay.tv_usec = 50000;
            select(0, NULL, NULL, NULL, &delay);
        }
    }
    return NULL;
#endif
}

static void remember_viewarea_session(void *session)
{
    void *retained = (void *)retain_cf(session);
    void *old;
    pthread_t thread;
    int thread_status = 0;

    pthread_mutex_lock(&g_viewarea_lock);
    old = g_viewarea_session;
    g_viewarea_session = retained;
    ++g_viewarea_generation;
    g_viewarea_last_sent = ALTSCREEN_INITIAL_VIEW_AREA;
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
    card_policy_reset(&g_card_policy,g_viewarea_generation,cpmap_card_view_for(card_snapshot().position,0));
    g_card_route_candidate=-1;g_card_route_stable=0;g_card_route_since=0;
    card_wire_reset(session);
#endif
    if (!g_viewarea_worker_started) {
        thread_status = pthread_create(&thread, NULL,
                                       viewarea_command_worker, NULL);
        if (thread_status == 0) {
            pthread_detach(thread);
            g_viewarea_worker_started = 1;
        }
    }
    pthread_mutex_unlock(&g_viewarea_lock);
    release_cf(old);
    log_status("viewarea-session-remembered", retained,
               retained && thread_status == 0 ? 0 : -1);
}

static void forget_viewarea_session(void *session)
{
    void *old = NULL;

    pthread_mutex_lock(&g_viewarea_lock);
    if (g_viewarea_session == session) {
        old = g_viewarea_session;
        g_viewarea_session = NULL;
        ++g_viewarea_generation;
        g_viewarea_last_sent = ALTSCREEN_INITIAL_VIEW_AREA;
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
        card_policy_reset(&g_card_policy,g_viewarea_generation,0);
        card_wire_reset(NULL);
#endif
    }
    pthread_mutex_unlock(&g_viewarea_lock);
    release_cf(old);
    if (old) log_status("viewarea-session-forgotten", session, 0);
}

#endif /* ALTSCREEN_RUNTIME_VIEWAREA_CONTROL */

static void *force_keyframe_thread(void *context)
{
    AltForceTask *task = (AltForceTask *)context;
    log_status("alt-force-after-showui-dispatch", task->session, 0);
    send_alt_command(task->session, "forceKeyFrame", NULL);
    release_cf(task->session);
    free(task);
    return NULL;
}

typedef struct AltActivationTask {
    void *session;
    unsigned int delay_us;
} AltActivationTask;

static void *delayed_alt_activation_thread(void *context)
{
    AltActivationTask *task = (AltActivationTask *)context;
    /* The stock QNX path is equivalent to LIVI's software path: SETUP only
     * opens the main receiver; actual readiness follows when the phone connects
     * and delivers its first main-screen frame. One second comfortably clears
     * that boundary without blocking the RTSP SETUP response. */
    if (task->delay_us) usleep(task->delay_us);
    log_status(task->delay_us ? "alt-delayed-activation" :
               "alt-screen-ready-activation", task->session, 0);
    /* Send showUI by itself. The stock 210.81 event client cannot reliably
     * correlate two completion-bearing commands queued back-to-back. A keyframe
     * request is only useful after the alt TCP connection exists anyway. */
#ifdef ALTSCREEN_RUNTIME_CARD_POSITION
    /* The same READY boundary now opens the single serialized worker. */
    card_signal_ready(task->session);
#else
    send_alt_command(task->session, "showUI", ALT_MAP_URL);
#endif
    release_cf(task->session);
    free(task);
    return NULL;
}

static OSStatus schedule_alt_activation(void *session, unsigned int delay_us)
{
    AltActivationTask *task;
    pthread_t thread;
    int status;

    task = (AltActivationTask *)calloc(1, sizeof(*task));
    if (!task) return -1;
    task->session = (void *)retain_cf(session);
    if (!task->session) {
        free(task);
        return -1;
    }
    task->delay_us = delay_us;
    status = pthread_create(&thread, NULL, delayed_alt_activation_thread, task);
    if (status != 0) {
        release_cf(task->session);
        free(task);
        return -1;
    }
    pthread_detach(thread);
    log_status("alt-activation-scheduled", session, 0);
    return 0;
}

#ifdef ALTSCREEN_ACTIVATE_ON_SCREEN_READY

/*
 * The stock 210.81 receiver changes its screen state to READY (value 2)
 * immediately after ScreenStreamProcessData accepts the first main-screen
 * frame. This is the exact readiness boundary used by LIVI before showUI;
 * observing it avoids a guessed delay after SETUP/RECORD.
 */
static pthread_mutex_t g_ready_lock = PTHREAD_MUTEX_INITIALIZER;
static void *g_ready_session;
static int g_ready_activation_pending;
static int g_last_screen_state = -1;

static void remember_alt_session(void *session)
{
    void *retained = (void *)retain_cf(session);
    void *old;
    pthread_mutex_lock(&g_ready_lock);
    old = g_ready_session;
    g_ready_session = retained;
    g_ready_activation_pending = retained != NULL;
    pthread_mutex_unlock(&g_ready_lock);
    release_cf(old);
    log_status("alt-session-remembered", retained, retained ? 0 : -1);
}

void AirPlayReceiverSessionScreen_UpdateState(void *screen, int state)
{
    ScreenUpdateStateFn real_function = (ScreenUpdateStateFn)resolve(
        "AirPlayReceiverSessionScreen_UpdateState");
    void *session = NULL;
    int log_transition = 0;

    if (real_function) real_function(screen, state);
    if (!enabled()) return;

    pthread_mutex_lock(&g_ready_lock);
    if (state != g_last_screen_state) {
        g_last_screen_state = state;
        log_transition = 1;
    }
    if (g_ready_activation_pending && g_ready_session) {
        if (state == 2) {
            session = g_ready_session;
            g_ready_session = NULL;
            g_ready_activation_pending = 0;
        }
    }
    pthread_mutex_unlock(&g_ready_lock);

    if (log_transition) log_capture("stock-screen-state", state, 0, 0);

    if (session) {
        OSStatus status = schedule_alt_activation(session, 0);
        log_status("alt-screen-ready-schedule-result", session, status);
        release_cf(session);
    }
}

#endif /* ALTSCREEN_ACTIVATE_ON_SCREEN_READY */

OSStatus AirPlayReceiverSessionStart(void *session, void *start_info)
{
    SessionStartFn real_function = (SessionStartFn)resolve(
        "AirPlayReceiverSessionStart");
    OSStatus status;

    if (!real_function) {
        log_status("session-start-missing-real", session, -1);
        return -1;
    }
    status = real_function(session, start_info);
    log_status("session-start-result", session, status);
#ifdef ALTSCREEN_RUNTIME_MAP_CONTROL
    if (enabled() && status == 0) remember_map_control_session(session);
#endif
#ifdef ALTSCREEN_RUNTIME_VIEWAREA_CONTROL
    if (enabled() && status == 0) remember_viewarea_session(session);
#endif
    return status;
}

#if defined(ALTSCREEN_RUNTIME_MAP_CONTROL) || \
    defined(ALTSCREEN_RUNTIME_VIEWAREA_CONTROL)
void AirPlayReceiverSessionTearDown(void *session, const void *request,
                                    OSStatus reason,
                                    unsigned char *out_done)
{
    SessionTearDownFn real_function = (SessionTearDownFn)resolve(
        "AirPlayReceiverSessionTearDown");
    if (real_function) real_function(session, request, reason, out_done);
    if (!out_done || *out_done) {
#ifdef ALTSCREEN_RUNTIME_MAP_CONTROL
        forget_map_control_session(session);
#endif
#ifdef ALTSCREEN_RUNTIME_VIEWAREA_CONTROL
        forget_viewarea_session(session);
#endif
    }
}
#endif

#endif /* ALTSCREEN_ACTIVATE_DISPLAY */

#endif /* ALTSCREEN_PASSIVE_CAPTURE */

OSStatus AirPlayReceiverSessionSetup(void *session, const void *request,
                                     void **response)
{
    SessionSetupFn real_function = (SessionSetupFn)resolve(
        "AirPlayReceiverSessionSetup");
    const void *streams;
    int stream_type = -1;
    OSStatus status;

    if (!real_function) {
        log_status("session-setup-missing-real", session, -1);
        return -1;
    }
    streams = request ? get_object(request, "streams") : NULL;
#ifdef ALTSCREEN_PASSIVE_CAPTURE
    if (streams) stream_type = first_stream_type(request);
#endif
    log_object(streams ? "stream-setup-request" : "session-setup-request",
               request);

#ifdef ALTSCREEN_PASSIVE_CAPTURE
    if (enabled() && streams && stream_type == 111) {
        uint16_t port = 0;
        void *passive_response = NULL;
#ifdef ALTSCREEN_AES_CTR_DECRYPT
        uint8_t alt_aes_key[16];
        uint8_t alt_aes_iv[16];
        int alt_aes_valid = 0;
        OSStatus stream_id_status = -1;
        uint64_t stream_connection_id = (uint64_t)first_stream_int64(
            request, "streamConnectionID", &stream_id_status);
        DeriveScreenAESFn derive_screen_aes =
            (DeriveScreenAESFn)resolve("AirPlay_DeriveAESKeySHA512ForScreen");

        memset(alt_aes_key, 0, sizeof(alt_aes_key));
        memset(alt_aes_iv, 0, sizeof(alt_aes_iv));
        if (stream_id_status == 0 && stream_connection_id != 0 &&
            derive_screen_aes) {
            const uint8_t *session_aes_key =
                (const uint8_t *)session + MHI2_SESSION_AES_KEY_OFFSET;
            derive_screen_aes(session_aes_key, 16, stream_connection_id,
                              alt_aes_key, alt_aes_iv);
            alt_aes_valid = 1;
            log_capture("alt-aes-key-derived", 16, 16,
                        (unsigned long long)stream_connection_id);
        } else {
            log_capture("alt-aes-key-derive-unavailable", stream_id_status,
                        derive_screen_aes != NULL,
                        (unsigned long long)stream_connection_id);
        }
#endif
#ifdef ALTSCREEN_STOCK_FIRST_SETUP
        void *stock_response = NULL;

        /*
         * libairplay 210.81 returns success and an empty response for type 111.
         * Although it does not expose a data port, it may still associate the
         * streamConnectionID with the receiver session and prepare the screen
         * security/lifecycle state.  Preserve those side effects before adding
         * the response that the old implementation omits.
         */
        status = real_function(session, request, &stock_response);
        log_status("alt-stock-first-result", session, status);
        if (stock_response)
            log_object("alt-stock-first-response", stock_response);
        if (status != 0) {
            if (response) {
                *response = stock_response;
                stock_response = NULL;
            }
            release_cf(stock_response);
            return status;
        }
#ifndef ALTSCREEN_STOCK_RESPONSE_MERGE
        release_cf(stock_response);
        stock_response = NULL;
#endif
#endif
        status = start_passive_listener(&port
#ifdef ALTSCREEN_AES_CTR_DECRYPT
                                        , alt_aes_key, alt_aes_iv,
                                        alt_aes_valid
#endif
#ifdef ALTSCREEN_ACTIVATE_DISPLAY
                                        , session
#endif
                                        );
        if (!status) passive_response = create_alt_stream_response(port);
#if defined(ALTSCREEN_STOCK_FIRST_SETUP) && \
    defined(ALTSCREEN_STOCK_RESPONSE_MERGE)
        if (passive_response && stock_response) {
            const void *new_streams = get_object(passive_response, "streams");
            status = new_streams ?
                set_object(stock_response, "streams", new_streams) : -1;
            if (!status) {
                release_cf(passive_response);
                passive_response = stock_response;
                stock_response = NULL;
                log_object("alt-stock-response-merged", passive_response);
            } else {
                log_status("alt-stock-response-merge-failed", stock_response,
                           status);
                release_cf(passive_response);
                passive_response = NULL;
            }
        }
        release_cf(stock_response);
#endif
        if (!passive_response) {
            log_status("alt-passive-setup-failed", session, -1);
            return -1;
        }
        if (response) *response = passive_response;
#if defined(ALTSCREEN_ACTIVATE_DISPLAY) && \
    defined(ALTSCREEN_ACTIVATE_ON_SCREEN_READY)
        remember_alt_session(session);
#endif
        log_capture("alt-passive-port-created", port, 111, 0);
        log_object("alt-passive-setup-response", passive_response);
        return 0;
    }
#endif

    status = real_function(session, request, response);
    log_status(streams ? "stream-setup-result" : "session-setup-result",
               session, status);

#if defined(ALTSCREEN_PASSIVE_CAPTURE) && defined(ALTSCREEN_ACTIVATE_DISPLAY)
    /* Match LIVI's software path: activate after the main stream has begun. */
    if (!status && enabled() && streams && stream_type == 110) {
#ifndef ALTSCREEN_ACTIVATE_ON_SCREEN_READY
        OSStatus schedule_status = schedule_alt_activation(session,
                                                            1000U * 1000U);
        log_status("alt-activation-schedule-result", session, schedule_status);
#endif
    }
#endif

    if (!status && enabled() && !streams && response && *response) {
        set_enabled_features(*response);
    }
    if (response && *response)
        log_object(streams ? "stream-setup-response" :
                   "session-setup-response", *response);
    return status;
}

void *AirPlayCopyServerInfo(void *server, const void *device_id,
                            const void *interface_address, OSStatus *error)
{
    CopyServerInfoFn real_function = (CopyServerInfoFn)resolve(
        "AirPlayCopyServerInfo");
    void *result;
    if (!real_function) {
        if (error) *error = -1;
        log_status("copy-server-info-missing-real", server, -1);
        return NULL;
    }
#ifdef ALTSCREEN_ALT_KNOB_HID
    if (enabled()) ensure_alt_knob();
#endif
    result = real_function(server, device_id, interface_address, error);
    log_object("server-info-original", result);
    if (result && enabled()) {
#ifdef ALTSCREEN_ADVERTISE_CARPLAY_CONTROL
        add_carplay_control_feature(result);
#endif
#ifdef ALTSCREEN_ADVERTISE_SOURCE_950
        {
            OSStatus source_status = set_cstring(result, "sourceVersion",
                                                 "950.7.1");
            log_status("server-info-source-950", result, source_status);
        }
#endif
        append_alt_display(result);
    }
    log_object("server-info-result", result);
    return result;
}

__attribute__((constructor))
static void probe_loaded(void)
{
    log_status("altscreen-handshake-probe-loaded", NULL, 0);
}
