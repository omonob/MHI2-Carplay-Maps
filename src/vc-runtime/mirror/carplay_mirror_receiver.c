#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "carplay_mirror_protocol.h"
#include "carplay_yuv_shm.h"
#include "cpvc_mirror_control.h"

typedef int32_t NvSSStatus;

typedef struct NvSSVideoOpenParams {
    uint32_t output_device;
    uint32_t reserved_04;
    uint8_t priority;
    uint8_t visible;
    uint8_t layer_id;
    uint8_t reserved_0b;
    uint32_t queue_depth;
    uint32_t reserved_10;
    uint32_t reserved_14;
} NvSSVideoOpenParams;

typedef struct NvSSVideoStreamParams {
    uint32_t width;
    uint32_t height;
    float fps;
} NvSSVideoStreamParams;

typedef struct NvSSVideoAccessUnit {
    uint8_t *data;
    uint32_t size;
    uint64_t pts;
    uint8_t append_markers;
    uint8_t reserved_11[3];
} NvSSVideoAccessUnit;

extern NvSSStatus NvSSVideoOpen(void **, const NvSSVideoOpenParams *);
extern NvSSStatus NvSSVideoStreamConfigure(void *,
                                            const NvSSVideoStreamParams *);
extern NvSSStatus NvSSVideoDecode(void *, const NvSSVideoAccessUnit *,
                                  uint32_t);
extern NvSSStatus NvSSVideoClose(void *);

#define NVSS_OK ((NvSSStatus)0)
#define NVSS_BUFFER_NOT_CONSUMED ((NvSSStatus)0x80000006u)
#ifndef CPVC_BUILD_PROFILE
#error CPVC_BUILD_PROFILE must be explicitly 790 or 791
#endif
#if CPVC_BUILD_PROFILE==790
static unsigned int cpvc_output_width=1010u,cpvc_output_height=376u;
#elif CPVC_BUILD_PROFILE==791
static unsigned int cpvc_output_width=800u,cpvc_output_height=480u;
#else
#error Unsupported instrument profile
#endif
#define OUTPUT_WIDTH cpvc_output_width
#define OUTPUT_HEIGHT cpvc_output_height
#define MAX_SOURCE_WIDTH 1920u
#define MAX_SOURCE_HEIGHT 1080u
#define MAX_Y_SIZE (MAX_SOURCE_WIDTH * MAX_SOURCE_HEIGHT)
#define MAX_UV_SIZE (((MAX_SOURCE_WIDTH + 1u) / 2u) * \
                     ((MAX_SOURCE_HEIGHT + 1u) / 2u))
#define MAX_FRAME_SIZE (MAX_Y_SIZE + 2u * MAX_UV_SIZE)

typedef struct FrameStore {
    pthread_mutex_t mutex;
    uint8_t *slots[2];
    unsigned int front;
    uint32_t width;
    uint32_t height;
    uint64_t sequence;
    uint64_t published;
    uint64_t timestamp_us;
} FrameStore;

typedef struct DecoderConfig {
    uint16_t port;
    uint32_t layer;
    uint32_t decode_width;
    uint32_t decode_height;
} DecoderConfig;

static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_session_done;
static volatile sig_atomic_t g_sender_connected;
static volatile sig_atomic_t g_stream_active;
static FrameStore g_frames = {
    PTHREAD_MUTEX_INITIALIZER, { NULL, NULL }, 0, 0, 0, 0, 0, 0
};

static uint64_t monotonic_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL +
           (uint64_t)ts.tv_nsec / 1000ULL;
}

#include "cpvc_output_contract.h"

static void stop_handler(int signo)
{
    (void)signo;
    g_running = 0;
}

/* Exported for libnvmedia-yuv-bridge.so (found with dlsym RTLD_DEFAULT). */
void NvssBridgePublishYuv(uint32_t width, uint32_t height,
                          const uint8_t *y, uint32_t pitch_y,
                          const uint8_t *u, uint32_t pitch_u,
                          const uint8_t *v, uint32_t pitch_v)
{
    unsigned int back;
    uint8_t *dst;
    uint32_t row;

    uint32_t chroma_width = (width + 1u) / 2u;
    uint32_t chroma_height = (height + 1u) / 2u;
    size_t y_size = (size_t)width * height;
    size_t uv_size = (size_t)chroma_width * chroma_height;

    if (width < 320 || height < 240 || width > MAX_SOURCE_WIDTH ||
        height > MAX_SOURCE_HEIGHT || !y || !u || !v ||
        pitch_y < width || pitch_u < chroma_width ||
        pitch_v < chroma_width) {
        return;
    }

    pthread_mutex_lock(&g_frames.mutex);
    back = g_frames.front ^ 1u;
    dst = g_frames.slots[back];
    if (dst) {
        for (row = 0; row < height; ++row) {
            memcpy(dst + (size_t)row * width,
                   y + (size_t)row * pitch_y, width);
        }
        dst += y_size;
        for (row = 0; row < chroma_height; ++row) {
            memcpy(dst + (size_t)row * chroma_width,
                   u + (size_t)row * pitch_u, chroma_width);
        }
        dst += uv_size;
        for (row = 0; row < chroma_height; ++row) {
            memcpy(dst + (size_t)row * chroma_width,
                   v + (size_t)row * pitch_v, chroma_width);
        }
        g_frames.front = back;
        g_frames.width = width;
        g_frames.height = height;
        ++g_frames.sequence;
        ++g_frames.published;
    }
    pthread_mutex_unlock(&g_frames.mutex);
}

