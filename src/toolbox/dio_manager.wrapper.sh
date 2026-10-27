#!/bin/sh
ROOT=/mnt/app/carplaymaps
REAL="$ROOT/stock_first/dio_manager.real"
LOG=/ramdisk/carplaymaps-launcher.log
if test ! -x "$REAL" || test ! -r "$ROOT/core.so";then exit 127;fi
case "$(cat "$ROOT/PROFILE" 2>/dev/null)" in
 MU1102/790|MU1367/790|MU1440/790) CPVC_CLUSTER_PROFILE=790 ;;
 MU1102/791|MU1367/791) CPVC_CLUSTER_PROFILE=791 ;;
 *) echo profile-invalid > "$LOG";exec "$REAL" "$@" ;;
esac
export CPVC_CLUSTER_PROFILE
export IPL_CONFIG_DIR=/etc/eso/production IPL_CONFIG_DIR_DIO_MANAGER=/etc/eso/production
export PATH=/proc/boot:/sbin:/bin:/usr/bin:/usr/sbin:/armle/usr/bin:/armle/usr/sbin:/armle/sbin:/armle/bin
export LD_LIBRARY_PATH=/mnt/app/root/lib-target:/eso/lib:/mnt/app/usr/lib:/mnt/app/armle/lib:/mnt/app/armle/lib/dll:/mnt/app/armle/usr/lib
export CARPLAY_ALTSCREEN_ENABLE=1 CARPLAY_ALTSCREEN_LOG=/ramdisk/carplaymaps-video.log CARPLAY_ALTSCREEN_CAPTURE_DIR=/ramdisk
unset CARPLAY_CLUSTER_INTRO CARPLAY_CLUSTER_INTRO_FPS LD_PRELOAD
echo $$ > /ramdisk/carplay-dio-manager.pid
if test -x "$ROOT/carplay-cluster-supervisor";then
 ("$ROOT/carplay-cluster-supervisor" >/dev/null 2>&1) &
fi
PAIR_ROOT="$ROOT/security_bridge"
if test -x "$PAIR_ROOT/carplay-pairing-helper" && test -r "$PAIR_ROOT/libCarPlayPairingCore.so";then
 (export LD_LIBRARY_PATH="$PAIR_ROOT:$LD_LIBRARY_PATH" CARPLAY_PAIRING_STORE="$PAIR_ROOT";exec "$PAIR_ROOT/carplay-pairing-helper" --server >/dev/null 2>&1) &
fi
LD_PRELOAD="$ROOT/core.so:$ROOT/libcarplay-security-bridge.so:$ROOT/libcpvc-mirror-tap.so"
export LD_PRELOAD
echo 'public-v2=map-and-mirror menu-policy=independent wireless-service=not-included' > "$LOG"
exec "$REAL" "$@"
