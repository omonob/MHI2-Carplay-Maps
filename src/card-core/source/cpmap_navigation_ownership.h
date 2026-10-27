#ifndef CPMAP_NAVIGATION_OWNERSHIP_H
#define CPMAP_NAVIGATION_OWNERSHIP_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cpmap_card_preferences.h"
/* RAM-only ownership flag, not private navigation metadata. Bind it to the
 * current dio_manager process so an old receiver cannot steer a new one. */
static int cpmap_read_navigation_ownership(const char *path,long owner,int *active){
 FILE *f;char data[193],body[128],canonical[193];size_t n;long pid;int state,used=-1,k;unsigned crc;unsigned long long gen;
 if(!active||owner<2)return 0;
 f=fopen(path,"rb");if(!f)return 0;
 n=fread(data,1,192,f);if(ferror(f)||fgetc(f)!=EOF){fclose(f);return 0;}fclose(f);
 if(!n||memchr(data,0,n))return 0;
 data[n]=0;
 if(sscanf(data,"format=1\nowner_pid=%ld\nactive=%d\ngeneration=%llu\ncrc32=%8x\n%n",&pid,&state,&gen,&crc,&used)!=4||used!=(int)n||pid!=owner||(state!=0&&state!=1)||!gen||gen>0x7fffffffffffffffULL)return 0;
 k=snprintf(body,sizeof(body),"format=1\nowner_pid=%ld\nactive=%d\ngeneration=%llu\n",pid,state,gen);
 if(k<0||(size_t)k>=sizeof(body)||cpmap_card_crc((const unsigned char *)body,(size_t)k)!=crc)return 0;
 k=snprintf(canonical,sizeof(canonical),"%scrc32=%x\n",body,crc);
 if(k!=(int)n||memcmp(data,canonical,n))return 0;
 *active=state;return 1;
}
#endif