static int parse_u32(const char *text, uint32_t minimum, uint32_t maximum,
                     uint32_t *value)
{
    char *end = NULL;
    unsigned long parsed;
    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno || !end || end == text || *end != '\0' ||
        parsed < minimum || parsed > maximum) return -1;
    *value = (uint32_t)parsed;
    return 0;
}

static int wait_readable(int fd, unsigned int milliseconds)
{
    fd_set readfds;
    struct timeval timeout;
    int result;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = milliseconds / 1000u;
    timeout.tv_usec = (milliseconds % 1000u) * 1000u;
    result = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (result < 0 && errno == EINTR) return 0;
    return result;
}

static int receive_exact(int fd, void *buffer, uint32_t length)
{
    uint8_t *out = (uint8_t *)buffer;
    uint32_t received = 0;
    while (g_running && received < length) {
        ssize_t result;
        if (wait_readable(fd, 200) <= 0) continue;
        result = recv(fd, out + received, length - received, 0);
        if (result > 0) {
            received += (uint32_t)result;
            continue;
        }
        if (result < 0 && errno == EINTR) continue;
        return -1;
    }
    return received == length ? 0 : -1;
}

static int create_listener(uint16_t port)
{
    int fd;
    int one = 1;
    struct sockaddr_in address;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(fd, 1) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void *decoder_thread(void *opaque)
{
    DecoderConfig *config = (DecoderConfig *)opaque;
    NvSSVideoOpenParams open_params;
    NvSSVideoStreamParams stream_params;
    NvSSStatus status;
    void *decoder = NULL;
    uint8_t *access_unit_data = NULL;
    uint64_t submitted = 0, errors = 0;
    uint64_t started_us = 0, last_report_us = 0;
    int listener = -1, client = -1;
    uint32_t configured_fps = 0;

    listener = create_listener(config->port);
    if (listener < 0) {
        fprintf(stderr, "carplay-mirror: listen port %u failed: %s\n",
                config->port, strerror(errno));
        goto done;
    }
    fprintf(stderr, "carplay-mirror: listening on 0.0.0.0:%u\n",
            config->port);
    while (g_running && client < 0) {
        if (wait_readable(listener, 200) > 0) client = accept(listener, NULL, NULL);
    }
    if (client < 0) goto done;
    g_sender_connected = 1;
    fprintf(stderr, "carplay-mirror: dio_manager tap connected\n");

    access_unit_data = (uint8_t *)malloc(CARPLAY_MIRROR_MAX_ACCESS_UNIT);
    if (!access_unit_data) goto done;
    started_us = last_report_us = monotonic_us();
    while (g_running) {
        CarPlayMirrorHeader header;

        if (receive_exact(client, &header, sizeof(header)) != 0) break;
        if (header.magic != CARPLAY_MIRROR_MAGIC ||
            header.version != CARPLAY_MIRROR_VERSION ||
            header.payload_size >
                sizeof(CarPlayMirrorFrame) + CARPLAY_MIRROR_MAX_ACCESS_UNIT) {
            fprintf(stderr, "carplay-mirror: invalid protocol header\n");
            break;
        }

        if (header.type == CARPLAY_MIRROR_CONFIG) {
            CarPlayMirrorConfig incoming;
            if (header.payload_size != sizeof(incoming) ||
                receive_exact(client, &incoming, sizeof(incoming)) != 0)
                break;
            if (incoming.width < 320 || incoming.width > MAX_SOURCE_WIDTH ||
                incoming.height < 240 || incoming.height > MAX_SOURCE_HEIGHT ||
                incoming.fps_milli < 1000 || incoming.fps_milli > 120000) {
                fprintf(stderr, "carplay-mirror: rejected config %ux%u@%u\n",
                        incoming.width, incoming.height,
                        incoming.fps_milli);
                break;
            }
            if (decoder) {
                (void)NvSSVideoClose(decoder);
                decoder = NULL;
            }
            memset(&open_params, 0, sizeof(open_params));
            open_params.output_device = 0;
            open_params.priority = 100;
            open_params.visible = 1;
            open_params.layer_id = (uint8_t)config->layer;
            open_params.queue_depth = 8;
            status = NvSSVideoOpen(&decoder, &open_params);
            fprintf(stderr,
                    "carplay-mirror: NvSSVideoOpen status=0x%08x handle=%p\n",
                    (unsigned int)status, decoder);
            if (status != NVSS_OK || !decoder) break;
            stream_params.width = config->decode_width ?
                                      config->decode_width : incoming.width;
            stream_params.height = config->decode_height ?
                                       config->decode_height : incoming.height;
            stream_params.fps = (float)incoming.fps_milli / 1000.0f;
            configured_fps = (incoming.fps_milli + 500u) / 1000u;
            status = NvSSVideoStreamConfigure(decoder, &stream_params);
            fprintf(stderr,
                    "carplay-mirror: source=%ux%u@%.3f decoder=%ux%u "
                    "configure=0x%08x\n",
                    incoming.width, incoming.height, stream_params.fps,
                    stream_params.width, stream_params.height,
                    (unsigned int)status);
            if (status != NVSS_OK) break;
            g_stream_active = 1;
            continue;
        }

        if (header.type == CARPLAY_MIRROR_STOP) {
            if (header.payload_size != 0) break;
            fprintf(stderr, "carplay-mirror: source stream stopped\n");
            g_stream_active = 0;
            break;
        }

        if (header.type == CARPLAY_MIRROR_FRAME) {
            CarPlayMirrorFrame frame;
            NvSSVideoAccessUnit unit;
            unsigned int attempt;
            if (!decoder || header.payload_size < sizeof(frame) ||
                receive_exact(client, &frame, sizeof(frame)) != 0 ||
                frame.data_size > CARPLAY_MIRROR_MAX_ACCESS_UNIT ||
                header.payload_size != sizeof(frame) + frame.data_size ||
                receive_exact(client, access_unit_data,
                              frame.data_size) != 0) {
                break;
            }
            memset(&unit, 0, sizeof(unit));
            unit.data = access_unit_data;
            unit.size = frame.data_size;
            unit.pts = frame.pts;
            unit.append_markers = frame.append_markers;
            status = NVSS_OK;
            for (attempt = 0; attempt < 4; ++attempt) {
                status = NvSSVideoDecode(
                    decoder, &unit,
                    configured_fps ? (1000u + configured_fps - 1u) /
                                         configured_fps : 34u);
                if (status != NVSS_BUFFER_NOT_CONSUMED) break;
                usleep(5000);
            }
            if (status != NVSS_OK) ++errors;
            ++submitted;
        } else {
            fprintf(stderr, "carplay-mirror: unknown message type=%u\n",
                    header.type);
            break;
        }

        if (monotonic_us() - last_report_us >= 1000000ULL) {
            uint64_t now = monotonic_us();
            double elapsed = (double)(now - started_us) / 1000000.0;
            fprintf(stderr,
                    "carplay-mirror: submitted=%llu errors=%llu avg=%.2f fps "
                    "published=%llu\n",
                    (unsigned long long)submitted,
                    (unsigned long long)errors,
                    elapsed > 0.0 ? submitted / elapsed : 0.0,
                    (unsigned long long)g_frames.published);
            last_report_us = now;
        }
    }
    fprintf(stderr, "carplay-mirror: tap disconnected\n");

done:
    g_sender_connected = 0;
    g_stream_active = 0;
    if (client >= 0) close(client);
    if (listener >= 0) close(listener);
    free(access_unit_data);
    if (decoder) {
        status = NvSSVideoClose(decoder);
        fprintf(stderr, "carplay-mirror: NvSSVideoClose status=0x%08x\n",
                (unsigned int)status);
    }
    g_session_done = 1;
    return NULL;
}

static void *shared_yuv_thread(void *unused)
{
    int fd = -1;
    CarPlayYuvSharedHeader *shared = NULL;
    uint32_t last_sequence = 0;
    (void)unused;

    while (g_running && fd < 0) {
        fd = open(CARPLAY_YUV_SHM_PATH, O_RDWR);
        if (fd < 0) usleep(100000);
    }
    if (fd < 0) goto done;
    shared = (CarPlayYuvSharedHeader *)mmap(
        NULL, CARPLAY_YUV_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        shared = NULL;
        goto done;
    }
    fprintf(stderr, "carplay-yuv: shared memory mapped, waiting for frame\n");

    while (g_running) {
        uint32_t sequence;
        if (shared->magic != CARPLAY_YUV_MAGIC ||
            shared->version != CARPLAY_YUV_VERSION ||
            shared->output_width != CARPLAY_YUV_WIDTH ||
            shared->output_height != CARPLAY_YUV_HEIGHT) {
            usleep(10000);
            continue;
        }
        g_stream_active = shared->stream_active ? 1 : 0;
        sequence = shared->sequence;
        if (sequence != last_sequence) {
            unsigned int back;
            uint8_t *destination;
            const uint8_t *source;
            uint32_t slot;
            __sync_synchronize();
            slot = shared->active_slot % CARPLAY_YUV_SLOT_COUNT;
            source = CarPlayYuvSlot(shared, slot);
            pthread_mutex_lock(&g_frames.mutex);
            back = g_frames.front ^ 1u;
            destination = g_frames.slots[back];
            if (destination) {
                memcpy(destination, source, CARPLAY_YUV_FRAME_SIZE);
                g_frames.front = back;
                g_frames.width = CARPLAY_YUV_WIDTH;
                g_frames.height = CARPLAY_YUV_HEIGHT;
                g_frames.timestamp_us = shared->timestamp_us;
                ++g_frames.sequence;
                ++g_frames.published;
                last_sequence = sequence;
            }
            pthread_mutex_unlock(&g_frames.mutex);
        } else {
            usleep(1000);
        }
    }

done:
    g_stream_active = 0;
    if (shared) munmap(shared, CARPLAY_YUV_SHM_SIZE);
    if (fd >= 0) close(fd);
    g_session_done = 1;
    return NULL;
}

static GLuint compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    GLint ok = 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "nvss-most: shader compile failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint create_program(void)
{
    static const char *vertex_source =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 uv;\n"
        "void main(){gl_Position=vec4(position,0.0,1.0);uv=texcoord;}\n";
    static const char *fragment_source =
        "precision mediump float;\n"
        "varying vec2 uv;\n"
        "uniform sampler2D tex_y;\n"
        "uniform sampler2D tex_u;\n"
        "uniform sampler2D tex_v;\n"
        "void main(){\n"
        " float y=1.16438356*(texture2D(tex_y,uv).r-0.0625);\n"
        " float u=texture2D(tex_u,uv).r-0.5;\n"
        " float v=texture2D(tex_v,uv).r-0.5;\n"
        " gl_FragColor=vec4(y+1.59602678*v,"
        "y-0.39176229*u-0.81296764*v,y+2.01723214*u,1.0);\n"
        "}\n";
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    GLuint program;
    GLint ok = 0;
    if (!vs || !fs) return 0;
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "position");
    glBindAttribLocation(program, 1, "texcoord");
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fprintf(stderr, "nvss-most: program link failed: %s\n", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static void configure_texture(GLuint texture, unsigned int unit,
                              GLint uniform, unsigned int width,
                              unsigned int height)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glUniform1i(uniform, unit);
}

static int report_gl_errors(const char *stage)
{
    GLenum error;
    int failed = 0;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "nvss-most: GL error stage=%s code=0x%04x\n",
                stage ? stage : "?", (unsigned int)error);
        failed = 1;
    }
    return failed;
}

