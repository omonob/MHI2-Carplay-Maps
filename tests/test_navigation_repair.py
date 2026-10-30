"""Installer edge cases: optional SD patch, old classpath duplicates, RCC dispatch."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from test_qnx_navigation import strict_environment
from test_toolbox_install import PROFILES, digest, fixture, host_checker, package, run, navigation_payload, navigation_name


def main():
    for mu, cluster, size in PROFILES:
        pkg = package(mu, cluster)
        for state in ("existing-no-sd", "existing-bad-sd", "partial", "commented", "multiple",
                      "own-and-foreign", "own-missing", "no-patch-no-sd", "rcc"):
            with tempfile.TemporaryDirectory(prefix="cpmap-nav-repair-") as tmp:
                base = Path(tmp)
                root, payload = base / "unit", base / "payload"
                shutil.copytree(pkg / "payload", payload)
                nav_name = navigation_name(mu)
                nav = navigation_payload(payload, mu, cluster)
                mode = "existing" if state.startswith("existing") or state == "own-and-foreign" else "partial" if state == "partial" else "stock"
                dio, lsd, jars = fixture(root, size, mode, nav)
                before = digest(dio), digest(lsd)
                foreign = jars / "NavActiveIgnore.jar"
                foreign_hash = digest(foreign) if foreign.exists() else None
                text = lsd.read_text()
                if state == "commented":
                    text = '# old CarPlayNavigationPatch.jar CarPlayClusterControls.jar\n' * 3 + text
                if state == "multiple":
                    (jars / "Unrelated.jar").write_bytes(b"unrelated metadata")
                    text = text.replace('$J9 $BOOTCLASSPATH Main',
                        'BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/Unrelated.jar"\n'
                        'BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/CarPlayClusterControls.jar:$BASE_DIR/lsd/jars/Unrelated.jar"\n'
                        '$J9 $BOOTCLASSPATH Main')
                if state in ("own-and-foreign", "own-missing"):
                    text = text.replace('$J9 $BOOTCLASSPATH Main',
                        'BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/'+nav_name+'"\n$J9 $BOOTCLASSPATH Main')
                    if state == "own-and-foreign":
                        (jars / nav_name).write_bytes(b"old broken owned jar")
                lsd.write_text(text)
                before = digest(dio), digest(lsd)
                if state in ("existing-no-sd", "no-patch-no-sd"):
                    nav.unlink()
                if state == "existing-bad-sd":
                    nav.write_bytes(b"damaged unused SD file")
                env, log = strict_environment(root)
                if state == "rcc":
                    on = root / "commands/on"
                    on.write_text('#!/bin/sh\n[ "$1" = -f ] && [ "$2" = mmx ] || exit 99\nshift 2\nshift\nexec "$CPMAP_HOST_BINARY" "$@"\n')
                    on.chmod(0o755)
                    env.update(CPMAP_TEST_NAV_CHECK="", CPMAP_HOST_BINARY=host_checker())
                output = run(pkg / "install.sh", root, payload, mu, cluster,
                             ok=state != "no-patch-no-sd", extra_env=env)
                if state == "no-patch-no-sd":
                    assert "PAYLOAD_MISSING_"+nav_name in output
                    assert before == (digest(dio), digest(lsd))
                    continue
                assert "CPMAP_INSTALL_OK" in output
                active = "\n".join(line for line in lsd.read_text().splitlines() if not line.lstrip().startswith('#'))
                assert active.count("-Xbootclasspath/p:") == 1
                assert active.count("CarPlayClusterControls.jar") == 1
                if state.startswith("existing") or state == "own-and-foreign":
                    assert "KEEP_EXISTING" in output
                    assert nav_name not in active
                else:
                    assert "INSTALL_MISSING" in output and active.count(nav_name) == 1
                if foreign_hash:
                    assert foreign_hash == digest(foreign)
                if state == "partial":
                    assert active.index(nav_name) < active.index("NavActiveIgnore.jar")
                if state == "multiple":
                    assert active.count("Unrelated.jar") == 1
                if state != "rcc":
                    first = digest(lsd)
                    run(pkg / "install.sh", root, payload, mu, cluster, extra_env=env)
                    assert first == digest(lsd)
                run(pkg / "restore-carplay-maps.sh", root, extra_env=env)
                assert before == (digest(dio), digest(lsd))
                assert not log.exists(), log.read_text() if log.exists() else ""
                print("PASS navigation repair", mu, cluster, state)
    print("NAVIGATION_AUTOMATIC_REPAIR_EDGE_CASES_OK")


if __name__ == "__main__":
    main()
