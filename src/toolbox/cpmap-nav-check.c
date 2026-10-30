/* SPDX-License-Identifier: GPL-3.0-only
 * A dependency-free ZIP/navigation checker for the minimal MHI2 QNX shell.
 * Never searches a binary JAR with grep and never requires cksum/tr/unzip.
 * Existing third-party archives are read only. No device identity is read.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIMIT (128u * 1024u * 1024u)
#define NAV_BYTES 27179u
#define NAV_CRC32 0xb6c892dcu /* Unchanged upstream navigation JAR. */

static const char *classes[] = {
 "de/esolutions/fw/comm/dsi/carplay/impl/DSICarplayProxy.class",
 "de/esolutions/fw/dsi/androidauto2/DSIAndroidAuto2Dispatcher.class",
 "de/esolutions/fw/dsi/androidauto/DSIAndroidAutoDispatcher.class",
 "de/vw/mib/asl/internal/androidauto/target/NavigationListener.class",
 "de/vw/mib/asl/internal/androidauto/target/RequestHandler.class",
 "de/vw/mib/asl/internal/carplay/common/CarPlayModeHandling.class",
 "de/vw/mib/asl/internal/exboxm/api/impl/guidance/ExboxGuidanceManagerImpl.class"
};
static const char *g11_classes[] = {
 "de/esolutions/fw/comm/dsi/carplay/impl/DSICarplayProxy.class",
 "de/vw/mib/asl/internal/carplay/common/CarPlayModeHandling.class",
 "de/vw/mib/asl/internal/navigation/api/impl/ASLNavigationServices.class",
 "org/omonob/cpmap/compat/G11NavigationRequestFilter.class"
};
static uint32_t u16(const unsigned char *p) { return (uint32_t)p[0] | ((uint32_t)p[1]<<8); }
static uint32_t u32(const unsigned char *p) { return u16(p) | (u16(p+2)<<16); }
static uint32_t crc32(const unsigned char *p,size_t n) {
 uint32_t c=~0u; size_t i; unsigned k;
 for(i=0;i<n;i++){c^=p[i];for(k=0;k<8;k++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}
 return ~c;
}
static int same(const char *a,const char *b) {
 FILE *x=fopen(a,"rb"),*y=fopen(b,"rb"); unsigned char p[4096],q[4096]; size_t n,m; int rc=1;
 if(!x||!y)goto done;
 do {n=fread(p,1,sizeof p,x);m=fread(q,1,sizeof q,y);if(n!=m||memcmp(p,q,n))goto done;}while(n);
 if(!ferror(x)&&!ferror(y))rc=0;
done: if(x)fclose(x);if(y)fclose(y);return rc;
}
static int inspect(const char *path,int payload,int g11) {
 FILE *f=fopen(path,"rb"); unsigned char *p=NULL; long length; size_t n,e,start,off,end,j;
 uint32_t entries,cd_size,cd_off,name_len,extra,comment,local,comp,method,mask=0; int rc=1;
 const char **required=g11?g11_classes:classes;
 unsigned required_count=g11?4u:7u;
 if(!f)return 2;
 if(fseek(f,0,SEEK_END)||(length=ftell(f))<22||(unsigned long)length>LIMIT)goto done;
 n=(size_t)length;p=(unsigned char*)malloc(n);if(!p){rc=2;goto done;}
 if(fseek(f,0,SEEK_SET)||fread(p,1,n,f)!=n||ferror(f)){rc=2;goto done;}
 if(g11){
   size_t expected=9482u;
   uint32_t expected_crc=0x2c0b9415u;
   if(n!=expected||crc32(p,n)!=expected_crc)goto done;
 }else if(payload&&(n!=NAV_BYTES||crc32(p,n)!=NAV_CRC32))goto done;
 start=n>65557?n-65557:0;e=n-22;
 for(;;){if(u32(p+e)==0x06054b50u&&e+22+u16(p+e+20)==n)break;if(e==start)goto done;e--;}
 if(u16(p+e+4)||u16(p+e+6)||u16(p+e+8)!=u16(p+e+10))goto done;
 entries=u16(p+e+10);cd_size=u32(p+e+12);cd_off=u32(p+e+16);
 if(entries==65535u||cd_off>e||cd_size>e-cd_off||cd_off+cd_size!=e)goto done;
 off=cd_off;end=off+cd_size;
 for(j=0;j<entries;j++){
   unsigned k;
   if(off>end||end-off<46||u32(p+off)!=0x02014b50u)goto done;
   name_len=u16(p+off+28);extra=u16(p+off+30);comment=u16(p+off+32);
   if(name_len+extra+comment>end-off-46||u16(p+off+34))goto done;
   local=u32(p+off+42);comp=u32(p+off+20);method=u16(p+off+10);
   if(local>cd_off||cd_off-local<30||u32(p+local)!=0x04034b50u)goto done;
   if(u16(p+local+26)!=name_len||u16(p+local+8)!=method)goto done;
   if(name_len+u16(p+local+28)>cd_off-local-30)goto done;
   if(comp>cd_off-local-30-name_len-u16(p+local+28))goto done;
   if(memcmp(p+off+46,p+local+30,name_len))goto done;
   for(k=0;k<required_count;k++){
     if(strlen(required[k])==name_len&&!memcmp(p+off+46,required[k],name_len)){
       if(mask&(1u<<k))goto done;
       if((u16(p+off+8)&1u)||(method!=0&&method!=8)||u32(p+off+24)<8u)goto done;
       mask|=1u<<k;
     }
   }
   off+=46+name_len+extra+comment;
 }
 if(off==end&&mask==((1u<<required_count)-1u))rc=0;
done: free(p);fclose(f);return rc;
}
int main(int argc,char **argv) {
 if(argc==2&&!strcmp(argv[1],"--self-test")){
   static const unsigned char test[]="123456789";
   return crc32(test,9)==0xcbf43926u?0:2;
 }
 if(argc==4&&!strcmp(argv[1],"--same"))return same(argv[2],argv[3]);
 if(argc==3&&!strcmp(argv[1],"--payload"))return inspect(argv[2],1,0);
 if(argc==3&&!strcmp(argv[1],"--has-patch"))return inspect(argv[2],0,0);
 if(argc==4&&!strcmp(argv[1],"--g11-payload")){
   int mu=!strcmp(argv[2],"MU1433")?1433:0;
   return mu?inspect(argv[3],1,mu):2;
 }
 fputs("cpmap-nav-check --payload|--has-patch JAR; --g11-payload MU1433 JAR; --same A B; --self-test\n",stderr);return 2;
}
