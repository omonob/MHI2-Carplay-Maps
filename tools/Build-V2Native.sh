#!/bin/sh
# Run inside the locally built Neutrino6.5 ARMv7 container, with repo at/src.
set -eu
cc=arm-unknown-nto-qnx6.5.0eabi-gcc
re=arm-unknown-nto-qnx6.5.0eabi-readelf
test "$("$cc" -dumpversion)" = 4.9.4
b=$(mktemp -d /tmp/cpmap-v2-XXXXXX)
cp -r /src/src/card-core /src/src/vc-runtime "$b/"
publish=/src/build-v2-native
test ! -e "$publish"
out="$b/output"
mkdir -p "$out"
flags='-std=gnu99 -O2 -Wall -Wextra -Werror -Wno-unused-function -mno-unaligned-access'
for pair in 1102:790 1102:791 1367:790 1367:791 1440:790;do
 mu=${pair%:*};cluster=${pair#*:}
 mkdir -p "$out/MU$mu/$cluster"
 "$cc" $flags -fPIC -shared -DCPMAP_FIRMWARE=$mu -DCPMAP_CLUSTER=$cluster -include "$b/card-core/source/profile_card.h" "$b/card-core/source/altscreen_handshake_probe.c" -o "$out/MU$mu/$cluster/core.so"
 "$re" -W -h -A -d "$out/MU$mu/$cluster/core.so" > "$out/MU$mu/$cluster/core.readelf.txt"
done
for cluster in 790 791;do
 defs="-DCPMAP_FIRMWARE=1102 -DCPMAP_CLUSTER=$cluster -DVW790_NAV_SAFE_X=386"
 scene=test_scene_runtime
 transport=test_card_transport790
 if test "$cluster" = 791;then scene=test_scene_791;transport=test_card_transport;fi
 for test in "$scene" "$transport";do
  /usr/bin/cc -B/usr/bin/ -std=gnu99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fno-pie -no-pie -fsanitize=address,undefined $defs -include "$b/card-core/source/profile_card.h" "$b/card-core/tests/$test.c" -ldl -pthread -o "$b/test"
  ASAN_OPTIONS=handle_segv=0:halt_on_error=1:detect_leaks=1 timeout -k 1 30 "$b/test"
 done
done
mkdir -p "$out/common"
"$cc" $flags "$b/vc-runtime/runtime/carplay_cluster_supervisor.c" -lsocket -o "$out/common/carplay-cluster-supervisor"
"$cc" $flags "$b/vc-runtime/runtime/altscreen_most_bridge.c" -lsocket -o "$out/common/altscreen-most-bridge"
"$cc" $flags -fPIC -shared "$b/vc-runtime/mirror/carplay_yuv_shm_tap.c" -o "$out/common/libcpvc-mirror-tap.so"
"$cc" $flags -fPIC -shared "$b/vc-runtime/mirror/dmdt_unbuffer.c" -o "$out/common/libcpvc-dmdt-query.so"
for cluster in 790 791;do
 "$cc" $flags -Wno-unused-but-set-variable -Wno-misleading-indentation -DCPVC_SHM_ONLY=1 -DCPVC_BUILD_PROFILE=$cluster "$b/vc-runtime/mirror/carplay_mirror_receiver.c" -Wl,-E -lEGL -lGLESv2 -lsocket -o "$out/common/cpvc-mirror-canvas-$cluster"
done
find "$out" -type f ! -name '*.txt' -exec sha256sum '{}' \;
echo V2_FIVE_NATIVE_CORES_TWO_FIXED_MIRROR_OUTPUTS_BUILT
cp -r "$out" "$publish"
