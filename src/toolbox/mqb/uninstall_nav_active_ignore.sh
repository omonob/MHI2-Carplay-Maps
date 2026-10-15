#!/bin/sh
export PATH=/proc/boot:/bin:/usr/bin:/usr/sbin:/sbin:/mnt/app/media/gracenote/bin:/mnt/app/armle/bin:/mnt/app/armle/sbin:/mnt/app/armle/usr/bin:/mnt/app/armle/usr/sbin:$PATH

if [ "$_" = "/bin/on" ]; then BASE="$0"; else BASE="$_"; fi
SCRIPTDIR=$( cd -P -- "$(dirname -- "$(command -v -- "$BASE")")" && pwd -P )

export LD_LIBRARY_PATH=/net/mmx/mnt/app/root/lib-target:/net/mmx/eso/lib:/net/mmx/mnt/app/usr/lib:/net/mmx/mnt/app/armle/lib:/net/mmx/mnt/app/armle/lib/dll:/net/mmx/mnt/app/armle/usr/lib

JAR_TARGET=/mnt/app/eso/hmi/lsd/jars
LSD=/mnt/app/eso/hmi/lsd/lsd.sh
BU=/mnt/app/eso/hmi/lsd/lsd.sh.bu
TMP=/tmp/lsd.sh.nai.$$
JARLINE='BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/NavActiveIgnore.jar"'

mount -uw /mnt/app

# Remove only our own entry, so other lsd.sh patches (CarPlay Maps VC etc.)
# installed after NavActiveIgnore are not rolled back by restoring the backup.
awk -v jarline="$JARLINE" '
  $0 == jarline { next }
  { gsub(/\$BASE_DIR\/lsd\/jars\/NavActiveIgnore\.jar:/, "");
    gsub(/:\$BASE_DIR\/lsd\/jars\/NavActiveIgnore\.jar/, "");
    print }' $LSD > $TMP

if [ -s $TMP ] && ! grep -q 'NavActiveIgnore.jar' $TMP && { [ ! -x /bin/ksh ] || /bin/ksh -n $TMP; }; then
  echo "Removing NavActiveIgnore from ${LSD}"
  cp $TMP $LSD && chmod 755 $LSD
elif [ -e $BU ]; then
  echo "Unexpected lsd.sh layout, restoring ${LSD} from backup"
  cp $BU $LSD
else
  echo "WARNING: could not clean ${LSD} and no backup found"
fi
rm -f $TMP
rm -f $BU

echo "Removing ${JAR_TARGET}/NavActiveIgnore.jar"
rm -f ${JAR_TARGET}/NavActiveIgnore.jar

sync
mount -ur /mnt/app
echo "Done, please restart headunit"
