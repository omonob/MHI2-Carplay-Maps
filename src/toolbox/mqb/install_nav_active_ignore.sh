#!/bin/sh
export PATH=/proc/boot:/bin:/usr/bin:/usr/sbin:/sbin:/mnt/app/media/gracenote/bin:/mnt/app/armle/bin:/mnt/app/armle/sbin:/mnt/app/armle/usr/bin:/mnt/app/armle/usr/sbin:$PATH

if [ "$_" = "/bin/on" ]; then BASE="$0"; else BASE="$_"; fi
SCRIPTDIR=$( cd -P -- "$(dirname -- "$(command -v -- "$BASE")")" && pwd -P )

export LD_LIBRARY_PATH=/net/mmx/mnt/app/root/lib-target:/net/mmx/eso/lib:/net/mmx/mnt/app/usr/lib:/net/mmx/mnt/app/armle/lib:/net/mmx/mnt/app/armle/lib/dll:/net/mmx/mnt/app/armle/usr/lib

. ${SCRIPTDIR}/util_mountsd.sh
if [[ -z "$VOLUME" ]]
then
  echo "No SD-card found, quitting"
  exit 0
fi

JXE=/ifs/lsd.jxe
JAR_TARGET=/mnt/app/eso/hmi/lsd/jars
LSD=/mnt/app/eso/hmi/lsd/lsd.sh
BU=/mnt/app/eso/hmi/lsd/lsd.sh.bu
TMP=/tmp/lsd.sh.nai.$$
JARLINE='BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/NavActiveIgnore.jar"'

# --- Pick the jar that matches this firmware --------------------------------
# NavActiveIgnore.jar overrides whole HMI classes, so it must be built from the
# exact lsd.jxe it runs on. The generic jar was built for a newer firmware
# (6-arg CarPlayModeHandling constructor / Exbox guidance API) and breaks
# CarPlay completely on MU1427.
set -- `ls -ln $JXE 2>/dev/null`
JXE_SIZE="$5"
case "$JXE_SIZE" in
  52751492) FW=MU1427; JAR_SRC=${VOLUME}/Modifications/Lsd/MU1427/NavActiveIgnore.jar ;;
  *)        FW="unknown ($JXE_SIZE)"; JAR_SRC=${VOLUME}/Modifications/Lsd/NavActiveIgnore.jar ;;
esac
echo "Firmware: $FW"
echo "Using: $JAR_SRC"
if [ ! -s "$JAR_SRC" ]; then
  echo "ERROR: $JAR_SRC not found on SD card, nothing changed"
  exit 1
fi

mount -uw /mnt/app

mkdir -p ${JAR_TARGET}
cp -v ${JAR_SRC} ${JAR_TARGET}/NavActiveIgnore.jar || { echo "ERROR: copy failed"; mount -ur /mnt/app; exit 1; }

if [ ! -e $BU ]; then
  echo "Backup lsd.sh -> $BU"
  cp -v $LSD $BU
fi

# --- Patch lsd.sh in place ----------------------------------------------------
# 1) drop any previous NavActiveIgnore entry (own line or inside a /p: list)
# 2) if a "-Xbootclasspath/p:" BOOTCLASSPATH line exists (e.g. from CarPlay
#    Maps VC), prepend our jar to it, otherwise insert our own line directly
#    before the single production "$J9 ... $BOOTCLASSPATH ... -jxe" launch.
# Other modifications in lsd.sh (CarPlayClusterControls.jar etc.) are kept.
awk -v jarline="$JARLINE" '
  { lines[NR] = $0 }
  END {
    n = 0
    for (i = 1; i <= NR; i++) {
      l = lines[i]
      if (l == jarline) continue
      gsub(/\$BASE_DIR\/lsd\/jars\/NavActiveIgnore\.jar:/, "", l)
      gsub(/:\$BASE_DIR\/lsd\/jars\/NavActiveIgnore\.jar/, "", l)
      out[++n] = l
    }
    pline = 0; jline = 0; jcount = 0
    for (i = 1; i <= n; i++) {
      if (!pline && out[i] ~ /^BOOTCLASSPATH="\$BOOTCLASSPATH -Xbootclasspath\/p:/) pline = i
      if (out[i] ~ /^\$J9 .*\$BOOTCLASSPATH .*-jxe/) { jcount++; jline = i }
    }
    if (jcount != 1) { print "UNSUPPORTED" > "/dev/stderr"; exit 3 }
    for (i = 1; i <= n; i++) {
      l = out[i]
      if (pline && i == pline) {
        sub(/-Xbootclasspath\/p:/, "-Xbootclasspath/p:$BASE_DIR/lsd/jars/NavActiveIgnore.jar:", l)
      }
      if (!pline && i == jline) print jarline
      print l
    }
  }' $LSD > $TMP
RC=$?

if [ $RC -ne 0 ] || [ ! -s $TMP ]; then
  echo "ERROR: lsd.sh has an unexpected layout (rc=$RC), lsd.sh NOT changed"
  rm -f $TMP
  mount -ur /mnt/app
  exit 1
fi
if [ `grep -c 'NavActiveIgnore.jar' $TMP` -ne 1 ]; then
  echo "ERROR: verification failed, lsd.sh NOT changed"
  rm -f $TMP
  mount -ur /mnt/app
  exit 1
fi
if [ -x /bin/ksh ]; then
  /bin/ksh -n $TMP || { echo "ERROR: syntax check failed, lsd.sh NOT changed"; rm -f $TMP; mount -ur /mnt/app; exit 1; }
fi

cp $TMP $LSD && chmod 755 $LSD
rm -f $TMP
echo "lsd.sh boot classpath:"
grep -n 'Xbootclasspath/p:' $LSD

sync
mount -ur /mnt/app
echo "Done, please restart headunit"
