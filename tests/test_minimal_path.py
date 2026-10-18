"""Run the real installer on a host fixture whose PATH lacks tr and cksum.

Reproduces a live MU1367/791 Tiguan report: under the MQB Toolbox GEM shell
neither `tr` nor `cksum` resolve, so v1.2.1 stopped with
NAVIGATION_PAYLOAD_CHECKSUM_MISMATCH_REEXTRACT_SD (cksum) and then
NAVIGATION_PAYLOAD_CLASSES_MISSING_REEXTRACT_SD (tr) although the SD payload
was byte-identical to the release. No head unit is contacted.
"""
import os
from pathlib import Path
import shutil
import tempfile

from test_toolbox_install import PROFILES, digest, fixture, package, run

# Every external command the installer and restore script need, minus tr/cksum.
AVAILABLE = ["sh", "ls", "grep", "awk", "cp", "mv", "rm", "mkdir", "chmod",
             "touch", "basename", "sync", "cat"]


def minimal_env(root, drop=()):
    commands = root / "minimal-bin"
    commands.mkdir()
    for name in AVAILABLE:
        if name in drop:
            continue
        target = shutil.which(name)
        assert target, name
        (commands / name).symlink_to(target)
    for name in ("tr", "cksum"):
        assert not (commands / name).exists()
    return {"PATH": str(commands)}


def exercise(mu, cluster, size):
    pkg = package(mu, cluster)
    payload = pkg / "payload"
    script = payload / "common/install-carplay-maps.sh"
    restore = pkg / "restore-carplay-maps.sh"
    nav = payload / "common/CarPlayNavigationPatch.jar"
    for mode in ("stock", "existing", "missing-file", "partial", "unknown-jar"):
        with tempfile.TemporaryDirectory(prefix="cpmap-minimal-path-") as tmp:
            root = Path(tmp)
            fixture_mode = "stock" if mode == "unknown-jar" else mode
            dio, lsd, jars = fixture(root, size, fixture_mode, nav)
            if mode == "unknown-jar":
                (jars / "OtherProject.jar").write_bytes(b"PK\x03\x04" + b"x" * 4000)
                lsd.write_text(lsd.read_text().replace(
                    '$J9 $BOOTCLASSPATH Main',
                    'BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/OtherProject.jar"\n'
                    '$J9 $BOOTCLASSPATH Main'))
            before = digest(dio), digest(lsd)
            env = minimal_env(root)
            if mode in ("partial", "unknown-jar"):
                output = run(script, root, payload, mu, cluster, ok=False, extra_env=env)
                assert "CUSTOM_CARPLAY_PATCH_REQUIRES_REVIEW" in output, output
                assert "CPMAP_UNKNOWN_BOOT_JAR=" in output, output
                assert before == (digest(dio), digest(lsd))
                assert not (root / "mnt/app/carplaymaps").exists()
            else:
                output = run(script, root, payload, mu, cluster, extra_env=env)
                assert "CPMAP_TOOLS_OK (tr=no)" in output, output
                assert "cksum unavailable, size check used" in output, output
                assert "CPMAP_INSTALL_OK" in output, output
                installed = jars / "CarPlayNavigationPatch.jar"
                if mode == "existing":
                    assert "KEEP_EXISTING" in output and not installed.exists()
                else:
                    assert "INSTALL_MISSING" in output and digest(installed) == digest(nav)
                first_lsd = digest(lsd)
                run(script, root, payload, mu, cluster, extra_env=env)
                assert digest(lsd) == first_lsd
                assert lsd.read_text().count("-Xbootclasspath/p:") == 1
                run(restore, root, extra_env=env)
                assert before == (digest(dio), digest(lsd))
                assert not installed.exists()
            print("PASS minimal PATH (no tr, no cksum)", mu, cluster, mode)


def wrong_size_payload_is_rejected():
    pkg = package("MU1367", "791")
    with tempfile.TemporaryDirectory(prefix="cpmap-minimal-corrupt-") as tmp:
        root = Path(tmp) / "unit"
        payload = Path(tmp) / "payload"
        shutil.copytree(pkg / "payload", payload)
        nav = payload / "common/CarPlayNavigationPatch.jar"
        dio, lsd, _ = fixture(root, 55840541, "stock", nav)
        before = digest(dio), digest(lsd)
        data = bytearray(nav.read_bytes())
        del data[-20:]
        nav.write_bytes(data)
        output = run(payload / "common/install-carplay-maps.sh", root, payload,
                     "MU1367", "791", ok=False, extra_env=minimal_env(root))
        assert "NAVIGATION_PAYLOAD_CHECKSUM_MISMATCH" in output, output
        assert before == (digest(dio), digest(lsd))
        assert not (root / "mnt/app/carplaymaps").exists()
        print("PASS minimal PATH rejects truncated navigation payload")


def missing_required_tool_changes_nothing():
    pkg = package("MU1367", "791")
    payload = pkg / "payload"
    with tempfile.TemporaryDirectory(prefix="cpmap-minimal-notool-") as tmp:
        root = Path(tmp)
        dio, lsd, _ = fixture(root, 55840541, "stock", payload / "common/CarPlayNavigationPatch.jar")
        before = digest(dio), digest(lsd)
        output = run(payload / "common/install-carplay-maps.sh", root, payload,
                     "MU1367", "791", ok=False, extra_env=minimal_env(root, drop=("awk",)))
        assert "CPMAP_MISSING_TOOLS= awk" in output, output
        assert "REQUIRED_TOOLS_MISSING_NOTHING_CHANGED" in output, output
        assert before == (digest(dio), digest(lsd))
        assert not (root / "mnt/app/carplaymaps").exists()
        print("PASS missing required tool stops before any change")


def main():
    for mu, cluster, size in PROFILES:
        exercise(mu, cluster, size)
    wrong_size_payload_is_rejected()
    missing_required_tool_changes_nothing()
    print("MINIMAL_PATH_REGRESSION_OK")


if __name__ == "__main__":
    main()
