#ifndef CPMAP_PROJECTION_PREFERENCES_H
#define CPMAP_PROJECTION_PREFERENCES_H
/* Read-only schema3 consumer. CP-only legacy fields remain explicitly CP-only. */
#include "cpmap_map_preference.h"
struct cpmap_projection_preferences {
    int map,mirror,information,carlife;
    uint64_t generation;
    int source;
    int map_mask,mirror_mask;
};
static int cpmap_projection_file(const char *path,struct cpmap_projection_preferences *out)
{
    struct stat st;FILE *file;char buf[257],canonical[257];
    unsigned long long generation=0;int map=-1,mirror=-1,info=-1,cl=0,used=0,length,error,closed,old;
    size_t n,body;
    if(lstat(path,&st)!=0)return errno==ENOENT?0:-1;
    if(!S_ISREG(st.st_mode)||st.st_size<=0||st.st_size>256)return -1;
    file=fopen(path,"rb");if(!file)return -1;
    n=fread(buf,1,sizeof(buf),file);error=ferror(file);closed=fclose(file);
    if(error||closed||n!=(size_t)st.st_size||n>256||memchr(buf,0,n))return -1;
    buf[n]=0;old=strncmp(buf,"format=2\n",9)==0;
    if(old){
        if(sscanf(buf,"format=2\ncarplay_map=%d\ncarplay_mirror=%d\ncarplay_information=%d\ncarlife_mirror=%d\ngeneration=%llu\n%n",&map,&mirror,&info,&cl,&generation,&used)!=5 || used<=0)return -1;
        if(map<0||map>1||mirror<0||mirror>1||info<0||info>2||cl<0||cl>1)return -1;
        length=snprintf(canonical,sizeof(canonical),"format=2\ncarplay_map=%d\ncarplay_mirror=%d\ncarplay_information=%d\ncarlife_mirror=%d\ngeneration=%llu\n",map,mirror,info,cl,generation);
    }else{
        if(sscanf(buf,"format=3\nvc_map_mask=%d\nvc_mirror_mask=%d\ncarplay_information=%d\ngeneration=%llu\n%n",&map,&mirror,&info,&generation,&used)!=4 || used<=0)return -1;
        if(map<0||map>3||mirror<0||mirror>7||info<0||info>3||(map&mirror)!=0)return -1;
        length=snprintf(canonical,sizeof(canonical),"format=3\nvc_map_mask=%d\nvc_mirror_mask=%d\ncarplay_information=%d\ngeneration=%llu\n",map,mirror,info,generation);
    }
    if(generation<1||generation>0x7fffffffffffffffULL||length<=0||length>=(int)sizeof(canonical))return -1;
    body=(size_t)length;
    length=snprintf(canonical+body,sizeof(canonical)-body,"crc32=%x\n",(unsigned)cpmap_preference_crc((unsigned char *)canonical,body));
    if(length<=0||body+(size_t)length!=n||memcmp(buf,canonical,n))return -1;
    if(old)mirror=(mirror|(cl<<2))&~map;
    out->map_mask=map;out->mirror_mask=mirror;out->information=info;out->generation=generation;
    out->map=(map&1)!=0;out->mirror=(mirror&1)!=0;out->carlife=(mirror&4)!=0;
    return 1;
}
static int cpmap_read_projection_preferences(const char *directory,struct cpmap_projection_preferences *out)
{
    char path[512];int first,second,result;struct cpmap_map_preference legacy;
    if(snprintf(path,sizeof(path),"%s/settings.state",directory)>=(int)sizeof(path))return -1;
    first=cpmap_projection_file(path,out);if(first==1){out->source=1;return 1;}
    if(snprintf(path,sizeof(path),"%s/settings.previous",directory)>=(int)sizeof(path))return -1;
    second=cpmap_projection_file(path,out);if(second==1){out->source=2;return 1;}
    if(first!=0||second!=0)return -1;
    result=cpmap_read_map_preference(directory,&legacy);if(result<0)return -1;
    out->map=legacy.enabled;out->mirror=0;out->information=0;out->carlife=0;
    out->map_mask=legacy.enabled?1:0;out->mirror_mask=0;out->generation=legacy.generation;out->source=result==0?0:3;
    return result;
}
#endif
