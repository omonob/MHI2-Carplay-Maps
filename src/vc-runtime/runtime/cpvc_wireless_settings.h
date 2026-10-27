#ifndef CPVC_WIRELESS_SETTINGS_H
#define CPVC_WIRELESS_SETTINGS_H
/* Read-only policy interface. No radio/service or connection side effects. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define CPVC_WIRELESS_DEFAULT_DIR "/mnt/icab/persistent/cpmap-ui"
struct cpvc_wireless_settings { int enabled; unsigned long long generation; int source; };
/* source: 0 default, 1 primary, 2 previous. rc: 0 valid, 1 absent/default,
 * -1 damaged or unreadable. On error enabled is always false. */
static uint32_t cpvc_wireless_crc32(const unsigned char *data,size_t length){
 uint32_t crc=0xffffffffu;size_t i;int bit;
 for(i=0;i<length;i++){crc^=data[i];for(bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u & (uint32_t)-(int)(crc&1u));}
 return crc^0xffffffffu;
}
static int cpvc_wireless_read_file(const char *path,struct cpvc_wireless_settings *out){
 FILE *file;char data[193],body[128],expected[193];size_t count;int enabled,used=-1,size;
 unsigned long long generation;unsigned int crc;
 file=fopen(path,"rb");if(!file)return errno==ENOENT?-2:-1;
 count=fread(data,1,192,file);
 if(ferror(file) || fgetc(file)!=EOF){fclose(file);return -1;}fclose(file);
 if(memchr(data,0,count)!=NULL)return -1;
 data[count]=0;
 if(sscanf(data,"format=1\nenabled=%d\ngeneration=%llu\ncrc32=%8x\n%n",&enabled,&generation,&crc,&used)!=3 || used!=(int)count)return -1;
 if((enabled!=0 && enabled!=1) || generation==0 || generation>0x7fffffffffffffffULL)return -1;
 size=snprintf(body,sizeof(body),"format=1\nenabled=%d\ngeneration=%llu\n",enabled,generation);
 if(size<0 || size>=(int)sizeof(body) || cpvc_wireless_crc32((const unsigned char *)body,(size_t)size)!=(uint32_t)crc)return -1;
 size=snprintf(expected,sizeof(expected),"%scrc32=%x\n",body,crc);
 if(size!=(int)count || strcmp(expected,data)!=0)return -1;
 out->enabled=enabled;out->generation=generation;return 0;
}
static int cpvc_wireless_read(const char *directory,struct cpvc_wireless_settings *out){
 char path[512];int primary,previous,size;
 if(!out)return -1;
 out->enabled=0;out->generation=0;out->source=0;
 if(!directory)directory=CPVC_WIRELESS_DEFAULT_DIR;
 size=snprintf(path,sizeof(path),"%s/wireless-carplay.state",directory);
 if(size<0 || size>=(int)sizeof(path))return -1;
 primary=cpvc_wireless_read_file(path,out);if(primary==0){out->source=1;return 0;}
 size=snprintf(path,sizeof(path),"%s/wireless-carplay.previous",directory);
 if(size<0 || size>=(int)sizeof(path))return -1;
 previous=cpvc_wireless_read_file(path,out);if(previous==0){out->source=2;return 0;}
 return primary==-2 && previous==-2?1:-1;
}
#endif