static int switch_context(unsigned int context)
{
    char command[160];
    int first, second;
    snprintf(command, sizeof(command),
             "IPL_CONFIG_DIR=/etc/eso/production /eso/bin/apps/dmdt dc 70 %u",
             context);
    first = system(command);
    second = system("IPL_CONFIG_DIR=/etc/eso/production "
                    "/eso/bin/apps/dmdt sc 4 70");
    fprintf(stderr, "nvss-most: DMDT context=%u result=%d/%d\n",
            context, first, second);
    return first == 0 && second == 0 ? 0 : -1;
}

int main(int argc, char **argv)
{
    pid_t owner_pid = getppid();
    uint32_t port = CARPLAY_MIRROR_DEFAULT_PORT, layer = 3;
    uint32_t decode_width = 0, decode_height = 0;
    int shared_mode = 0;
    DecoderConfig decoder_config;
    void *display_library = NULL;
    void (*display_init)(int *, int *) = NULL;
    void (*display_deinit)(void) = NULL;
    int (*display_create_window)(EGLDisplay, EGLConfig, int, int, int,
                                  EGLNativeWindowType *, int *) = NULL;
    void (*display_destroy_window)(int) = NULL;
    int (*kd_get_size)(int,int,int *) = NULL;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config;
    EGLint count = 0, major = 0, minor = 0;
    EGLNativeWindowType native_window;
    int kd_window = 0;
    GLuint program = 0, textures[3] = {0, 0, 0};
    pthread_t decoder;
    int decoder_started = 0, context_active = 0;
    int context_switch_attempted = 0;
    int post_switch_latch_pending = 0;
    uint64_t uploaded_sequence = 0;
    uint64_t render_count = 0, report_start = 0;
    uint64_t last_frame_timestamp_us = 0;
    uint64_t latency_last_us = 0, latency_total_us = 0;
    uint64_t latency_min_us = UINT64_MAX, latency_max_us = 0;
    uint64_t latency_samples = 0;
    uint64_t swap_last_us = 0, swap_total_us = 0;
    uint64_t swap_min_us = UINT64_MAX, swap_max_us = 0;
    uint64_t swap_samples = 0;
    uint32_t texture_width = 0, texture_height = 0;
    int framebuffer_sampled = 0;
    int result = 1;
    int canvas_socket=-1,canvas_changed=1,canvas_redraw=1;
    unsigned long long canvas_dumped_generation=~0ULL;
    struct cpvc_canvas canvas={100,0};
    struct cpvc_rect canvas_rect;
    uint64_t policy_check_us=0;
    int self_test=(argc==2 && strcmp(argv[1],"--output-selftest")==0);
    int output_failed=0;
    static const GLfloat vertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    };
    static const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };

    if(getenv("CPVC_CLUSTER_PROFILE") && atoi(getenv("CPVC_CLUSTER_PROFILE"))!=CPVC_BUILD_PROFILE){fprintf(stderr,"cpvc-mirror: wrong cluster profile; no context change\n");return 2;}
    fprintf(stderr,"cpvc-mirror: %s build_profile=%d output=%ux%u displayable=%u\n",CPVC_OUTPUT_BUILD,CPVC_BUILD_PROFILE,OUTPUT_WIDTH,OUTPUT_HEIGHT,CPVC_MIRROR_DISPLAYABLE);
    if(!self_test && !cpvc_mirror_allowed()){fprintf(stderr,"cpvc-mirror: permission off, no context change\n");return 0;}
    if(cpvc_canvas_read(cpvc_state_dir(),&canvas)<0){fprintf(stderr,"cpvc-mirror: corrupt canvas state\n");return 3;}
    if (argc > 1 && strcmp(argv[1], "shm") == 0) shared_mode = 1;
    if (!self_test && ((shared_mode && argc != 2) || (!shared_mode &&
        (argc > 5 || argc == 4 ||
        (argc > 1 && parse_u32(argv[1], 1, 65535, &port) != 0) ||
        (argc > 2 && parse_u32(argv[2], 0, 255, &layer) != 0) ||
        (argc > 3 && parse_u32(argv[3], 320, MAX_SOURCE_WIDTH,
                              &decode_width) != 0) ||
        (argc > 4 && parse_u32(argv[4], 240, MAX_SOURCE_HEIGHT,
                              &decode_height) != 0))))) {
        fprintf(stderr,
                "usage: %s shm | [port [nvssLayer "
                "[decodeWidth decodeHeight]]]\n",
                argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    signal(SIGINT, stop_handler);
    signal(SIGTERM, stop_handler);
    g_frames.slots[0] = (uint8_t *)malloc(MAX_FRAME_SIZE);
    g_frames.slots[1] = (uint8_t *)malloc(MAX_FRAME_SIZE);
    if (!g_frames.slots[0] || !g_frames.slots[1]) goto cleanup;

    display_library = dlopen("libdisplayinit.so", RTLD_NOW | RTLD_GLOBAL);
    if (!display_library) {
        fprintf(stderr, "nvss-most: libdisplayinit: %s\n", dlerror());
        goto cleanup;
    }
    display_init = (void (*)(int *, int *))dlsym(display_library, "display_init");
    display_deinit = (void (*)(void))dlsym(display_library, "display_deinit");
    display_create_window = (int (*)(EGLDisplay, EGLConfig, int, int, int,
                                      EGLNativeWindowType *, int *))dlsym(
        display_library, "display_create_window");
    display_destroy_window=(void (*)(int))dlsym(display_library,"display_destroy_window");
    kd_get_size=(int (*)(int,int,int *))dlsym(RTLD_DEFAULT,"kdGetWindowPropertyiv");
    if (!display_init || !display_create_window || !display_destroy_window || !kd_get_size) goto cleanup;
    if (cpvc_query_displayable(0)!=0) {
        fprintf(stderr,"cpvc-output: REJECT displayable=%u already exists or query failed; no route\n",CPVC_MIRROR_DISPLAYABLE);
        goto cleanup;
    }
    display_init(0, 0);

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, &major, &minor) ||
        !eglChooseConfig(display, config_attributes, &config, 1, &count) ||
        count < 1) {
        fprintf(stderr, "nvss-most: EGL initialization failed 0x%x\n",
                eglGetError());
        goto cleanup;
    }
    if(display_create_window(display, config, OUTPUT_WIDTH, OUTPUT_HEIGHT,
                             CPVC_MIRROR_DISPLAYABLE,&native_window,&kd_window)!=1 || !kd_window) {
        fprintf(stderr,"cpvc-output: REJECT native window creation failed\n");goto cleanup;
    }
    {
        int actual[2]={0,0};
        if(kd_get_size(kd_window,66,actual)!=0 || actual[0]!=(int)OUTPUT_WIDTH || actual[1]!=(int)OUTPUT_HEIGHT){
            fprintf(stderr,"cpvc-output: REJECT KD=%dx%d expected=%ux%u\n",actual[0],actual[1],OUTPUT_WIDTH,OUTPUT_HEIGHT);goto cleanup;
        }
    }
    surface = eglCreateWindowSurface(display, config, native_window, NULL);
    if (surface == EGL_NO_SURFACE || !eglBindAPI(EGL_OPENGL_ES_API)) goto cleanup;
    if(cpvc_check_surface(display,surface)!=0)goto cleanup;
    context = eglCreateContext(display, config, EGL_NO_CONTEXT,
                               context_attributes);
    if (context == EGL_NO_CONTEXT ||
        !eglMakeCurrent(display, surface, surface, context)) goto cleanup;

    glViewport(0,0,OUTPUT_WIDTH,OUTPUT_HEIGHT);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    if(!eglSwapBuffers(display,surface))goto cleanup;
    {
        int attempt,valid=0;
        for(attempt=0;attempt<5 && !valid;++attempt){
            valid=cpvc_query_displayable(1)==0;
            if(!valid)usleep(100000);
        }
        if(!valid){fprintf(stderr,"cpvc-output: REJECT DisplayManager registration missing/duplicate/wrong dimensions\n");goto cleanup;}
    }
    fprintf(stderr,"cpvc-output: VERIFIED KD/EGL/DisplayManager=%ux%u id=%u unique; canvas never changes encoder size\n",OUTPUT_WIDTH,OUTPUT_HEIGHT,CPVC_MIRROR_DISPLAYABLE);
    if(self_test){fprintf(stderr,"cpvc-output: SELFTEST_OK NO_MOST_ROUTE NO_PREFERENCES_WRITE\n");result=0;goto cleanup;}

    {
        EGLint min_interval = -1, max_interval = -1;
        EGLint requested_interval = 1;
        EGLBoolean interval_result;
        EGLint interval_error = EGL_SUCCESS;
        const char *interval_text = getenv("CARPLAY_EGL_SWAP_INTERVAL");
        if (interval_text && strcmp(interval_text, "0") == 0)
            requested_interval = 0;
        eglGetConfigAttrib(display, config, EGL_MIN_SWAP_INTERVAL,
                           &min_interval);
        eglGetConfigAttrib(display, config, EGL_MAX_SWAP_INTERVAL,
                           &max_interval);
        interval_result = eglSwapInterval(display, requested_interval);
        if (!interval_result) interval_error = eglGetError();
        fprintf(stderr,
                "nvss-most: EGL swap interval request=%d result=%d "
                "config_min=%d config_max=%d error=0x%x\n",
                requested_interval, (int)interval_result,
                (int)min_interval, (int)max_interval, (unsigned)interval_error);
    }

    program = create_program();
    if (!program) goto cleanup;
    glUseProgram(program);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(3, textures);
    configure_texture(textures[0], 0, glGetUniformLocation(program, "tex_y"),
                      shared_mode ? CARPLAY_YUV_WIDTH : 1,
                      shared_mode ? CARPLAY_YUV_HEIGHT : 1);
    configure_texture(textures[1], 1, glGetUniformLocation(program, "tex_u"),
                      shared_mode ? CARPLAY_YUV_WIDTH / 2u : 1,
                      shared_mode ? CARPLAY_YUV_HEIGHT / 2u : 1);
    configure_texture(textures[2], 2, glGetUniformLocation(program, "tex_v"),
                      shared_mode ? CARPLAY_YUV_WIDTH / 2u : 1,
                      shared_mode ? CARPLAY_YUV_HEIGHT / 2u : 1);
    if (shared_mode) {
        texture_width = CARPLAY_YUV_WIDTH;
        texture_height = CARPLAY_YUV_HEIGHT;
    }
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          vertices + 2);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glViewport(0, 0, OUTPUT_WIDTH, OUTPUT_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (report_gl_errors("initialization") ||
        !eglSwapBuffers(display, surface)) {
        fprintf(stderr, "nvss-most: initial swap failed EGL=0x%x\n",
                eglGetError());
        goto cleanup;
    }
    fprintf(stderr,
            "nvss-most: EGL %d.%d context3 ready; DMDT remains on stock map\n",
            major, minor);

    decoder_config.port = (uint16_t)port;
    decoder_config.layer = layer;
    decoder_config.decode_width = decode_width;
    decoder_config.decode_height = decode_height;
#ifdef CPVC_SHM_ONLY
    if (pthread_create(&decoder, NULL,shared_yuv_thread,NULL) != 0)
#else
    if (pthread_create(&decoder, NULL,
                       shared_mode ? shared_yuv_thread : decoder_thread,
                       shared_mode ? NULL : &decoder_config) != 0)
#endif
        goto cleanup;
    decoder_started = 1;
    report_start = monotonic_us();
    canvas_socket=cpvc_control_socket();
    if(canvas_socket<0){fprintf(stderr,"cpvc-canvas: loopback control unavailable\n");goto cleanup;}

    while (g_running && !g_session_done) {
        int have_frame = 0;
        int redraw_last_frame = 0;
        uint32_t frame_width = 0, frame_height = 0;
        if(monotonic_us()>=policy_check_us){
            policy_check_us=monotonic_us()+250000ULL;
            cpmap_limit_fd(STDERR_FILENO, CPMAP_BRIDGE_LOG_LIMIT);
            if(getppid()!=owner_pid || !cpvc_mirror_allowed()){fprintf(stderr,"cpvc-mirror: owner or permission off, stopping before OEM restore\n");break;}
        }
        if(cpvc_poll_canvas(canvas_socket))canvas_changed=1;
        if(canvas_changed){
            if(cpvc_canvas_read(cpvc_state_dir(),&canvas)<0){fprintf(stderr,"cpvc-canvas: invalid state, retaining last viewport\n");}
            canvas_rect=cpvc_canvas_rect((int)OUTPUT_WIDTH,(int)OUTPUT_HEIGHT,CARPLAY_YUV_WIDTH,CARPLAY_YUV_HEIGHT,canvas.percent);
            glViewport(canvas_rect.x,canvas_rect.y,canvas_rect.width,canvas_rect.height);
            fprintf(stderr,"cpvc-canvas: percent=%d viewport=%d,%d,%d,%d generation=%llu\n",canvas.percent,canvas_rect.x,canvas_rect.y,canvas_rect.width,canvas_rect.height,canvas.generation);
            canvas_changed=0;canvas_redraw=1;
        }
        pthread_mutex_lock(&g_frames.mutex);
        if (g_frames.sequence != uploaded_sequence) {
            const uint8_t *frame = g_frames.slots[g_frames.front];
            uint32_t chroma_width;
            uint32_t chroma_height;
            size_t y_size;
            size_t uv_size;
            frame_width = g_frames.width;
            frame_height = g_frames.height;
            chroma_width = (frame_width + 1u) / 2u;
            chroma_height = (frame_height + 1u) / 2u;
            y_size = (size_t)frame_width * frame_height;
            uv_size = (size_t)chroma_width * chroma_height;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            if (texture_width != frame_width || texture_height != frame_height)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width,
                             frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                             frame);
            else
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width,
                                frame_height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                frame);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            if (texture_width != frame_width || texture_height != frame_height)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, chroma_width,
                             chroma_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                             frame + y_size);
            else
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width,
                                chroma_height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                frame + y_size);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textures[2]);
            if (texture_width != frame_width || texture_height != frame_height)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, chroma_width,
                             chroma_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                             frame + y_size + uv_size);
            else
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width,
                                chroma_height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                frame + y_size + uv_size);
            texture_width = frame_width;
            texture_height = frame_height;
            last_frame_timestamp_us = g_frames.timestamp_us;
            uploaded_sequence = g_frames.sequence;
            have_frame = report_gl_errors("YUV upload") ? 0 : 1;
        }
        pthread_mutex_unlock(&g_frames.mutex);

        if (context_active && !g_stream_active) {
            switch_context(33);
            context_active = 0;
            post_switch_latch_pending = 0;
        }

        /* One extra swap after DMDT switches to context 3 is enough to latch
         * the already-rendered first frame.  Continuously swapping the same
         * frame used to generate about 40 swaps/s from only 15 new frames/s,
         * which can build latency in the DisplayManager/MOST queue. */
        redraw_last_frame = context_active && post_switch_latch_pending &&
                            uploaded_sequence != 0;
        if (have_frame || redraw_last_frame || (canvas_redraw && uploaded_sequence)) {
            GLubyte sample[4];
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(program);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            if(canvas_dumped_generation!=canvas.generation){cpvc_dump_frame(OUTPUT_WIDTH,OUTPUT_HEIGHT,canvas.generation);canvas_dumped_generation=canvas.generation;}
            canvas_redraw=0;
            if (!framebuffer_sampled) {
                glReadPixels(OUTPUT_WIDTH / 2u, OUTPUT_HEIGHT / 2u,
                             1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sample);
                fprintf(stderr,
                        "nvss-most: framebuffer center RGBA=%u,%u,%u,%u\n",
                        (unsigned int)sample[0], (unsigned int)sample[1],
                        (unsigned int)sample[2], (unsigned int)sample[3]);
                framebuffer_sampled = 1;
            }
            {
                uint64_t swap_started_us;
                uint64_t swap_finished_us;
                if (report_gl_errors("draw")) break;
                swap_started_us = monotonic_us();
                if (!eglSwapBuffers(display, surface)) {
                    fprintf(stderr,
                            "nvss-most: frame swap failed EGL=0x%x\n",
                            eglGetError());
                    break;
                }
                swap_finished_us = monotonic_us();
                swap_last_us = swap_finished_us - swap_started_us;
                swap_total_us += swap_last_us;
                ++swap_samples;
                if (swap_last_us < swap_min_us) swap_min_us = swap_last_us;
                if (swap_last_us > swap_max_us) swap_max_us = swap_last_us;
            }
            if (have_frame && last_frame_timestamp_us) {
                uint64_t rendered_us = monotonic_us();
                if (rendered_us >= last_frame_timestamp_us) {
                    latency_last_us = rendered_us - last_frame_timestamp_us;
                    latency_total_us += latency_last_us;
                    ++latency_samples;
                    if (latency_last_us < latency_min_us)
                        latency_min_us = latency_last_us;
                    if (latency_last_us > latency_max_us)
                        latency_max_us = latency_last_us;
                }
            }
            ++render_count;
            if (redraw_last_frame) post_switch_latch_pending = 0;
            if (!context_active && g_stream_active) {
                /* The context already contains a complete frame before MOST
                 * is redirected, avoiding a visible uninitialized window. */
                if(cpvc_check_surface(display,surface)!=0 || cpvc_query_displayable(1)!=0){
                    fprintf(stderr,"cpvc-output: REJECT before activation; retaining OEM map\n");output_failed=1;break;
                }
                context_switch_attempted = 1;
                if (switch_context(CPVC_MIRROR_DISPLAYABLE) == 0) {
                    context_active = 1;
                    post_switch_latch_pending = 1;
                } else {output_failed=1;break;}
            }
        } else {
            usleep(1000);
        }
        if (context_active && monotonic_us() - report_start >= 1000000ULL) {
            fprintf(stderr,
                    "carplay-mirror: render=%llu latest_sequence=%llu "
                    "source=%ux%u sender=%d latency_ms=%llu "
                    "avg_ms=%llu min_ms=%llu max_ms=%llu samples=%llu "
                    "swap_ms=%llu swap_avg_ms=%llu swap_min_ms=%llu "
                    "swap_max_ms=%llu swap_samples=%llu\n",
                    (unsigned long long)render_count,
                    (unsigned long long)uploaded_sequence,
                    texture_width, texture_height,
                    (int)g_sender_connected,
                    (unsigned long long)(latency_last_us / 1000ULL),
                    (unsigned long long)(latency_samples ?
                        latency_total_us / latency_samples / 1000ULL : 0),
                    (unsigned long long)(latency_samples ?
                        latency_min_us / 1000ULL : 0),
                    (unsigned long long)(latency_samples ?
                        latency_max_us / 1000ULL : 0),
                    (unsigned long long)latency_samples,
                    (unsigned long long)(swap_last_us / 1000ULL),
                    (unsigned long long)(swap_samples ?
                        swap_total_us / swap_samples / 1000ULL : 0),
                    (unsigned long long)(swap_samples ?
                        swap_min_us / 1000ULL : 0),
                    (unsigned long long)(swap_samples ?
                        swap_max_us / 1000ULL : 0),
                    (unsigned long long)swap_samples);
            render_count = 0;
            latency_total_us = 0;
            latency_min_us = UINT64_MAX;
            latency_max_us = 0;
            latency_samples = 0;
            swap_total_us = 0;
            swap_min_us = UINT64_MAX;
            swap_max_us = 0;
            swap_samples = 0;
            report_start = monotonic_us();
        }
    }
    result = output_failed ? 1 : 0;

