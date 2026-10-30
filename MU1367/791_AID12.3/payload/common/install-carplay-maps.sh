#!/bin/ksh

PAYLOAD="$1"
EXPECTED_MU="$2"
INSTRUMENT="$3"
VERSION="VC-PLAINTEXT-2.1.0-g11-beta.1"

fail() {
    echo "CPMAP_INSTALL_FAIL=$1"
    exit 1
}

if [ -z "$PAYLOAD" ] || [ -z "$EXPECTED_MU" ] || [ -z "$INSTRUMENT" ]; then
    fail "INVALID_ARGUMENTS"
fi

case "$EXPECTED_MU:$INSTRUMENT" in
    MU1102:791|MU1102:790|MU1367:791|MU1367:790|MU1440:790|MU1433:790|MU1447:790) ;;
    *) fail "UNSUPPORTED_PROFILE" ;;
esac

if [ -n "${CPMAP_TEST_ROOT:-}" ] && [ -s "$CPMAP_TEST_ROOT/ifs/lsd.jxe" ] && [ -d "$CPMAP_TEST_ROOT/mnt/app" ]; then
    MMX="$CPMAP_TEST_ROOT"
elif [ -s /ifs/lsd.jxe ] && [ -d /mnt/app ]; then
    MMX=""
elif [ -s /net/mmx/ifs/lsd.jxe ] && [ -d /net/mmx/mnt/app ]; then
    MMX="/net/mmx"
else
    fail "MMX_FILESYSTEM_NOT_FOUND"
fi

APP="$MMX/mnt/app"
JXE="$MMX/ifs/lsd.jxe"
DIO="$APP/eso/bin/apps/dio_manager"
LSD="$APP/eso/hmi/lsd/lsd.sh"
JARS="$APP/eso/hmi/lsd/jars"
ROOT="$APP/carplaymaps"
NEW="$APP/.carplaymaps-new-$$"
OLD="$APP/.carplaymaps-old-$$"
STAGE="$MMX/ramdisk/.cpmap-plaintext-stage-$$"
[ -n "$MMX" ] || STAGE="/ramdisk/.cpmap-plaintext-stage-$$"

if [ ! -s "$JXE" ] || [ ! -s "$DIO" ] || [ ! -s "$LSD" ]; then
    fail "REQUIRED_FACTORY_FILE_MISSING"
fi
if [ -z "${CPMAP_TEST_ROOT:-}" ] && [ ! -x "$DIO" ]; then
    fail "DIO_MANAGER_NOT_EXECUTABLE"
fi

line=`ls -ln "$JXE" 2>/dev/null` || fail "JXE_STAT_FAILED"
set -- $line
JXE_SIZE="$5"
case "$JXE_SIZE" in
    55915411) DETECTED_MU="MU1102" ;;
    55840541) DETECTED_MU="MU1367" ;;
    55840933) DETECTED_MU="MU1440" ;;
    52751492) DETECTED_MU="MU1433" ;;
    55712574) DETECTED_MU="MU1447" ;;
    *) fail "UNSUPPORTED_JXE_SIZE_$JXE_SIZE" ;;
esac

if [ "$DETECTED_MU" != "$EXPECTED_MU" ]; then
    fail "PROFILE_MISMATCH_${DETECTED_MU}_EXPECTED_${EXPECTED_MU}"
fi
case "$DETECTED_MU:$INSTRUMENT" in
    MU1440:791|MU1433:791|MU1447:791) fail "SKODA_SEAT_REQUIRES_AID10_5" ;;
esac
if [ -e "$MMX/dev/ipod0" ]; then
    fail "DISCONNECT_CARPLAY_BEFORE_INSTALL"
fi

CORE="$PAYLOAD/profiles/$EXPECTED_MU/$INSTRUMENT/core.so"
WRAPPER="$PAYLOAD/common/dio_manager.wrapper.sh"
CONTROL_JAR="$PAYLOAD/common/CarPlayClusterControls.jar"
if [ -s "$PAYLOAD/profiles/$EXPECTED_MU/$INSTRUMENT/CarPlayClusterControls.jar" ]; then
    CONTROL_JAR="$PAYLOAD/profiles/$EXPECTED_MU/$INSTRUMENT/CarPlayClusterControls.jar"
