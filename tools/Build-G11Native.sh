#!/bin/sh
# G11 prerelease. Uses only the existing locally built Neutrino GNU image.
set -eu
cc=arm-unknown-nto-qnx6.5.0eabi-gcc
re=arm-unknown-nto-qnx6.5.0eabi-readelf
test "$("$cc" -dumpversion)" = 4.9.4
publish=${CPMAP_G11_BUILD_OUT:-/src/build-g11-native}
test ! -e "$publish"
out=$(mktemp -d /tmp/cpmap-g11-native-XXXXXX)
mkdir -p "$out"
flags='-std=gnu99 -O2 -Wall -Wextra -Werror -Wno-unused-function -mno-unaligned-access'
mkdir -p "$out/common"
"$cc" $flags /src/src/toolbox/cpmap-nav-check.c -o "$out/common/cpmap-nav-check"
for pair in 1433:MU1433 1447:MU1447; do
 number=${pair%:*};name=${pair#*:}
 mkdir -p "$out/$name/790"
 "$cc" $flags -fPIC -shared -DCPMAP_FIRMWARE=$number -DCPMAP_CLUSTER=790 -include /src/src/card-core/source/profile_card.h /src/src/card-core/source/altscreen_handshake_probe.c -o "$out/$name/790/core.so"
 "$re" -W -h -A -d "$out/$name/790/core.so" > "$out/$name/790/core.readelf.txt"
 for test in test_scene_runtime test_card_transport790 test_session_layout; do
  /usr/bin/cc -B/usr/bin/ -std=gnu99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fno-pie -no-pie -fsanitize=address,undefined -DCPMAP_FIRMWARE=$number -DCPMAP_CLUSTER=790 -DVW790_NAV_SAFE_X=386 -include /src/src/card-core/source/profile_card.h "/src/src/card-core/tests/$test.c" -ldl -pthread -o /tmp/cpmap-g11-host-test
  ASAN_OPTIONS=handle_segv=0:halt_on_error=1:detect_leaks=1 timeout -k 1 30 /tmp/cpmap-g11-host-test
 done
 if "$cc" $flags -fPIC -shared -DCPMAP_FIRMWARE=$number -DCPMAP_CLUSTER=791 -include /src/src/card-core/source/profile_card.h /src/src/card-core/source/altscreen_handshake_probe.c -o /tmp/cpmap-g11-invalid.so > "$out/$name/reject-aid12.log" 2>&1; then
  echo 'FAIL: G11 must reject AID12.3';exit 9
 fi
 echo "G11_CORE_HOST_TEST_PASS=$name output=1010x376 aid12=rejected"
done
echo G11_NATIVE_EXPERIMENT_BUILT_NOT_VEHICLE_VALIDATED
cp -r "$out" "$publish"
