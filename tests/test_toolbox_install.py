"""Host-side shell fixture tests; never connects to a head unit."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

REPO = Path(__file__).resolve().parents[1]
PROFILES = [("MU1102", "791", 55915411), ("MU1102", "790", 55915411),
            ("MU1367", "791", 55840541), ("MU1367", "790", 55840541),
            ("MU1440", "790", 55840933), ("MU1433", "790", 52751492),
            ("MU1447", "790", 55712574)]
_CHECKER_DIR = tempfile.TemporaryDirectory(prefix="cpmap-host-checker-")
_CHECKER = Path(_CHECKER_DIR.name) / "cpmap-nav-check"


def host_checker():
    if not _CHECKER.exists():
        subprocess.run(["/usr/bin/gcc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                        str(REPO / "src/toolbox/cpmap-nav-check.c"), "-o", str(_CHECKER)], check=True)
    return str(_CHECKER)


def package(mu, cluster):
    brand={"MU1440":"MU1440_SKODA","MU1433":"MU1433_SKODA","MU1447":"MU1447_SEAT"}.get(mu,mu)
    return REPO / brand / (
        "791_AID12.3" if cluster == "791" else "790_AID10.5")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def navigation_name(mu):
    return "CarPlayMapsG11Navigation.jar" if mu == "MU1433" else "CarPlayNavigationPatch.jar"


def navigation_payload(payload, mu, cluster):
    name = navigation_name(mu)
    return payload / ("profiles/" + mu + "/" + cluster if mu == "MU1433" else "common") / name


def run(script, root, *args, ok=True, extra_env=None):
    result = subprocess.run(["sh", str(script), *map(str, args)],
                            env={**os.environ, "CPMAP_TEST_ROOT": str(root),
                                 "CPMAP_TEST_NAV_CHECK": host_checker(), **(extra_env or {})},
                            capture_output=True, text=True)
    if (result.returncode == 0) != ok:
        raise AssertionError(result.stdout + result.stderr)
    return result.stdout


def fixture(root, size, mode, nav_payload):
    jxe = root / "ifs/lsd.jxe"
    jxe.parent.mkdir(parents=True)
    with jxe.open("wb") as output:
        output.truncate(size)
    app = root / "mnt/app"
    dio = app / "eso/bin/apps/dio_manager"
    dio.parent.mkdir(parents=True)
    dio.write_bytes(b"factory-dio" * 65000)
    lsd = app / "eso/hmi/lsd/lsd.sh"
    jars = lsd.parent / "jars"
    jars.mkdir(parents=True)
    text = '#!/bin/sh\nBASE_DIR=/mnt/app/eso/hmi\nBOOTCLASSPATH=""\n'
    if mode in ("existing", "merged", "missing-file", "partial"):
        name = "ExistingMergedPatch.jar" if mode == "merged" else "NavActiveIgnore.jar"
        text += 'BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/' + name + '"\n'
        if mode in ("existing", "merged"):
            (jars / name).write_bytes(nav_payload.read_bytes())
        elif mode == "partial":
            with zipfile.ZipFile(jars / "NavActiveIgnore.jar", "w") as output:
                output.writestr("de/esolutions/fw/comm/dsi/carplay/impl/DSICarplayProxy.class", b"custom")
    if mode == "bad-startup":
        text += "echo unsupported-startup\n"
    else:
        text += '$J9 $BOOTCLASSPATH Main\n'
    lsd.write_text(text)
    (root / "ramdisk").mkdir()
    return dio, lsd, jars


def main():
    for mu, cluster, size in PROFILES:
        pkg = package(mu, cluster)
        payload = pkg / "payload"
        nav = navigation_payload(payload, mu, cluster)
        with zipfile.ZipFile(nav) as archive:
            assert archive.testzip() is None
            assert len([n for n in archive.namelist() if n.endswith(".class")]) == (4 if mu == "MU1433" else 9)
            for name in archive.namelist():
                if name.endswith(".class"):
                    assert int.from_bytes(archive.read(name)[6:8], "big") == 46
        control = payload / "common/CarPlayClusterControls.jar"
        with zipfile.ZipFile(control) as archive:
            assert archive.testzip() is None
            for name in archive.namelist():
                if name.endswith(".class"):
                    expected_major = 46 if name.startswith("org/omonob/") else 48
                    assert int.from_bytes(archive.read(name)[6:8], "big") == expected_major, name
            assert not any("AbstractDispatcher" in n or "IReplyService" in n for n in archive.namelist())
        for mode in ("stock", "existing", "merged", "missing-file", "partial", "bad-startup"):
            with tempfile.TemporaryDirectory(prefix="cpmap-offline-") as tmp:
                root = Path(tmp)
                dio, lsd, jars = fixture(root, size, mode, nav)
                before_dio, before_lsd = digest(dio), digest(lsd)
                if mode == "bad-startup":
                    run(pkg / "install.sh", root, payload, mu, cluster, ok=False)
                    assert digest(dio) == before_dio and digest(lsd) == before_lsd
                    continue
                output = run(pkg / "install.sh", root, payload, mu, cluster)
                assert "CPMAP_INSTALL_OK" in output
                installed_nav = jars / navigation_name(mu)
                if mode in ("existing", "merged"):
                    assert "KEEP_EXISTING" in output and not installed_nav.exists()
                    existing = jars / ("ExistingMergedPatch.jar" if mode == "merged" else "NavActiveIgnore.jar")
                    assert digest(existing) == digest(nav)
                else:
                    assert "INSTALL_MISSING" in output and digest(installed_nav) == digest(nav)
                if mode == "partial":
                    with zipfile.ZipFile(jars / "NavActiveIgnore.jar") as archive:
                        assert archive.read(archive.namelist()[0]) == b"custom"
                    assert lsd.read_text().index(navigation_name(mu)) < lsd.read_text().index("NavActiveIgnore.jar")
                first_lsd = digest(lsd)
                run(pkg / "install.sh", root, payload, mu, cluster)
                assert digest(lsd) == first_lsd
                assert lsd.read_text().count("CarPlayClusterControls.jar") == 1
                assert lsd.read_text().count("-Xbootclasspath/p:") == 1
                run(pkg / "restore-carplay-maps.sh", root)
                assert digest(dio) == before_dio and digest(lsd) == before_lsd
                assert not installed_nav.exists()
                assert not (jars / "CarPlayClusterControls.jar").exists()
                if mode in ("existing", "merged"):
                    assert digest(existing) == digest(nav)
            print("PASS", mu, cluster, mode)
        with tempfile.TemporaryDirectory(prefix="cpmap-wrong-") as tmp:
            root = Path(tmp)
            dio, lsd, _ = fixture(root, size + 1, "stock", nav)
            before = digest(dio), digest(lsd)
            run(pkg / "install.sh", root, payload, mu, cluster, ok=False)
            assert before == (digest(dio), digest(lsd))
        with tempfile.TemporaryDirectory(prefix="cpmap-failed-commit-") as tmp:
            root = Path(tmp)
            dio, lsd, jars = fixture(root, size, "stock", nav)
            before = digest(dio), digest(lsd)
            command_dir = root / "commands"
            command_dir.mkdir()
            fake_mv = command_dir / "mv"
            fake_mv.write_text('#!/bin/sh\ncase "$1" in */'+navigation_name(mu)+'.new) exit 99;; esac\nexec /bin/mv "$@"\n')
            fake_mv.chmod(0o755)
            run(pkg / "install.sh", root, payload, mu, cluster, ok=False,
                extra_env={"PATH": str(command_dir) + ":" + os.environ["PATH"]})
            assert before == (digest(dio), digest(lsd))
            assert not (jars / "CarPlayClusterControls.jar").exists()
            assert not (jars / navigation_name(mu)).exists()
            assert not (root / "mnt/app/carplaymaps").exists()
    print("TOOLBOX_NAVIGATION_OFFLINE_TEST_OK")


if __name__ == "__main__":
    main()
