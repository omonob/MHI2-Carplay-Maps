#ifndef CPMAP_RESOURCES_H
#define CPMAP_RESOURCES_H

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef CPMAP_CACHE_LOCK
#define CPMAP_CACHE_LOCK "/ramdisk/cpmap-video-cache.lock"
#endif
#define CPMAP_VIDEO_LOG_LIMIT (256U * 1024U)
#define CPMAP_BRIDGE_LOG_LIMIT (128U * 1024U)
#define CPMAP_SMALL_LOG_LIMIT (64U * 1024U)

/* Truncate the same inode, never rotate an open writer into an unlinked
 * file. Every producer opens O_APPEND; flushing precedes size checks. */
static inline void cpmap_limit_fd(int fd, size_t limit)
{
    struct stat st;
    int saved = errno;
    if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
        st.st_size > (off_t)limit) {
        if (ftruncate(fd, 0) == 0) (void)lseek(fd, 0, SEEK_SET);
    }
    errno = saved;
}

static inline void cpmap_limit_path(const char *path, size_t limit)
{
    int saved = errno;
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd >= 0) { cpmap_limit_fd(fd, limit); close(fd); }
    errno = saved;
}

static pthread_mutex_t cpmap_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline FILE *cpmap_log_open(const char *path, size_t limit)
{
    FILE *file;
    pthread_mutex_lock(&cpmap_log_mutex);
    file = fopen(path, "a");
    if (!file) { pthread_mutex_unlock(&cpmap_log_mutex); return NULL; }
    cpmap_limit_fd(fileno(file), limit);
    return file;
}

static inline void cpmap_log_close(FILE *file, size_t limit)
{
    fflush(file);
    cpmap_limit_fd(fileno(file), limit);
    fclose(file);
    pthread_mutex_unlock(&cpmap_log_mutex);
}

static inline int cpmap_log_fprintf(FILE *file, size_t limit,
                                  const char *format, ...)
{
    int rc, flags;
    va_list ap;
    int saved = errno;
    pthread_mutex_lock(&cpmap_log_mutex);
    flags = fcntl(fileno(file), F_GETFL);
    if (flags >= 0) (void)fcntl(fileno(file), F_SETFL, flags | O_APPEND);
    cpmap_limit_fd(fileno(file), limit);
    va_start(ap, format);
    rc = vfprintf(file, format, ap);
    va_end(ap);
    fflush(file);
    cpmap_limit_fd(fileno(file), limit);
    pthread_mutex_unlock(&cpmap_log_mutex);
    errno = saved;
    return rc;
}

/* POSIX locks are per process, not per descriptor/thread. Keep exactly one
 * descriptor and reference count all listener threads. Do not unlink this
 * tiny lock file: replacing its inode would split the locking domain. */
static pthread_mutex_t cpmap_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static int cpmap_cache_fd = -1;
static unsigned int cpmap_cache_users;

static inline int cpmap_cache_acquire(void)
{
    struct flock lock;
    int rc = -1;
    pthread_mutex_lock(&cpmap_cache_mutex);
    if (cpmap_cache_users) { ++cpmap_cache_users; rc = 0; goto done; }
    cpmap_cache_fd = open(CPMAP_CACHE_LOCK, O_RDWR | O_CREAT, 0600);
    if (cpmap_cache_fd < 0) goto done;
    (void)fcntl(cpmap_cache_fd, F_SETFD, FD_CLOEXEC);
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    do { rc = fcntl(cpmap_cache_fd, F_SETLKW, &lock); }
    while (rc < 0 && errno == EINTR);
    if (rc == 0) cpmap_cache_users = 1;
    else { close(cpmap_cache_fd); cpmap_cache_fd = -1; }
done:
    pthread_mutex_unlock(&cpmap_cache_mutex);
    return rc;
}

static inline void cpmap_cache_release(void)
{
    pthread_mutex_lock(&cpmap_cache_mutex);
    if (cpmap_cache_users && --cpmap_cache_users == 0) {
        close(cpmap_cache_fd); /* last close releases the process lease */
        cpmap_cache_fd = -1;
    }
    pthread_mutex_unlock(&cpmap_cache_mutex);
}

/* Only the supervisor uses exclusive cleanup. A new listener or a surviving
 * MOST reader blocks this nonblocking request; cleanup must then be deferred. */
static inline int cpmap_cache_cleanup_lock(void)
{
    struct flock lock;
    int saved, fd = open(CPMAP_CACHE_LOCK, O_RDWR | O_CREAT, 0600);
    if (fd < 0) return -1;
    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    if (fcntl(fd, F_SETLK, &lock) == 0) return fd;
    saved = errno;
    close(fd);
    errno = saved;
    return -1;
}
#endif