fi
NAV_NAME="CarPlayNavigationPatch.jar"
NAV_KIND="standard"
case "$DETECTED_MU" in
    MU1433) NAV_NAME="CarPlayMapsG11Navigation.jar"; NAV_KIND="g11" ;;
esac
NAV_JAR="$PAYLOAD/common/$NAV_NAME"
[ "$NAV_KIND" != g11 ] || NAV_JAR="$PAYLOAD/profiles/$EXPECTED_MU/$INSTRUMENT/$NAV_NAME"
NAV_TARGET="$JARS/$NAV_NAME"
PREVIOUS_NAV_NAME="CarPlayNavigationPatch.jar"
if [ -f "$ROOT/rollback/NAV_JAR_NAME" ]; then
    PREVIOUS_NAV_NAME=`cat "$ROOT/rollback/NAV_JAR_NAME"` || fail "NAV_BACKUP_METADATA_UNREADABLE"
    case "$PREVIOUS_NAV_NAME" in CarPlayNavigationPatch.jar|CarPlayMapsG11Navigation.jar) ;; *) fail "NAV_BACKUP_METADATA_INVALID" ;; esac
fi
NAV_CHECK="$PAYLOAD/common/cpmap-nav-check"
BRIDGE="$PAYLOAD/common/altscreen-most-bridge"
SUPERVISOR="$PAYLOAD/common/carplay-cluster-supervisor"
SEC="$PAYLOAD/common/libcarplay-security-bridge.so"
PAIR_HELPER="$PAYLOAD/common/carplay-pairing-helper"
PAIR_CORE="$PAYLOAD/common/libCarPlayPairingCore.so"
MENU_JAR="$PAYLOAD/profiles/$EXPECTED_MU/$INSTRUMENT/VCMenu.jar"
RATE_JAR="$PAYLOAD/profiles/$EXPECTED_MU/$INSTRUMENT/VCMirrorRate.jar"
MIRROR="$PAYLOAD/common/cpvc-mirror-canvas-$INSTRUMENT"
TAP="$PAYLOAD/common/libcpvc-mirror-tap.so"
QUERY="$PAYLOAD/common/libcpvc-dmdt-query.so"
# Do not let the public installer dismantle a private encrypted installation.
if [ -s "$ROOT/cpmap-auth" ] || grep -q 'VCMenu-V73.jar' "$LSD"; then
    fail "ENCRYPTED_INSTALLATION_REQUIRES_ITS_OWN_MIGRATION_TOOL"
fi

for required in "$CORE" "$WRAPPER" "$CONTROL_JAR" "$NAV_CHECK" "$BRIDGE" "$SUPERVISOR" "$SEC" "$PAIR_HELPER" "$PAIR_CORE" "$MENU_JAR" "$RATE_JAR" "$MIRROR" "$TAP" "$QUERY"; do
    [ -s "$required" ] || fail "PAYLOAD_MISSING_`basename "$required"`"
done

# The target may have neither cksum nor binary-safe text tools. Use our tiny
# read-only checker, on MMX when invoked from the other node. The test override
# is deliberately unavailable unless an explicit fixture filesystem is active.
run_nav_check() {
    if [ -n "${CPMAP_TEST_ROOT:-}" ] && [ -n "${CPMAP_TEST_NAV_CHECK:-}" ]; then
        "$CPMAP_TEST_NAV_CHECK" "$@"
    elif [ -n "$MMX" ]; then
        on -f mmx "$NAV_CHECK" "$@"
    else
        "$NAV_CHECK" "$@"
    fi
}
if [ ! -x "$NAV_CHECK" ]; then
    chmod 755 "$NAV_CHECK" 2>/dev/null || true
fi
run_nav_check --self-test || fail "NAVIGATION_CHECKER_UNAVAILABLE_REEXTRACT_SD"

navigation_payload_intact() {
    [ -s "$1" ] || return 1
    if [ "$NAV_KIND" = g11 ]; then run_nav_check --g11-payload "$DETECTED_MU" "$1"; else run_nav_check --payload "$1"; fi
}

