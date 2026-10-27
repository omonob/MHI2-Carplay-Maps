/* SPDX-License-Identifier: GPL-3.0-only
 * Read the saved menu switch. This is NOT a wireless CarPlay receiver.
 * No Bluetooth, Wi-Fi, SDK authentication or service actions occur here. */
#include <stdio.h>
#include "../src/vc-runtime/runtime/cpvc_wireless_settings.h"
int main(int argc,char **argv){
 struct cpvc_wireless_settings state;
 int rc=cpvc_wireless_read(argc>1?argv[1]:NULL,&state);
 printf("enabled=%d generation=%llu source=%d status=%d\n",state.enabled,state.generation,state.source,rc);
 return rc<0?2:0;
}
