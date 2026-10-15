#!/bin/ksh

SCRIPTS=/mnt/app/eso/hmi/engdefs/scripts/mqb
[ -s "$SCRIPTS/cpmap_common.sh" ] || SCRIPTS=/net/mmx/mnt/app/eso/hmi/engdefs/scripts/mqb
. "$SCRIPTS/cpmap_common.sh" || exit 1

echo "CarPlay Maps VC - 791 AID 12.3"
echo "Free open-source project by omonob (QCDWJ)"
find_cpmap_payload || exit 1
detect_cpmap_mu || exit 1
case "$CPMAP_MU" in
    MU1102|MU1367|MU1427) ;;
    MU1440) echo "FAIL: MU1440 supports the 790 package only"; exit 1 ;;
    *) exit 1 ;;
esac
/bin/ksh "$CPMAP_PAYLOAD/common/install-carplay-maps.sh" "$CPMAP_PAYLOAD" "$CPMAP_MU" 791
exit $?
