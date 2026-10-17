"""Exercise the real installer with a text-only grep, not GNU binary grep.

No head unit is contacted. The shim models the documented QNX grep boundary;
it is deliberately not described as an execution of the QNX binary itself.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

from test_toolbox_install import PROFILES, REPO, digest, fixture, package, run


STRICT_GREP = r'''#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import sys
files = [a for a in sys.argv[1:] if not a.startswith('-') and Path(a).is_file()]
data = b''.join(Path(a).read_bytes() for a in files) if files else sys.stdin.buffer.read()
if b'\0' in data:
    with open(os.environ['CPMAP_STRICT_GREP_LOG'], 'a') as output:
        output.write('binary-input: ' + repr(files) + '\n')
    sys.stderr.write('text-only grep: binary input is unsupported\n')
    sys.exit(2)
result = subprocess.run(['/bin/grep', *sys.argv[1:]], input=None if files else data)
sys.exit(result.returncode)
'''


def strict_environment(root):
    commands = root / "commands"
    commands.mkdir(exist_ok=True)
    grep = commands / "grep"
    grep.write_text(STRICT_GREP)
    grep.chmod(0o755)
    log = root / "binary-grep.log"
    return {"PATH": str(commands) + ":" + os.environ["PATH"],
            "CPMAP_STRICT_GREP_LOG": str(log)}, log


def baseline_failure(archive):
    """Reproduce the reported marker using the actually published old archive."""
    with tempfile.TemporaryDirectory(prefix="cpmap-v120-repro-") as tmp:
        base = Path(tmp)
        with zipfile.ZipFile(archive) as source:
            source.extractall(base / "sd")
        payload = base / "sd/Custom/carplay_maps_vc/payload"
        nav = payload / "common/CarPlayNavigationPatch.jar"
        with zipfile.ZipFile(nav) as source:
            assert source.testzip() is None
        for mu, cluster, size in PROFILES:
            root = base / (mu + cluster)
            dio, lsd, _ = fixture(root, size, "stock", nav)
            before = digest(dio), digest(lsd)
            env, log = strict_environment(root)
            output = run(payload / "common/install-carplay-maps.sh", root,
                         payload, mu, cluster, ok=False, extra_env=env)
            assert "CPMAP_INSTALL_FAIL=NAVIGATION_PAYLOAD_INVALID" in output
            assert log.is_file() and "binary-input" in log.read_text()
            assert before == (digest(dio), digest(lsd))
            print("REPRODUCED v1.2.0", mu, cluster, "NAVIGATION_PAYLOAD_INVALID")


def exercise(payload, script, restore, mu, cluster, size):
    nav = payload / "common/CarPlayNavigationPatch.jar"
    for mode in ("stock", "existing", "merged", "missing-file", "own-corrupt", "partial", "bad-startup"):
        with tempfile.TemporaryDirectory(prefix="cpmap-text-grep-") as tmp:
            root = Path(tmp)
            dio, lsd, jars = fixture(root, size, mode, nav)
            own_before = None
            if mode == "own-corrupt":
                text = lsd.read_text().replace('$J9 $BOOTCLASSPATH Main',
                    'BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/CarPlayNavigationPatch.jar"\n$J9 $BOOTCLASSPATH Main')
                lsd.write_text(text)
                broken = bytearray(nav.read_bytes())
                broken[256] ^= 1
                (jars / "CarPlayNavigationPatch.jar").write_bytes(broken)
                own_before = digest(jars / "CarPlayNavigationPatch.jar")
            before = digest(dio), digest(lsd)
            env, log = strict_environment(root)
            if mode in ("partial", "bad-startup"):
                output = run(script, root, payload, mu, cluster, ok=False, extra_env=env)
                expected = "CUSTOM_CARPLAY_PATCH_REQUIRES_REVIEW" if mode == "partial" else "BOOTCLASSPATH_STRUCTURE_UNSUPPORTED"
                assert expected in output, output
                assert before == (digest(dio), digest(lsd))
            else:
                output = run(script, root, payload, mu, cluster, extra_env=env)
                assert "CPMAP_INSTALL_OK" in output
                installed = jars / "CarPlayNavigationPatch.jar"
                if mode in ("existing", "merged"):
                    assert "KEEP_EXISTING" in output and not installed.exists()
                    existing = jars / ("ExistingMergedPatch.jar" if mode == "merged" else "NavActiveIgnore.jar")
                    assert digest(existing) == digest(nav)
                else:
                    assert "INSTALL_MISSING" in output and digest(installed) == digest(nav)
                    if mode == "own-corrupt":
                        assert "REPAIR_OWN_JAR" in output
                first_lsd = digest(lsd)
                run(script, root, payload, mu, cluster, extra_env=env)
                assert digest(lsd) == first_lsd
                assert lsd.read_text().count("-Xbootclasspath/p:") == 1
                run(restore, root, extra_env=env)
                assert before == (digest(dio), digest(lsd))
                if own_before:
                    assert digest(installed) == own_before
                else:
                    assert not installed.exists()
            assert not log.exists(), log.read_text() if log.exists() else ""
            print("PASS text-only grep", mu, cluster, mode)


def corrupt_payload_checks():
    pkg = package("MU1102", "791")
    for corruption in ("same-size", "truncated"):
        with tempfile.TemporaryDirectory(prefix="cpmap-corrupt-nav-") as tmp:
            root = Path(tmp) / "unit"
            payload = Path(tmp) / "payload"
            shutil.copytree(pkg / "payload", payload)
            nav = payload / "common/CarPlayNavigationPatch.jar"
            dio, lsd, _ = fixture(root, 55915411, "stock", nav)
            before = digest(dio), digest(lsd)
            data = bytearray(nav.read_bytes())
            if corruption == "same-size":
                data[256] ^= 1
            else:
                del data[-20:]
            nav.write_bytes(data)
            env, log = strict_environment(root)
            output = run(pkg / "install.sh", root, payload, "MU1102", "791", ok=False, extra_env=env)
            assert "NAVIGATION_PAYLOAD_CHECKSUM_MISMATCH" in output, output
            assert before == (digest(dio), digest(lsd))
            assert not (root / "mnt/app/carplaymaps").exists()
            assert not log.exists()
            print("PASS navigation integrity", corruption)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline-archive", type=Path)
    parser.add_argument("--baseline-only", action="store_true")
    parser.add_argument("--release-archive", type=Path)
    args = parser.parse_args()
    if args.baseline_archive:
        baseline_failure(args.baseline_archive)
    if args.baseline_only:
        assert args.baseline_archive
        return
    if args.release_archive:
        with tempfile.TemporaryDirectory(prefix="cpmap-mqb-release-test-") as tmp:
            root = Path(tmp)
            with zipfile.ZipFile(args.release_archive) as archive:
                assert archive.testzip() is None
                archive.extractall(root)
            payload = root / "Custom/carplay_maps_vc/payload"
            for mu, cluster, size in PROFILES:
                exercise(payload, payload / "common/install-carplay-maps.sh",
                         root / "Toolbox/scripts/cpmap_restore_core.sh", mu, cluster, size)
    else:
        for mu, cluster, size in PROFILES:
            pkg = package(mu, cluster)
            exercise(pkg / "payload", pkg / "install.sh", pkg / "restore-carplay-maps.sh", mu, cluster, size)
        corrupt_payload_checks()
    print("QNX_TEXT_ONLY_NAVIGATION_REGRESSION_OK")


if __name__ == "__main__":
    main()
