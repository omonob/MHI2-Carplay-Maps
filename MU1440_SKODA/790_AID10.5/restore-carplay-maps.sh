#!/bin/ksh

if [ -n "${CPMAP_TEST_ROOT:-}" ] && [ -d "$CPMAP_TEST_ROOT/mnt/app" ] && [ -s "$CPMAP_TEST_ROOT/ifs/lsd.jxe" ]; then
    MMX="$CPMAP_TEST_ROOT"
elif [ -d /mnt/app ] && [ -s /ifs/lsd.jxe ]; then
    MMX=""
elif [ -d /net/mmx/mnt/app ] && [ -s /net/mmx/ifs/lsd.jxe ]; then
    MMX="/net/mmx"
else
    echo "CPMAP_RESTORE_FAIL=MMX_FILESYSTEM_NOT_FOUND"
    exit 1
fi

APP="$MMX/mnt/app"
ROOT="$APP/carplaymaps"
DIO="$APP/eso/bin/apps/dio_manager"
LSD="$APP/eso/hmi/lsd/lsd.sh"
JAR="$APP/eso/hmi/lsd/jars/CarPlayClusterControls.jar"
NAV_NAME="CarPlayNavigationPatch.jar"
if [ -f "$ROOT/rollback/NAV_JAR_NAME" ]; then
    NAV_NAME=`cat "$ROOT/rollback/NAV_JAR_NAME"` || exit 1
    case "$NAV_NAME" in CarPlayNavigationPatch.jar|CarPlayMapsG11Navigation.jar) ;; *) echo "CPMAP_RESTORE_FAIL=NAV_BACKUP_METADATA_INVALID"; exit 1 ;; esac
fi
NAV_JAR="$APP/eso/hmi/lsd/jars/$NAV_NAME"

[ -s "$ROOT/stock_first/dio_manager.real" ] || { echo "CPMAP_RESTORE_FAIL=STOCK_MISSING"; exit 1; }
[ -s "$ROOT/rollback/lsd.sh" ] || { echo "CPMAP_RESTORE_FAIL=LSD_BACKUP_MISSING"; exit 1; }

if [ -n "${CPMAP_TEST_ROOT:-}" ]; then
    :
elif [ -n "$MMX" ]; then
    on -f mmx mount -uw /mnt/app >/dev/null 2>&1 || exit 1
    on -f mmx slay -f cpmap-guard >/dev/null 2>&1 || true
    on -f mmx slay -f carplay-cluster-supervisor >/dev/null 2>&1 || true
    on -f mmx slay -f altscreen-most-bridge >/dev/null 2>&1 || true
    on -f mmx slay -f carplay-pairing-helper >/dev/null 2>&1 || true
else
    mount -uw /mnt/app >/dev/null 2>&1 || exit 1
    slay -f cpmap-guard >/dev/null 2>&1 || true
    slay -f carplay-cluster-supervisor >/dev/null 2>&1 || true
    slay -f altscreen-most-bridge >/dev/null 2>&1 || true
    slay -f carplay-pairing-helper >/dev/null 2>&1 || true
fi

cp "$ROOT/stock_first/dio_manager.real" "$DIO.restore" || exit 1
cp "$ROOT/rollback/lsd.sh" "$LSD.restore" || exit 1
chmod 755 "$DIO.restore" "$LSD.restore"
mv "$DIO.restore" "$DIO" || exit 1
mv "$LSD.restore" "$LSD" || exit 1

if [ -s "$ROOT/rollback/CarPlayClusterControls.jar" ]; then
    cp "$ROOT/rollback/CarPlayClusterControls.jar" "$JAR.restore" || exit 1
    chmod 644 "$JAR.restore"
    mv "$JAR.restore" "$JAR" || exit 1
elif [ -e "$ROOT/rollback/JAR_ABSENT" ]; then
    rm -f "$JAR"
fi

if [ -s "$ROOT/rollback/$NAV_NAME" ]; then
    cp "$ROOT/rollback/$NAV_NAME" "$NAV_JAR.restore" || exit 1
    chmod 644 "$NAV_JAR.restore"
    mv "$NAV_JAR.restore" "$NAV_JAR" || exit 1
elif [ -e "$ROOT/rollback/NAV_JAR_ABSENT" ]; then
    rm -f "$NAV_JAR"
fi
for name in VCMenu.jar VCMirrorRate.jar; do
    if [ -e "$ROOT/rollback/$name" ]; then
        cp "$ROOT/rollback/$name" "$JAR.restore" || exit 1
        chmod 644 "$JAR.restore"
        mv "$JAR.restore" "$APP/eso/hmi/lsd/jars/$name" || exit 1
    elif [ -e "$ROOT/rollback/$name.absent" ]; then
        rm -f "$APP/eso/hmi/lsd/jars/$name"
    fi
done

rm -rf "$ROOT"
sync 2>/dev/null || true
echo "CPMAP_RESTORE_OK"
echo "A full unit reboot is required."
exit 0
