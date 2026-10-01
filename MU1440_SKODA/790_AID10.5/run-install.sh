#!/bin/ksh

BASE="${0%/*}"
[ "$BASE" = "$0" ] && BASE="."
exec /bin/ksh "$BASE/install.sh" "$BASE/payload" MU1440 790
