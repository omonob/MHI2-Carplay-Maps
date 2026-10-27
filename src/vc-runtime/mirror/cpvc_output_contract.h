#ifndef CPVC_OUTPUT_CONTRACT_H
#define CPVC_OUTPUT_CONTRACT_H

#include <sys/wait.h>
#include <spawn.h>

/* KD/DisplayManager on MU1102 interprets only the first two caption digits.
 * Generic displayable 3 already exists at 800x480. Never reuse it, or pass
 * 790/791 and assume the server kept all three digits. */
#if CPVC_BUILD_PROFILE == 790
#define CPVC_MIRROR_DISPLAYABLE 79u
#elif CPVC_BUILD_PROFILE == 791
#define CPVC_MIRROR_DISPLAYABLE 78u
#else
#error Explicit instrument profile required
#endif
#define CPVC_OUTPUT_BUILD "CPVC_MIRROR_R3_NO_PERIODIC_QUERY_20261027"

static int cpvc_parse_displayables(const char *text, unsigned int expected_id,
                                  unsigned int width, unsigned int height,
                                  int must_be_present)
{
    unsigned int matches = 0, correct = 0;
    const char *line = text;
    if (!text || !strstr(text, "displaymanager knows")) return -1;
    while (*line) {
        unsigned int id = 0, w = 0, h = 0;
        char type[16];
        if (sscanf(line, " %u %15s %u x %u", &id, type, &w, &h) == 4 &&
            id == expected_id) {
            ++matches;
            if (strcmp(type,"window") == 0 && w == width && h == height)
                ++correct;
        }
        line = strchr(line, '\n');
        if (!line) break;
        ++line;
    }
    return must_be_present ? (matches == 1 && correct == 1 ? 0 : -1)
                           : (matches == 0 ? 0 : -1);
}

#ifndef CPVC_OUTPUT_PARSE_TEST
/* Query only: the unbuffer shim is loaded solely into the short-lived dmdt
 * child, never DisplayManager, HMI, or this renderer. QNX dmdt uses _exit,
 * hence normal non-TTY stdout loses its buffered output without this shim. */
static int cpvc_query_displayable(int must_be_present)
{
    const char *shim = getenv("CPVC_DMDT_QUERY_LIBRARY");
    char text[16384];
    char preload[640],libpath[2048];
    char *args[]={"dmdt","gd",NULL};
    char *child_env[]={preload,libpath,"IPL_CONFIG_DIR=/etc/eso/production",NULL};
    const char *inherited_libpath=getenv("LD_LIBRARY_PATH");
    posix_spawn_file_actions_t actions;
    size_t used = 0;
    int p[2], status = 0, finished = 0, eof = 0;
    uint64_t deadline;
    pid_t pid;
    if (!shim || !*shim) shim="/mnt/app/carplaymaps/libcpvc-dmdt-query.so";
    if (access(shim,R_OK) != 0 || pipe(p) != 0) return -1;
    if(snprintf(preload,sizeof(preload),"LD_PRELOAD=%s",shim)>=(int)sizeof(preload) ||
       snprintf(libpath,sizeof(libpath),"LD_LIBRARY_PATH=%s",inherited_libpath?inherited_libpath:"/lib:/eso/lib")>=(int)sizeof(libpath)) {
        close(p[0]);close(p[1]);return -1;
    }
    if(posix_spawn_file_actions_init(&actions)!=0){close(p[0]);close(p[1]);return -1;}
    status=posix_spawn_file_actions_addclose(&actions,p[0]);
    status|=posix_spawn_file_actions_adddup2(&actions,p[1],STDOUT_FILENO);
    status|=posix_spawn_file_actions_adddup2(&actions,p[1],STDERR_FILENO);
    status|=posix_spawn_file_actions_addclose(&actions,p[1]);
    if(status==0)status=posix_spawn(&pid,"/eso/bin/apps/dmdt",&actions,NULL,args,child_env);
    posix_spawn_file_actions_destroy(&actions);
    if(status!=0){close(p[0]);close(p[1]);return -1;}
    close(p[1]);
    fcntl(p[0],F_SETFL,O_NONBLOCK);
    deadline=monotonic_us()+2000000ULL;
    while (monotonic_us()<deadline && (!finished || !eof)) {
        ssize_t n=read(p[0],text+used,sizeof(text)-1-used);
        if (n>0) used+=(size_t)n;
        else if (n==0) eof=1;
        else if(errno!=EAGAIN && errno!=EINTR) break;
        if (!finished) finished=waitpid(pid,&status,WNOHANG)==pid;
        if (used==sizeof(text)-1) break;
        if(!finished || !eof)usleep(10000);
    }
    close(p[0]);
    if (!finished) {kill(pid,SIGKILL);waitpid(pid,&status,0);return -1;}
    if (!eof || !WIFEXITED(status) || WEXITSTATUS(status)!=0) return -1;
    text[used]=0;
    return cpvc_parse_displayables(text,CPVC_MIRROR_DISPLAYABLE,
                                  OUTPUT_WIDTH,OUTPUT_HEIGHT,must_be_present);
}

static int cpvc_check_surface(EGLDisplay display, EGLSurface surface)
{
    EGLint width=0,height=0;
    if (!eglQuerySurface(display,surface,EGL_WIDTH,&width) ||
        !eglQuerySurface(display,surface,EGL_HEIGHT,&height) ||
        width!=(EGLint)OUTPUT_WIDTH || height!=(EGLint)OUTPUT_HEIGHT) {
        fprintf(stderr,"cpvc-output: REJECT EGL=%dx%d expected=%ux%u no mirror route\n",
                width,height,OUTPUT_WIDTH,OUTPUT_HEIGHT);
        return -1;
    }
    return 0;
}
#endif
#endif