cleanup:
    g_running = 0;
    if(canvas_socket>=0)close(canvas_socket);
    fprintf(stderr,"cpvc-cleanup: decoder-join-start\n");
    if (decoder_started) pthread_join(decoder, NULL);
    fprintf(stderr,"cpvc-cleanup: decoder-joined\n");
    /* Restore even if only one of the two activation commands succeeded. */
    fprintf(stderr,"cpvc-cleanup: restore-context-start\n");
    if (context_active || context_switch_attempted) switch_context(33);
    fprintf(stderr,"cpvc-cleanup: restore-context-end\n");
    if (textures[0]) glDeleteTextures(3, textures);
    if (program) glDeleteProgram(program);
    if (display != EGL_NO_DISPLAY) {
        fprintf(stderr,"cpvc-cleanup: release-current\n");
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        fprintf(stderr,"cpvc-cleanup: destroy-context\n");
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        fprintf(stderr,"cpvc-cleanup: destroy-surface\n");
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        if(kd_window && display_destroy_window){
            fprintf(stderr,"cpvc-cleanup: destroy-own-native-window id=%u\n",CPVC_MIRROR_DISPLAYABLE);
            display_destroy_window(kd_window);kd_window=0;
        }
        fprintf(stderr,"cpvc-cleanup: terminate-egl\n");
        eglTerminate(display);
    }
    /* This is a standalone context-3 client, not the display server owner.
     * Its GL context/surface and EGL session were released above. The legacy
     * display_deinit helper tears down the process-wide native drawable and
     * can block indefinitely after an independent context window is closed
     * (reproduced on the firmware VM). End this process instead; QNX closes
     * its remaining native client session. Never call this path from HMI.
     */
    fprintf(stderr,"cpvc-cleanup: own-egl-released global-display-helper-skipped\n");
    free(g_frames.slots[0]);
    free(g_frames.slots[1]);
    fprintf(stderr, "nvss-most: exit result=%d\n", result);
    fflush(stderr);
    _exit(result);
}
