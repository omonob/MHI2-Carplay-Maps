#!/bin/sh
# Fixture-only Linux host tests. The explicit sync shim is never distributed.
set -eu
cd /src
export PATH=/src/tests/fixtures/host-only:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
chmod +x /src/tests/fixtures/host-only/sync
python3 tests/test_toolbox_install.py
python3 tests/test_v2_install.py
python3 tests/test_g11_navigation.py
python3 tests/test_navigation_repair.py
python3 tests/test_nav_checker.py
python3 tests/test_g11_release_scope.py
python3 tests/test_v2_archive.py "$1"
echo G11_PRERELEASE_HOST_REGRESSION_PASS_NO_NETWORK