# Inspect real ZIP central/local directory entries, not byte-string matches.
has_navigation_patch() {
    [ -s "$1" ] || return 1
    if [ "$NAV_KIND" = g11 ]; then run_nav_check --g11-payload "$DETECTED_MU" "$1"; else run_nav_check --has-patch "$1"; fi
}
NAV_INSTALL=yes
NAV_USE_OWN=yes
NAV_EXISTING=""
BOOT_JARS=`awk '
    /^[ \t]*#/ { next }
    {
        line=$0
        while (match(line, /-Xbootclasspath\/p:[^" \t]+/)) {
            value=substr(line, RSTART+18, RLENGTH-18)
            gsub(/:/, " ", value)
            print value
            line=substr(line, RSTART+RLENGTH)
        }
    }
' "$LSD"`
for boot_jar in $BOOT_JARS; do
    case "$boot_jar" in
        '$BASE_DIR/'*) boot_file="$APP/eso/hmi/${boot_jar#\$BASE_DIR/}" ;;
        '${BASE_DIR}/'*) boot_file="$APP/eso/hmi/${boot_jar#\$\{BASE_DIR\}/}" ;;
        /mnt/app/*) boot_file="$MMX$boot_jar" ;;
        *) continue ;;
    esac
    # Repair our own incomplete copy; keep unrelated third-party JAR files.
    if [ "$boot_file" = "$NAV_TARGET" ] && ! navigation_payload_intact "$boot_file"; then
        echo "CPMAP_NAVIGATION_PATCH=REPAIR_OWN_JAR"
        continue
    fi
    if has_navigation_patch "$boot_file"; then
        NAV_INSTALL=no
        NAV_EXISTING="$boot_jar"
        [ "$boot_file" = "$NAV_TARGET" ] || NAV_USE_OWN=no
        break
    fi
done
if [ "$NAV_INSTALL" = no ]; then
    echo "CPMAP_NAVIGATION_PATCH=KEEP_EXISTING ($NAV_EXISTING)"
else
    # Only demand a valid SD copy if it is actually needed. A partial custom
    # patch is completed by our seven-class prepend, without changing its JAR.
    [ -s "$NAV_JAR" ] || fail "PAYLOAD_MISSING_$NAV_NAME"
    navigation_payload_intact "$NAV_JAR" || fail "NAVIGATION_PAYLOAD_DAMAGED_REEXTRACT_SD"
    echo "CPMAP_NAVIGATION_PATCH=INSTALL_MISSING"
fi

echo "CarPlay Maps VC"
echo "Free open-source project by omonob (QCDWJ)"
echo "Detected firmware: $DETECTED_MU (lsd.jxe $JXE_SIZE bytes)"
echo "Selected cluster: $INSTRUMENT"
echo "Scope: VC settings, CarPlay map video, native mirroring and navigation cards"
echo "Wireless CarPlay: saved test interface only; no wireless service is included"
echo "No music metadata, startup animation, device binding, or paid watchdog"

if [ -n "${CPMAP_TEST_ROOT:-}" ]; then
    :
elif [ -n "$MMX" ]; then
    on -f mmx mount -uw /mnt/app >/dev/null 2>&1 || fail "APP_REMOUNT_FAILED"
    on -f mmx slay -f cpmap-guard >/dev/null 2>&1 || true
    on -f mmx slay -f carplay-cluster-supervisor >/dev/null 2>&1 || true
    on -f mmx slay -f altscreen-most-bridge >/dev/null 2>&1 || true
    on -f mmx slay -f carplay-pairing-helper >/dev/null 2>&1 || true
else
    mount -uw /mnt/app >/dev/null 2>&1 || fail "APP_REMOUNT_FAILED"
    slay -f cpmap-guard >/dev/null 2>&1 || true
    slay -f carplay-cluster-supervisor >/dev/null 2>&1 || true
    slay -f altscreen-most-bridge >/dev/null 2>&1 || true
    slay -f carplay-pairing-helper >/dev/null 2>&1 || true
fi

rm -rf "$STAGE" "$NEW" "$OLD" 2>/dev/null
mkdir -p "$STAGE" "$NEW/stock_first" "$NEW/rollback" "$NEW/security_bridge" || fail "STAGE_CREATE_FAILED"
cp "$LSD" "$STAGE/lsd.sh.current" || fail "CURRENT_LSD_SNAPSHOT_FAILED"
cp "$DIO" "$STAGE/dio_manager.current" || fail "CURRENT_DIO_SNAPSHOT_FAILED"
if [ -s "$JARS/CarPlayClusterControls.jar" ]; then
    cp "$JARS/CarPlayClusterControls.jar" "$STAGE/control.current" || fail "CURRENT_CONTROL_SNAPSHOT_FAILED"
fi
if [ -s "$NAV_TARGET" ]; then
    cp "$NAV_TARGET" "$STAGE/nav.current" || fail "CURRENT_NAV_SNAPSHOT_FAILED"
fi
for name in VCMenu.jar VCMirrorRate.jar; do
    if [ -e "$JARS/$name" ]; then
        cp "$JARS/$name" "$STAGE/$name.current" || fail "CURRENT_MENU_SNAPSHOT_FAILED"
    fi
done

STOCK=""
for candidate in \
    "$ROOT/stock_first/dio_manager.real" \
    "$ROOT/rollback/dio_manager" \
    "$APP/carplaymaps-open/stock_first/dio_manager.real" \
    "$APP/carplaymaps-open/rollback/dio_manager" \
    "$APP/eso/bin/apps/dio_manager.stock"; do
    if [ -s "$candidate" ]; then
        size=`ls -ln "$candidate" 2>/dev/null`; set -- $size; size="$5"
        if [ "${size:-0}" -gt 500000 ] 2>/dev/null; then
            STOCK="$candidate"
            break
        fi
    fi
done
if [ -z "$STOCK" ]; then
    size=`ls -ln "$DIO" 2>/dev/null`; set -- $size; size="$5"
    if [ "${size:-0}" -gt 500000 ] 2>/dev/null; then
        STOCK="$DIO"
    fi
fi
[ -n "$STOCK" ] || fail "FACTORY_DIO_MANAGER_NOT_FOUND"

LSD_BASE="$LSD"
if [ -s "$ROOT/VERSION" ] && [ -s "$ROOT/rollback/lsd.sh" ]; then
    # A repeat install must retain our first saved startup byte-for-byte, even
    # if that original script contained comments or older classpath references.
    LSD_BASE="$ROOT/rollback/lsd.sh"
else
    for candidate in "$ROOT/map_control_rollback/lsd.sh" "$ROOT/rollback/lsd.sh" "$APP/carplaymaps-open/rollback/lsd.sh"; do
        if [ -s "$candidate" ] && ! grep -q 'CarPlayClusterControls.jar' "$candidate" 2>/dev/null; then
            LSD_BASE="$candidate"
            break
        fi
    done
fi

cp "$STOCK" "$NEW/stock_first/dio_manager.real" || fail "STOCK_COPY_FAILED"
cp "$LSD_BASE" "$NEW/rollback/lsd.sh" || fail "LSD_BACKUP_FAILED"
if [ -e "$ROOT/rollback/JAR_ABSENT" ]; then
    touch "$NEW/rollback/JAR_ABSENT" || fail "JAR_ABSENT_MARK_FAILED"
elif [ -s "$ROOT/rollback/CarPlayClusterControls.jar" ]; then
    cp "$ROOT/rollback/CarPlayClusterControls.jar" "$NEW/rollback/CarPlayClusterControls.jar" || fail "JAR_BACKUP_FAILED"
elif [ -s "$JARS/CarPlayClusterControls.jar" ]; then
    cp "$JARS/CarPlayClusterControls.jar" "$NEW/rollback/CarPlayClusterControls.jar" || fail "JAR_BACKUP_FAILED"
else
    touch "$NEW/rollback/JAR_ABSENT" || fail "JAR_ABSENT_MARK_FAILED"
fi

echo "$NAV_NAME" > "$NEW/rollback/NAV_JAR_NAME" || fail "NAV_BACKUP_METADATA_WRITE_FAILED"
if [ "$PREVIOUS_NAV_NAME" = "$NAV_NAME" ] && [ -e "$ROOT/rollback/NAV_JAR_ABSENT" ]; then
    touch "$NEW/rollback/NAV_JAR_ABSENT" || fail "NAV_ABSENT_MARK_FAILED"
elif [ "$PREVIOUS_NAV_NAME" = "$NAV_NAME" ] && [ -s "$ROOT/rollback/$NAV_NAME" ]; then
    cp "$ROOT/rollback/$NAV_NAME" "$NEW/rollback/$NAV_NAME" || fail "NAV_BACKUP_FAILED"
elif [ -s "$NAV_TARGET" ]; then
    cp "$NAV_TARGET" "$NEW/rollback/$NAV_NAME" || fail "NAV_BACKUP_FAILED"
else
    touch "$NEW/rollback/NAV_JAR_ABSENT" || fail "NAV_ABSENT_MARK_FAILED"
fi

ADD_CLASSPATH='$BASE_DIR/lsd/jars/CarPlayClusterControls.jar'
ADD_CLASSPATH='$BASE_DIR/lsd/jars/VCMenu.jar:$BASE_DIR/lsd/jars/VCMirrorRate.jar:'"$ADD_CLASSPATH"
if [ "$NAV_USE_OWN" = yes ]; then
    [ -z "$ADD_CLASSPATH" ] || ADD_CLASSPATH="$ADD_CLASSPATH:"
    NAV_REFERENCE='$BASE_DIR/lsd/jars/'"$NAV_NAME"
    if [ "$NAV_KIND" = g11 ]; then
        # The exact older ABI must win over any unrelated, existing JAR.
        ADD_CLASSPATH="$NAV_REFERENCE:${ADD_CLASSPATH%:}"
    else
        ADD_CLASSPATH="$ADD_CLASSPATH$NAV_REFERENCE"
    fi
fi
if [ -z "$ADD_CLASSPATH" ]; then
    cp "$LSD" "$STAGE/lsd.sh.patched" || fail "LSD_STAGE_FAILED"
else
    # Extend the existing prepend option if present. Do not emit a second
    # -Xbootclasspath/p option: some J9 builds keep only its last occurrence.
    # On a stock script, add one line immediately before J9 instead.
    awk -v prepend="$ADD_CLASSPATH" -v nav_name="$NAV_NAME" '
        {
            lines[NR]=$0
            if ($0 ~ /^[ \t]*#/) next
            if ($0 ~ /-Xbootclasspath\/p:/ && $0 !~ /^[ \t]*BOOTCLASSPATH="\$BOOTCLASSPATH -Xbootclasspath\/p:[^" \t]+"[ \t]*(#.*)?$/) unsupported=1
            if ($0 ~ /^[ \t]*BOOTCLASSPATH="\$BOOTCLASSPATH -Xbootclasspath\/p:[^" \t]+"[ \t]*(#.*)?$/) {
                remove[NR]=1
                if (!insert) insert=NR
                match($0, /-Xbootclasspath\/p:[^" \t]+/)
                value=substr($0,RSTART+18,RLENGTH-18)
                count=split(value,items,":")
                for(i=1;i<=count;i++) {
                    item_count=split(items[i],path_parts,"/")
                    if(items[i] ~ /\/CarPlayClusterControls\.jar$/ || path_parts[item_count]==nav_name || items[i] ~ /\/VCMenu\.jar$/ || items[i] ~ /\/VCMirrorRate\.jar$/) continue
                    if(length(items[i]) && !seen[items[i]]++) kept=kept ":" items[i]
                }
            }
            if (!j9 && $0 ~ /^[ \t]*\$J9[ \t]/) j9=NR
        }
        END {
            if (!j9 || unsupported) exit 9
            if (!insert) insert=j9
            if (insert>j9) exit 9
            for(row=1;row<=NR;row++) {
                if(row==insert) print "BOOTCLASSPATH=\"$BOOTCLASSPATH -Xbootclasspath/p:" prepend kept "\""
                if(!remove[row]) print lines[row]
            }
        }
    ' "$LSD" > "$STAGE/lsd.sh.patched" || fail "BOOTCLASSPATH_STRUCTURE_UNSUPPORTED"
fi

if [ -n "${CPMAP_TEST_ROOT:-}" ]; then
    /bin/sh -n "$STAGE/lsd.sh.patched" >/dev/null 2>&1 || fail "PATCHED_LSD_SYNTAX_FAILED"
else
    /bin/ksh -n "$STAGE/lsd.sh.patched" >/dev/null 2>&1 || fail "PATCHED_LSD_SYNTAX_FAILED"
fi

cp "$CORE" "$NEW/core.so" || fail "CORE_COPY_FAILED"
cp "$WRAPPER" "$NEW/dio_manager.wrapper.sh" || fail "WRAPPER_COPY_FAILED"
cp "$BRIDGE" "$NEW/altscreen-most-bridge" || fail "BRIDGE_COPY_FAILED"
cp "$SUPERVISOR" "$NEW/carplay-cluster-supervisor" || fail "SUPERVISOR_COPY_FAILED"
cp "$SEC" "$NEW/libcarplay-security-bridge.so" || fail "SESSION_BRIDGE_COPY_FAILED"
cp "$PAIR_HELPER" "$NEW/security_bridge/carplay-pairing-helper" || fail "PAIR_HELPER_COPY_FAILED"
cp "$PAIR_CORE" "$NEW/security_bridge/libCarPlayPairingCore.so" || fail "PAIR_CORE_COPY_FAILED"
cp "$MIRROR" "$NEW/cpvc-mirror-canvas" || fail "MIRROR_COPY_FAILED"
cp "$TAP" "$NEW/libcpvc-mirror-tap.so" || fail "TAP_COPY_FAILED"
cp "$QUERY" "$NEW/libcpvc-dmdt-query.so" || fail "QUERY_COPY_FAILED"
for name in VCMenu.jar VCMirrorRate.jar; do
    if [ -e "$ROOT/rollback/$name.absent" ]; then
        touch "$NEW/rollback/$name.absent" || fail "MENU_BACKUP_MARK_FAILED"
    elif [ -s "$ROOT/rollback/$name" ]; then
        cp "$ROOT/rollback/$name" "$NEW/rollback/$name" || fail "MENU_BACKUP_FAILED"
    elif [ -e "$JARS/$name" ]; then
        cp "$JARS/$name" "$NEW/rollback/$name" || fail "MENU_BACKUP_FAILED"
    else
        touch "$NEW/rollback/$name.absent" || fail "MENU_BACKUP_MARK_FAILED"
    fi
done
echo "$VERSION" > "$NEW/VERSION"
echo "$DETECTED_MU/$INSTRUMENT" > "$NEW/PROFILE"
touch "$NEW/CLUSTER_AUTO_ENABLE"

chmod 755 "$NEW/dio_manager.wrapper.sh" "$NEW/altscreen-most-bridge" "$NEW/carplay-cluster-supervisor" "$NEW/security_bridge/carplay-pairing-helper" "$NEW/stock_first/dio_manager.real"
chmod 644 "$NEW/core.so" "$NEW/libcarplay-security-bridge.so" "$NEW/security_bridge/libCarPlayPairingCore.so" "$NEW/VERSION" "$NEW/PROFILE" "$NEW/CLUSTER_AUTO_ENABLE"
chmod 755 "$NEW/cpvc-mirror-canvas"
chmod 644 "$NEW/libcpvc-mirror-tap.so" "$NEW/libcpvc-dmdt-query.so"

cp "$CONTROL_JAR" "$JARS/CarPlayClusterControls.jar.new" || fail "CONTROL_JAR_STAGE_FAILED"
cp "$MENU_JAR" "$JARS/VCMenu.jar.new" || fail "MENU_JAR_STAGE_FAILED"
cp "$RATE_JAR" "$JARS/VCMirrorRate.jar.new" || fail "RATE_JAR_STAGE_FAILED"
cp "$STAGE/lsd.sh.patched" "$LSD.cpmap-new" || fail "LSD_ACTIVATION_STAGE_FAILED"
chmod 644 "$JARS/CarPlayClusterControls.jar.new"
chmod 644 "$JARS/VCMenu.jar.new" "$JARS/VCMirrorRate.jar.new"
chmod 755 "$LSD.cpmap-new"
if [ "$NAV_INSTALL" = yes ]; then
    cp "$NAV_JAR" "$NAV_TARGET.new" || fail "NAV_JAR_STAGE_FAILED"
    chmod 644 "$NAV_TARGET.new"
fi

if [ -d "$ROOT" ]; then
    mv "$ROOT" "$OLD" || fail "OLD_ROOT_MOVE_FAILED"
fi
mv "$NEW" "$ROOT" || {
    [ -d "$OLD" ] && mv "$OLD" "$ROOT" 2>/dev/null
    fail "NEW_ROOT_ACTIVATION_FAILED"
}

rollback_commit() {
    cp "$STAGE/dio_manager.current" "$DIO" 2>/dev/null || true
    cp "$STAGE/lsd.sh.current" "$LSD" 2>/dev/null || true
    for name in VCMenu.jar VCMirrorRate.jar; do
        if [ -e "$STAGE/$name.current" ]; then
            cp "$STAGE/$name.current" "$JARS/$name" 2>/dev/null || true
        else
            rm -f "$JARS/$name" 2>/dev/null
        fi
    done
    if [ -s "$STAGE/control.current" ]; then
        cp "$STAGE/control.current" "$JARS/CarPlayClusterControls.jar" 2>/dev/null || true
    else
        rm -f "$JARS/CarPlayClusterControls.jar" 2>/dev/null
    fi
    if [ -s "$STAGE/nav.current" ]; then
        cp "$STAGE/nav.current" "$NAV_TARGET" 2>/dev/null || true
    else
        rm -f "$NAV_TARGET" 2>/dev/null
    fi
    rm -rf "$ROOT" 2>/dev/null
    [ -d "$OLD" ] && mv "$OLD" "$ROOT" 2>/dev/null
}

mv "$JARS/CarPlayClusterControls.jar.new" "$JARS/CarPlayClusterControls.jar" || { rollback_commit; fail "CONTROL_JAR_ACTIVATION_FAILED"; }
mv "$JARS/VCMenu.jar.new" "$JARS/VCMenu.jar" || { rollback_commit; fail "MENU_JAR_ACTIVATION_FAILED"; }
mv "$JARS/VCMirrorRate.jar.new" "$JARS/VCMirrorRate.jar" || { rollback_commit; fail "RATE_JAR_ACTIVATION_FAILED"; }
if [ "$NAV_INSTALL" = yes ]; then
    mv "$NAV_TARGET.new" "$NAV_TARGET" || { rollback_commit; fail "NAV_JAR_ACTIVATION_FAILED"; }
fi
mv "$LSD.cpmap-new" "$LSD" || { rollback_commit; fail "LSD_ACTIVATION_FAILED"; }
cp "$ROOT/dio_manager.wrapper.sh" "$DIO.new" || { rollback_commit; fail "DIO_WRAPPER_STAGE_FAILED"; }
chmod 755 "$DIO.new"
mv "$DIO.new" "$DIO" || { rollback_commit; fail "DIO_WRAPPER_ACTIVATION_FAILED"; }

rm -f "$ROOT/cpmap-guard" "$ROOT/cpmap-auth" "$ROOT/cpmap-provision" "$ROOT/intro.eft" "$ROOT"/*.eft "$ROOT"/k1 "$ROOT"/k2 2>/dev/null

[ -s "$DIO" ] || fail "DIO_WRAPPER_VERIFY_FAILED"
[ -s "$ROOT/core.so" ] || fail "CORE_VERIFY_FAILED"
[ -s "$ROOT/stock_first/dio_manager.real" ] || fail "STOCK_VERIFY_FAILED"
[ -s "$JARS/CarPlayClusterControls.jar" ] || fail "CONTROL_JAR_VERIFY_FAILED"
run_nav_check --same "$MENU_JAR" "$JARS/VCMenu.jar" || { rollback_commit; fail "MENU_VERIFY_FAILED"; }
run_nav_check --same "$RATE_JAR" "$JARS/VCMirrorRate.jar" || { rollback_commit; fail "RATE_VERIFY_FAILED"; }
active_reference_count() {
    awk -v name="$1" '/^[ \t]*#/ {next} index($0,name) {n++} END {print n+0}' "$LSD"
}
[ `active_reference_count 'CarPlayClusterControls.jar'` -eq 1 ] || fail "BOOTCLASSPATH_VERIFY_FAILED"
[ `active_reference_count 'VCMenu.jar'` -eq 1 ] || { rollback_commit; fail "MENU_CLASSPATH_VERIFY_FAILED"; }
[ `active_reference_count 'VCMirrorRate.jar'` -eq 1 ] || { rollback_commit; fail "RATE_CLASSPATH_VERIFY_FAILED"; }
if [ "$NAV_INSTALL" = yes ]; then
    navigation_payload_intact "$NAV_TARGET" && run_nav_check --same "$NAV_JAR" "$NAV_TARGET" || { rollback_commit; fail "NAVIGATION_VERIFY_FAILED"; }
    [ `active_reference_count "$NAV_NAME"` -eq 1 ] || { rollback_commit; fail "NAVIGATION_CLASSPATH_VERIFY_FAILED"; }
fi
rm -rf "$OLD" "$STAGE" 2>/dev/null

sync 2>/dev/null || true
echo "CPMAP_PROFILE=$DETECTED_MU/$INSTRUMENT"
echo "CPMAP_PLAINTEXT_ROOT=/mnt/app/carplaymaps"
echo "CPMAP_INSTALL_OK"
echo "A full unit reboot is required."
exit 0
