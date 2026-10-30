#!/bin/ksh

if [ -n "${CPMAP_TEST_ROOT:-}" ] && [ -d "$CPMAP_TEST_ROOT/mnt/app" ] && [ -s "$CPMAP_TEST_ROOT/ifs/lsd.jxe" ]; then
    MMX="$CPMAP_TEST_ROOT"
elif [ -d /mnt/app ] && [ -s /ifs/lsd.jxe ]; then
    MMX=""
elif [ -d /net/mmx/mnt/app ] && [ -s /net/mmx/ifs/lsd.jxe ]; then
    MMX="/net/mmx"
else
    echo "Status: MMX filesystem not found"
    exit 1
fi

ROOT="$MMX/mnt/app/carplaymaps"
JXE="$MMX/ifs/lsd.jxe"
line=`ls -ln "$JXE" 2>/dev/null`; set -- $line
echo "CarPlay Maps VC status"
echo "lsd.jxe size: $5"
if [ -s "$ROOT/VERSION" ]; then
    echo "Version: `cat "$ROOT/VERSION"`"
    echo "Profile: `cat "$ROOT/PROFILE" 2>/dev/null`"
    echo "Runtime root: /mnt/app/carplaymaps"
    echo "Encryption watchdog: not installed"
    echo "Startup animation: not installed"
    echo "Music and navigation metadata: not installed"
else
    echo "Status: not installed"
fi
exit 0
