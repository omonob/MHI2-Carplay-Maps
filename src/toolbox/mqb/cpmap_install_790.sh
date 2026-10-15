#!/bin/ksh

SCRIPTS=/mnt/app/eso/hmi/engdefs/scripts/mqb
[ -s "$SCRIPTS/cpmap_common.sh" ] || SCRIPTS=/net/mmx/mnt/app/eso/hmi/engdefs/scripts/mqb
. "$SCRIPTS/cpmap_common.sh" || exit 1

echo "CarPlay Maps VC - 790 AID 10.5"
echo "Free open-source project by omonob (QCDWJ)"
find_cpmap_payload || exit 1
detect_cpmap_mu || exit 1
case "$CPMAP_MU" in
    MU1102|MU1367|MU1440) ;;
    MU1427) echo "FAIL: MU1427 supports the 791 package only"; exit 1 ;;
    *) exit 1 ;;
esac
/bin/ksh "$CPMAP_PAYLOAD/common/install-carplay-maps.sh" "$CPMAP_PAYLOAD" "$CPMAP_MU" 790
exit $?
