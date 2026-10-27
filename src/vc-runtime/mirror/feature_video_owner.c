/* Offline integration candidate: one video owner; metadata is never stopped. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include "../integration/cpmap_projection_preferences.h"
static volatile sig_atomic_t running=1;
static void stop(int sig){(void)sig;running=0;}
static unsigned long long ms(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (unsigned long long)t.tv_sec*1000+t.tv_nsec/1000000;}
static void pause_poll(void){struct timespec t={0,100000000};nanosleep(&t,0);}
static int desired(const char *state){struct cpmap_projection_preferences p;int rc=cpmap_read_projection_preferences(state,&p);if(rc<0)return 0;if(p.map_mask&1)return 1;if(p.mirror_mask&1)return 2;return 0;}
static int stop_child(pid_t *child){
 unsigned long long end;int status;pid_t r;
 if(*child<1)return 0;
 r=waitpid(*child,&status,WNOHANG);if(r==*child){*child=-1;return WIFEXITED(status)&&WEXITSTATUS(status)==0?0:-1;}
 if(r<0)return -1;
 if(kill(*child,SIGTERM) && errno!=ESRCH)return -1;
 end=ms()+15000;
 do{r=waitpid(*child,&status,WNOHANG);if(r==*child){fprintf(stderr,"VIDEO_OWNER child_stopped pid=%ld status=%d\n",(long)*child,status);*child=-1;return WIFEXITED(status)&&WEXITSTATUS(status)==0?0:-1;}if(r<0)return -1;pause_poll();}while(ms()<end);
 fprintf(stderr,"VIDEO_OWNER stop_timeout no_new_owner=1 pid=%ld\n",(long)*child);return -1;
}
static pid_t start_child(const char *root,int mode,int profile,const char *state){
 char path[512],number[8];pid_t pid;int n;
 n=mode==1?snprintf(path,sizeof(path),"%s/carplay-cluster-supervisor.menu-test",root):snprintf(path,sizeof(path),"%s/cpvc-mirror-canvas-%d",root,profile);
 if(n<0 || (size_t)n>=sizeof(path) || access(path,X_OK))return -1;
 pid=fork();if(pid!=0)return pid;
 snprintf(number,sizeof(number),"%d",profile);setenv("CPVC_CLUSTER_PROFILE",number,1);setenv("CPVC_STATE_DIR",state,1);
 if(mode==1)execl(path,path,(char *)0);else execl(path,path,"shm",(char *)0);
 _exit(127);
}
int main(int argc,char **argv){
 int profile,duration,lockfd,active=0,status,failed=0;pid_t child=-1,r;char lockpath[512];struct flock lock;unsigned long long end,retry=0;
 if(argc!=6 || strcmp(argv[1],"--offline"))return 2;
 profile=atoi(argv[3]);duration=atoi(argv[5]);if((profile!=790 && profile!=791)||duration<1||duration>180||argv[2][0]!='/'||argv[4][0]!='/')return 2;
 if(snprintf(lockpath,sizeof(lockpath),"%s/video-owner.lock",argv[4])>=(int)sizeof(lockpath))return 2;
 lockfd=open(lockpath,O_CREAT|O_RDWR,0600);if(lockfd<0)return 3;
 memset(&lock,0,sizeof(lock));lock.l_type=F_WRLCK;lock.l_whence=SEEK_SET;
 if(fcntl(lockfd,F_SETLK,&lock)){close(lockfd);return 4;}fcntl(lockfd,F_SETFD,FD_CLOEXEC);
 signal(SIGTERM,stop);signal(SIGINT,stop);end=ms()+(unsigned)duration*1000;
 fprintf(stderr,"VIDEO_OWNER offline=1 profile=%d metadata_process_actions=0\n",profile);
 while(running && ms()<end){
  int want=desired(argv[4]);
  if(child>0){r=waitpid(child,&status,WNOHANG);if(r==child){fprintf(stderr,"VIDEO_OWNER child_exit mode=%d status=%d\n",active,status);child=-1;active=0;if(!WIFEXITED(status)||WEXITSTATUS(status)!=0){failed=1;break;}retry=ms()+1000;}}
  if(want!=active && child>0){if(stop_child(&child)){failed=1;break;}active=0;retry=0;}
  if(want && !active && ms()>=retry){child=start_child(argv[2],want,profile,argv[4]);if(child>0){active=want;fprintf(stderr,"VIDEO_OWNER started mode=%d pid=%ld\n",active,(long)child);}else{fprintf(stderr,"VIDEO_OWNER missing_engine mode=%d\n",want);retry=ms()+1000;}}
  pause_poll();
 }
 if(stop_child(&child))failed=1;
 close(lockfd);fprintf(stderr,"VIDEO_OWNER exit failed=%d metadata_process_actions=0\n",failed);return failed?5:0;
}
