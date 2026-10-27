#ifndef CPVC_MIRROR_CONTROL_H
#define CPVC_MIRROR_CONTROL_H
#include "cpvc_canvas.h"
#include "../runtime/cpmap_projection_preferences.h"
#include "../runtime/cpvc_authorized_runtime.h"
#include "../runtime/cpmap_resources.h"
static const char *cpvc_state_dir(void){return CPVC_PREF_DIR;}
static int cpvc_mirror_allowed(void){
 struct cpmap_projection_preferences p;int rc=cpmap_read_projection_preferences(cpvc_state_dir(),&p);
 return cpvc_authorized_runtime() && rc>=0 && (p.mirror_mask&1)!=0 && (p.map_mask&1)==0;
}
static int cpvc_control_socket(void){
 int fd=socket(AF_INET,SOCK_DGRAM,0);struct sockaddr_in address;
 if(fd<0)return -1;memset(&address,0,sizeof(address));address.sin_family=AF_INET;
 address.sin_port=htons(7033);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(bind(fd,(struct sockaddr *)&address,sizeof(address)) || fcntl(fd,F_SETFL,O_NONBLOCK)<0){close(fd);return -1;}return fd;
}
static int cpvc_poll_canvas(int fd){
 char data[64];struct sockaddr_in from;socklen_t size=sizeof(from);ssize_t n;int changed=0;
 while((n=recvfrom(fd,data,sizeof(data)-1,0,(struct sockaddr *)&from,&size))>0){
  int rc;if(from.sin_addr.s_addr!=htonl(INADDR_LOOPBACK))continue;data[n]=0;
  rc=cpvc_canvas_command(cpvc_state_dir(),data,cpvc_mirror_allowed());
  fprintf(stderr,"cpvc-canvas: command=%s rc=%d phone_zoom=0\n",data,rc);if(rc==0)changed=1;
 }return changed;
}
static void cpvc_dump_frame(int width,int height,unsigned long long generation){
 const char *prefix=getenv("CPVC_FRAME_DUMP");unsigned char *pixels;FILE *f;char path[512];int y,x;
 if(!prefix || !*prefix)return;
 if(snprintf(path,sizeof(path),"%s-%llu.ppm",prefix,generation)>=(int)sizeof(path))return;
 /* GLES2 guarantees RGBA/UNSIGNED_BYTE readback, not RGB readback. */
 pixels=(unsigned char *)malloc((size_t)width*height*4);if(!pixels)return;
 glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
 if(glGetError()!=GL_NO_ERROR){free(pixels);return;}
 f=fopen(path,"wb");if(f){fprintf(f,"P6\n%d %d\n255\n",width,height);for(y=height-1;y>=0;y--)for(x=0;x<width;x++)fwrite(pixels+((size_t)y*width+x)*4,1,3,f);fclose(f);}
 free(pixels);
}
#endif
