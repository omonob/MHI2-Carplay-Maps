#!/bin/bash
set -u
T=${WORK:?set WORK to a scratch dir containing lsd.sh.factory and sd/}; SCR=${SCR:?path to Toolbox/scripts}
CPM=${CPM:?path to MU1427/791_AID12.3}
PASS=0; FAIL=0
check(){ if eval "$2"; then echo "  PASS  $1"; PASS=$((PASS+1)); else echo "  FAIL  $1"; FAIL=$((FAIL+1)); fi; }
sha(){ sha1sum "$1" | cut -d' ' -f1; }
mk(){ R=$T/root; rm -rf $R; mkdir -p $R/ifs $R/ramdisk $R/mnt/app/eso/bin/apps $R/mnt/app/eso/hmi/lsd/jars
  truncate -s ${1:-52751492} $R/ifs/lsd.jxe; head -c 600000 /dev/urandom > $R/mnt/app/eso/bin/apps/dio_manager; chmod 755 $R/mnt/app/eso/bin/apps/dio_manager
  cp $T/lsd.sh.factory $R/mnt/app/eso/hmi/lsd/lsd.sh; LSD=$R/mnt/app/eso/hmi/lsd/lsd.sh; }
xform(){ sed -e "s,/ifs/lsd.jxe,$R/ifs/lsd.jxe,g" -e "s,/mnt/app/eso,$R/mnt/app/eso,g" -e 's,^mount ,: mount ,' -e "s,^\. \${SCRIPTDIR}/util_mountsd.sh,VOLUME=$T/sd," "$SCR/$1" > $T/$1.run; }
nai(){ xform install_nav_active_ignore.sh; bash $T/install_nav_active_ignore.sh.run; }
unnai(){ xform uninstall_nav_active_ignore.sh; bash $T/uninstall_nav_active_ignore.sh.run; }
cpm(){ CPMAP_TEST_ROOT=$R bash $CPM/install.sh $CPM/payload MU1427 791 >$T/cpm.log 2>&1; }
bcp(){ awk '/^FIXBOOTCLASSPATH_EXT=/{on=1} on && /^\$J9 /{exit} on && !/^export LD_PRELOAD|^info /' $LSD > $T/bcp.sh
  BASE_DIR=/B DSITRACER_ACTIVATED=no bash -c ". $T/bcp.sh 2>/dev/null; echo \"\$BOOTCLASSPATH\""; }
F0=$(sha $T/lsd.sh.factory)

echo "[A] factory -> install -> reinstall -> uninstall"
mk; nai > $T/a.log 2>&1
check "uses MU1427 jar" 'grep -q "Lsd/MU1427/NavActiveIgnore.jar" $T/a.log'
check "jar installed = MU1427 build" '[ "$(sha $R/mnt/app/eso/hmi/lsd/jars/NavActiveIgnore.jar)" = "$(sha $T/sd/Modifications/Lsd/MU1427/NavActiveIgnore.jar)" ]'
check "one reference" '[ $(grep -c NavActiveIgnore.jar $LSD) -eq 1 ]'
check "line directly before J9 launch" 'grep -B1 "^\$J9 " $LSD | head -1 | grep -q NavActiveIgnore'
check "bash -n" 'bash -n $LSD'
check "effective BCP has /p:NAI and keeps /a:lsd.jxe" 'bcp | grep -q -- "-Xbootclasspath/a:/ifs/lsd.jxe.* -Xbootclasspath/p:/B/lsd/jars/NavActiveIgnore.jar$"'
check "diff vs factory is exactly 1 added line" '[ "$(diff $T/lsd.sh.factory $LSD | grep -c "^[<>]")" = 1 ]'
check "backup = factory" '[ "$(sha $LSD.bu)" = "$F0" ]'
S1=$(sha $LSD); nai >/dev/null 2>&1
check "reinstall idempotent" '[ "$(sha $LSD)" = "$S1" ]'
unnai >/dev/null 2>&1
check "uninstall -> byte-identical factory" '[ "$(sha $LSD)" = "$F0" ]'
check "jar + backup removed" '[ ! -e $R/mnt/app/eso/hmi/lsd/jars/NavActiveIgnore.jar ] && [ ! -e $LSD.bu ]'

echo "[B] factory -> CarPlay Maps -> install -> uninstall"
mk; cpm; check "CPM install ok" 'grep -c CarPlayClusterControls.jar $LSD | grep -qx 1'
C1=$(sha $LSD); nai >/dev/null 2>&1
check "single /p: line holding both jars, NAI first" '[ $(grep -c "Xbootclasspath/p:" $LSD) -eq 1 ] && grep -q "/p:\$BASE_DIR/lsd/jars/NavActiveIgnore.jar:\$BASE_DIR/lsd/jars/CarPlayClusterControls.jar\"" $LSD'
check "bash -n" 'bash -n $LSD'
check "effective BCP" 'bcp | grep -q -- "-Xbootclasspath/p:/B/lsd/jars/NavActiveIgnore.jar:/B/lsd/jars/CarPlayClusterControls.jar$"'
unnai >/dev/null 2>&1
check "uninstall -> CPM-only lsd.sh restored exactly" '[ "$(sha $LSD)" = "$C1" ]'

echo "[C] factory -> install -> CarPlay Maps -> uninstall NAI"
mk; nai >/dev/null 2>&1; cpm
check "CPM merged into NAI /p: line" '[ $(grep -c "Xbootclasspath/p:" $LSD) -eq 1 ] && grep -q CarPlayClusterControls $LSD && grep -q NavActiveIgnore $LSD'
unnai >/dev/null 2>&1
check "CPM jar kept after NAI uninstall" '[ $(grep -c CarPlayClusterControls.jar $LSD) -eq 1 ] && [ $(grep -c NavActiveIgnore $LSD) -eq 0 ]'
check "bash -n" 'bash -n $LSD'
check "effective BCP = CPM only" 'bcp | grep -q -- "-Xbootclasspath/p:/B/lsd/jars/CarPlayClusterControls.jar$"'

echo "[D] legacy install (old toolbox sed result + .bu) -> new install"
mk; cp $LSD $LSD.bu
awk '/^\$J9 /{print "BOOTCLASSPATH=\"$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/NavActiveIgnore.jar\""}1' $T/lsd.sh.factory > $LSD
L1=$(sha $LSD); nai >/dev/null 2>&1
check "legacy layout kept, one reference" '[ "$(sha $LSD)" = "$L1" ]'
check "jar replaced by MU1427 build" '[ "$(sha $R/mnt/app/eso/hmi/lsd/jars/NavActiveIgnore.jar)" = "$(sha $T/sd/Modifications/Lsd/MU1427/NavActiveIgnore.jar)" ]'

echo "[E] other firmware falls back to generic jar"
mk 55840541; nai > $T/e.log 2>&1
check "generic jar selected" 'grep -q "Using: $T/sd/Modifications/Lsd/NavActiveIgnore.jar" $T/e.log'

echo "[F] unknown lsd.sh layout is refused untouched"
mk; grep -v '^\$J9 ' $T/lsd.sh.factory > $LSD; U1=$(sha $LSD); nai > $T/f.log 2>&1
check "refused" 'grep -q "unexpected layout" $T/f.log && [ "$(sha $LSD)" = "$U1" ]'

echo "RESULT: $PASS passed, $FAIL failed"
