#ifndef CPMAP_CARD_PREFERENCES_H
#define CPMAP_CARD_PREFERENCES_H
/* Wire-identical to Java CardPositionSettings. Reader only; never writes UI state. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
enum {CPMAP_CARD_CENTER=0,CPMAP_CARD_LEFT=1,CPMAP_CARD_RIGHT=2,CPMAP_CARD_OFF=3};
struct cpmap_card_preference {int position;uint64_t generation;int source;};
static uint32_t cpmap_card_crc(const unsigned char *s,size_t n){
 uint32_t crc=0xffffffffu;size_t i;int bit;
 for(i=0;i<n;i++){crc^=s[i];for(bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(uint32_t)-(int)(crc&1));}
 return crc^0xffffffffu;
}
static int cpmap_card_file(const char *path,struct cpmap_card_preference *out){
 struct stat st;char data[193],body[128],canonical[193];FILE *f;size_t n;int position,used=0,len,bad;
 unsigned long long gen;unsigned crc;
 if(lstat(path,&st))return errno==ENOENT?0:-1;
 if(!S_ISREG(st.st_mode)||st.st_size<1||st.st_size>192)return -1;
 f=fopen(path,"rb");if(!f)return -1;n=fread(data,1,192,f);bad=ferror(f);if(fclose(f)||bad||n!=(size_t)st.st_size||memchr(data,0,n))return -1;data[n]=0;
 if(sscanf(data,"format=1\nposition=%d\ngeneration=%llu\ncrc32=%x\n%n",&position,&gen,&crc,&used)!=3||used!=(int)n||position<0||position>3||gen<1||gen>0x7fffffffffffffffULL)return -1;
 len=snprintf(body,sizeof(body),"format=1\nposition=%d\ngeneration=%llu\n",position,gen);
 if(len<0||(size_t)len>=sizeof(body)||cpmap_card_crc((const unsigned char *)body,(size_t)len)!=crc)return -1;
 len=snprintf(canonical,sizeof(canonical),"%scrc32=%x\n",body,crc);
 if(len!=(int)n||memcmp(data,canonical,n))return -1;
 out->position=position;out->generation=gen;return 1;
}
/* 1 primary, 2 recovered, 0 no preference yet (accepted existing RIGHT), -1 bad. */
static int cpmap_card_read(const char *root,struct cpmap_card_preference *out){
 char path[512];int a,b;
 if(!root||!out||snprintf(path,sizeof(path),"%s/navigation-card.state",root)>=(int)sizeof(path))return -1;
 a=cpmap_card_file(path,out);if(a==1){out->source=1;return 1;}
 if(snprintf(path,sizeof(path),"%s/navigation-card.previous",root)>=(int)sizeof(path))return -1;
 b=cpmap_card_file(path,out);if(b==1){out->source=2;return 2;}
 if(a||b)return -1;
 out->position=CPMAP_CARD_RIGHT;out->generation=0;out->source=0;return 0;
}
#endif
