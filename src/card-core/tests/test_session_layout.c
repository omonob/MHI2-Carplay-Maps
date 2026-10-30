/* Synthetic bytes only; no real session keys or device identities. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if CPMAP_FIRMWARE == 1433
#define EXPECTED_OFFSET 448U
#elif CPMAP_FIRMWARE == 1447
#define EXPECTED_OFFSET 456U
#else
#error ExactAuditedG11ProfileRequired
#endif
int main(void) {
 uint8_t session[512],expected[16];unsigned i;
 assert(MHI2_SESSION_AES_KEY_OFFSET==EXPECTED_OFFSET);
 assert(ALTSCREEN_DISPLAY_WIDTH==1010 && ALTSCREEN_DISPLAY_HEIGHT==376);
 assert(ALTSCREEN_SAFE_ORIGIN_X==202 && ALTSCREEN_SAFE_ORIGIN_Y==16);
 assert(ALTSCREEN_SAFE_WIDTH==606 && ALTSCREEN_SAFE_HEIGHT==344);
 memset(session,0xa5,sizeof(session));
 for(i=0;i<sizeof(expected);i++)expected[i]=(uint8_t)(i+1);
 memcpy(session+EXPECTED_OFFSET,expected,sizeof(expected));
 assert(!memcmp(session+MHI2_SESSION_AES_KEY_OFFSET,expected,sizeof(expected)));
 assert(memcmp(session+(EXPECTED_OFFSET==448U?456U:448U),expected,sizeof(expected))!=0);
 printf("G11_EXACT_SESSION_LAYOUT_PASS firmware=%d offset=%u output=1010x376 no_real_keys=1\n",CPMAP_FIRMWARE,(unsigned)MHI2_SESSION_AES_KEY_OFFSET);
 return 0;
}
