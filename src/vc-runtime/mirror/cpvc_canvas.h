#ifndef CPVC_CANVAS_H
#define CPVC_CANVAS_H
/* Test-branch canvas state; no phone/map commands and no radio operations. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

struct cpvc_canvas { int percent; unsigned long long generation; };
struct cpvc_rect { int x,y,width,height; };
static uint32_t cpvc_canvas_crc(const char *data,size_t length){
 uint32_t crc=0xffffffffu;size_t i;int bit;
 for(i=0;i<length;i++){crc^=(unsigned char)data[i];for(bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(uint32_t)-(int)(crc&1u));}
 return crc^0xffffffffu;
}
static int cpvc_canvas_path(char *path,size_t size,const char *root,const char *name){
 int n=snprintf(path,size,"%s/%s",root,name);return n<0 || (size_t)n>=size?-1:0;
}
static int cpvc_canvas_decode(const char *path,struct cpvc_canvas *out){
 char data[193],body[128],canonical[193];FILE *f;size_t n;int percent,used=-1,k;unsigned long long gen;unsigned crc;
 f=fopen(path,"rb");if(!f)return errno==ENOENT?0:-1;
 n=fread(data,1,192,f);if(ferror(f)||fgetc(f)!=EOF){fclose(f);return -1;}fclose(f);
 if(memchr(data,0,n))return -1;data[n]=0;
 if(sscanf(data,"format=1\npercent=%d\ngeneration=%llu\ncrc32=%8x\n%n",&percent,&gen,&crc,&used)!=3 || used!=(int)n)return -1;
 if(percent<50 || percent>150 || percent%5 || gen<1 || gen>0x7fffffffffffffffULL)return -1;
 k=snprintf(body,sizeof(body),"format=1\npercent=%d\ngeneration=%llu\n",percent,gen);
 if(k<0 || (size_t)k>=sizeof(body) || cpvc_canvas_crc(body,(size_t)k)!=crc)return -1;
 k=snprintf(canonical,sizeof(canonical),"%scrc32=%x\n",body,crc);
 if(k!=(int)n || memcmp(canonical,data,n))return -1;
 out->percent=percent;out->generation=gen;return 1;
}
static int cpvc_canvas_read(const char *root,struct cpvc_canvas *out){
 char path[512];int a,b;out->percent=100;out->generation=0;
 if(cpvc_canvas_path(path,sizeof(path),root,"canvas.state"))return -1;
 a=cpvc_canvas_decode(path,out);if(a==1)return 1;
 if(cpvc_canvas_path(path,sizeof(path),root,"canvas.previous"))return -1;
 b=cpvc_canvas_decode(path,out);if(b==1)return 2;
 return a==0 && b==0?0:-1;
}
static struct cpvc_rect cpvc_canvas_rect(int ow,int oh,int sw,int sh,int percent){
 struct cpvc_rect r={0,0,0,0};int bw,bh;
 if(ow<1 || ow>1920 || oh<1 || oh>1080 || sw<1 || sw>1920 || sh<1 || sh>1080 || percent<50 || percent>150)return r;
 if(ow*sh<=oh*sw){bw=ow;bh=sh*ow/sw;}else{bh=oh;bw=sw*oh/sh;}
 r.width=bw*percent/100;r.height=bh*percent/100;r.x=(ow-r.width)/2;r.y=(oh-r.height)/2;return r;
}
static int cpvc_canvas_set(const char *root,int percent){
 struct cpvc_canvas old,check;char pending[512],current[512],previous[512],body[128],encoded[193];FILE *f;int n,fd,main_valid;
 if(percent<50 || percent>150 || percent%5 || cpvc_canvas_read(root,&old)<0 || old.generation==0x7fffffffffffffffULL)return -1;
 if(old.percent==percent && old.generation)return 0;
 if(cpvc_canvas_path(pending,sizeof(pending),root,"canvas.pending") || cpvc_canvas_path(current,sizeof(current),root,"canvas.state") || cpvc_canvas_path(previous,sizeof(previous),root,"canvas.previous"))return -1;
 n=snprintf(body,sizeof(body),"format=1\npercent=%d\ngeneration=%llu\n",percent,old.generation+1);
 if(n<0 || (size_t)n>=sizeof(body))return -1;
 n=snprintf(encoded,sizeof(encoded),"%scrc32=%x\n",body,cpvc_canvas_crc(body,(size_t)n));
 if(n<0 || (size_t)n>=sizeof(encoded))return -1;
 f=fopen(pending,"wb");if(!f)return -1;fd=fileno(f);
 if(fwrite(encoded,1,(size_t)n,f)!=(size_t)n || fflush(f) || fsync(fd)){fclose(f);return -1;}
 if(fclose(f) || cpvc_canvas_decode(pending,&check)!=1 || check.percent!=percent || check.generation!=old.generation+1)return -1;
 main_valid=cpvc_canvas_decode(current,&check)==1;
 if(main_valid){if(unlink(previous) && errno!=ENOENT)return -1;if(rename(current,previous))return -1;}
 else if(unlink(current) && errno!=ENOENT)return -1;
 if(rename(pending,current))return -1;
 return cpvc_canvas_read(root,&check)>0 && check.percent==percent && check.generation==old.generation+1?0:-1;
}
static int cpvc_canvas_command(const char *root,const char *command,int mirror_allowed){
 struct cpvc_canvas state;int percent;
 if(!mirror_allowed)return -3;
 if(cpvc_canvas_read(root,&state)<0)return -1;percent=state.percent;
 if(!strcmp(command,"canvas-zoom-in")){if(percent<150)percent+=5;}
 else if(!strcmp(command,"canvas-zoom-out")){if(percent>50)percent-=5;}
 else if(!strcmp(command,"canvas-reset"))percent=100;
 else return -2;
 return cpvc_canvas_set(root,percent);
}
#endif
