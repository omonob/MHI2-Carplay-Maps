#!/bin/bash
# Rebuild MU1427 NavActiveIgnore.jar from the factory SWDL package.
#
#   build.sh <MHI2_ER_..._MU1427 dir> <work dir>
#
# Needs: python3 + liblzo2, JDK 11+, ASM 9.x (asm, asm-tree, asm-analysis jars in $ASM_DIR),
#        jxe2jar (https://github.com/luka-dev/jxe2jar) in $JXE2JAR.
# Steps: LZOZ/LZO1Z -> QNX IFS -> /ifs/lsd.jxe -> classes (jxe2jar) -> ASM patch -> verify -> jar
set -euo pipefail
FW=$(cd "$1" && pwd); W=$(mkdir -p "$2" && cd "$2" && pwd); H=$(cd "$(dirname "$0")" && pwd)
: "${ASM_DIR:?}" "${JXE2JAR:?}"
CP="$ASM_DIR/asm-9.7.jar:$ASM_DIR/asm-tree-9.7.jar:$ASM_DIR/asm-analysis-9.7.jar"
CLASSES="de/esolutions/fw/comm/dsi/carplay/impl/DSICarplayProxy de/vw/mib/asl/internal/carplay/common/CarPlayModeHandling
 de/vw/mib/asl/internal/navigation/api/impl/ASLNavigationServices de/vw/mib/asl/internal/androidauto/target/NavigationListener
 de/vw/mib/asl/internal/androidauto/target/RequestHandler"

python3 -I "$H/extract/unlzoz.py" "$FW/MMX2/mifs-stage2/50/default/mifs-stage2.img" "$W/ifs.bin"
(cd "$W" && python3 -I "$H/extract/ifs.py" ifs.bin ifs/lsd.jxe)
[ "$(stat -c %s "$W/lsd.jxe")" = 52751492 ] || { echo "not MU1427 lsd.jxe"; exit 1; }
python3 "$JXE2JAR/src/jxe2jar.py" "$W/lsd.jxe" "$W/base.jar" > "$W/jxe2jar.log"
rm -rf "$W/stock" "$W/helper" "$W/out" "$W/pbin"; mkdir -p "$W/stock" "$W/helper" "$W/out" "$W/pbin"
(cd "$W/stock" && for c in $CLASSES; do unzip -oq ../base.jar "$c.class"; done)
javac --release 8 -nowarn -cp "$W/base.jar" -d "$W/helper" "$H/src/de/vw/mib/navactiveignore/NavActiveIgnore.java"
javac -cp "$CP" -d "$W/pbin" "$H/patcher/Patch.java" "$H/patcher/Verify.java"
java -cp "$CP:$W/pbin" Patch "$W/stock" "$W/helper" "$W/out"
java -cp "$CP:$W/pbin" Verify "$W/out" "$W/base.jar" $(find "$W/out" -name '*.class')
printf 'Manifest-Version: 1.0\nCreated-By: NavActiveIgnore MU1427 (ASM patch of stock lsd.jxe classes)\n' > "$W/MANIFEST.MF"
(cd "$W/out" && jar cfm "$W/NavActiveIgnore.jar" "$W/MANIFEST.MF" $(find . -name '*.class' | sort))
echo "built $W/NavActiveIgnore.jar"
