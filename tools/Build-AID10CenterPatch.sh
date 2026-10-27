#!/bin/sh
# Locally built Neutrino ARMv7 compiler only; no original SDK compiler/license.
set -eu
cd /src
cc=arm-unknown-nto-qnx6.5.0eabi-gcc
test "$("$cc" -dumpversion)" = 4.9.4
b=$(mktemp -d /tmp/aid-center-XXXXXX)
cp -r src/card-core "$b/"
test ! -e build-aid-center
mkdir -p build-aid-center
for mu in 1102 1367 1440;do
 "$cc" -std=gnu99 -O2 -Wall -Wextra -Werror -Wno-unused-function -mno-unaligned-access -fPIC -shared -DCPMAP_FIRMWARE=$mu -DCPMAP_CLUSTER=790 -include "$b/card-core/source/profile_card.h" "$b/card-core/source/altscreen_handshake_probe.c" -o "$b/core-$mu.so"
 arm-unknown-nto-qnx6.5.0eabi-readelf -W -h -A -d "$b/core-$mu.so" > "build-aid-center/MU$mu.readelf.txt"
 cp "$b/core-$mu.so" "build-aid-center/MU$mu.so"
done
"$cc" -std=gnu99 -O2 -Wall -Wextra -Werror -Wno-unused-function -mno-unaligned-access -fPIC -shared -DCPMAP_FIRMWARE=1102 -DCPMAP_CLUSTER=791 -include "$b/card-core/source/profile_card.h" "$b/card-core/source/altscreen_handshake_probe.c" -o "$b/aid12.so"
cmp "$b/aid12.so" MU1102/791_AID12.3/payload/profiles/MU1102/791/core.so
cmp "$b/aid12.so" MU1367/791_AID12.3/payload/profiles/MU1367/791/core.so
echo AID12_3_BOTH_NATIVE_CORES_UNCHANGED
for t in test_scene_runtime test_card_transport790;do
 /usr/bin/cc -B/usr/bin/ -std=gnu99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fno-pie -no-pie -fsanitize=address,undefined -DCPMAP_FIRMWARE=1102 -DCPMAP_CLUSTER=790 -DVW790_NAV_SAFE_X=386 -include "$b/card-core/source/profile_card.h" "$b/card-core/tests/$t.c" -ldl -pthread -o "$b/test"
 ASAN_OPTIONS=handle_segv=0:halt_on_error=1:detect_leaks=1 timeout -k 1 30 "$b/test"
done
sha256sum build-aid-center/*.so
echo THREE_FIRMWARE_AID10_IPHONE17_CENTER_BUILD_PASS
