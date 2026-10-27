#ifndef CPMAP_ANIMATION_PREFERENCE_H
#define CPMAP_ANIMATION_PREFERENCE_H
#include "cpmap_map_preference.h"
#ifndef CPMAP_ANIMATION_STATE_DIR
#define CPMAP_ANIMATION_STATE_DIR "/mnt/icab/persistent/cpmap-ui"
#endif
/* Java AnimationSettings wire format. Return 0 only if no saved state exists. */
static int cpmap_animation_file(const char *path,int *mode){
 char data[193],canonical[193],body[128];struct stat st;FILE *f;size_t n;int used=0,m,k;unsigned crc;unsigned long long gen;
 if(lstat(path,&st))return errno==ENOENT?0:-1;
 if(!S_ISREG(st.st_mode)||st.st_size<1||st.st_size>192)return -1;
 f=fopen(path,"rb");if(!f)return -1;n=fread(data,1,192,f);k=ferror(f);if(fclose(f)||k||n!=(size_t)st.st_size||memchr(data,0,n))return -1;data[n]=0;
 if(sscanf(data,"format=1\nmode=%d\ngeneration=%llu\ncrc32=%x\n%n",&m,&gen,&crc,&used)!=3||used!=(int)n||m<0||m>2||gen<1||gen>0x7fffffffffffffffULL)return -1;
 k=snprintf(body,sizeof(body),"format=1\nmode=%d\ngeneration=%llu\n",m,gen);
 if(k<0||(size_t)k>=sizeof(body)||cpmap_preference_crc((const unsigned char *)body,(size_t)k)!=crc)return -1;
 k=snprintf(canonical,sizeof(canonical),"%scrc32=%x\n",body,crc);
 if(k!=(int)n||memcmp(data,canonical,n))return -1;
 *mode=m;return 1;
}
static int cpmap_animation_read(const char *root,int *mode){
 char path[512];int a,b;
 if(snprintf(path,sizeof(path),"%s/animation.state",root)>=(int)sizeof(path))return -1;
 a=cpmap_animation_file(path,mode);if(a==1)return 1;
 if(snprintf(path,sizeof(path),"%s/animation.previous",root)>=(int)sizeof(path))return -1;
 b=cpmap_animation_file(path,mode);if(b==1)return 2;
 return a==0&&b==0?0:-1;
}
#endif
