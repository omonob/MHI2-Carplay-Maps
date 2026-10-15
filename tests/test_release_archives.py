"""Verify that the update changes only navigation install, MU1440 controls and MU1427 support."""
import argparse
import hashlib
from pathlib import Path
import zipfile


def contents(path):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None, path
        return {name: hashlib.sha256(archive.read(name)).hexdigest()
                for name in archive.namelist() if not name.endswith("/")}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("build_root", type=Path)
    args = parser.parse_args()
    build = args.build_root
    old_dir = build / "previous/FULL_TOOLBOX_RELEASES"
    assets = build / "release-assets"
    count = 0
    for new_zip in sorted(assets.glob("*.zip")):
        name = new_zip.name
        if "MQB" in name:
            old_name = "CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip"
            allowed = {
                "Custom/carplay_maps_vc/payload/common/install-carplay-maps.sh",
                "Toolbox/scripts/cpmap_restore_core.sh",
                "Custom/carplay_maps_vc/payload/common/CarPlayNavigationPatch.jar",
                "Custom/carplay_maps_vc/payload/profiles/MU1440/790/CarPlayClusterControls.jar",
                "Custom/carplay_maps_vc/payload/profiles/MU1427/791/core.so",
                "Custom/carplay_maps_vc/payload/profiles/MU1427/791/CarPlayNavigationPatch.jar",
                "Modifications/Lsd/MU1427/NavActiveIgnore.jar",
                "Toolbox/scripts/cpmap_common.sh",
                "Toolbox/scripts/cpmap_install_791.sh",
                "Toolbox/scripts/cpmap_install_790.sh",
                "Toolbox/scripts/install_nav_active_ignore.sh",
                "Toolbox/scripts/uninstall_nav_active_ignore.sh",
            }
        else:
            mu = "MU1440" if name.startswith("SKODA") else ("MU1367" if "MU1367" in name else "MU1102")
            cluster = "791" if "791.794" in name else "790"
            suffix = f"{mu}_{cluster}" + ("_SKODA" if mu == "MU1440" else "")
            old_name = f"CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_{suffix}.zip"
            allowed = {"CarPlayMapsVC/install.sh",
                       "CarPlayMapsVC/payload/common/install-carplay-maps.sh",
                       "CarPlayMapsVC/payload/common/CarPlayNavigationPatch.jar"}
            if mu == "MU1440":
                allowed.add("CarPlayMapsVC/payload/common/CarPlayClusterControls.jar")
        old, new = contents(old_dir / old_name), contents(new_zip)
        assert not (old.keys() - new.keys()), "Existing files removed"
        delta = {n for n in old.keys() | new.keys() if old.get(n) != new.get(n)}
        assert delta == allowed, (name, delta ^ allowed)
        for filename in new:
            if filename.endswith(".sh") and filename in allowed:
                with zipfile.ZipFile(new_zip) as archive:
                    assert b"\r" not in archive.read(filename), filename
        print("PASS archive scope and CRC", name)
        count += 1
    assert count == 6
    for line in (assets / "FULL_TOOLBOX_SHA256.txt").read_text().splitlines():
        expected, filename = line.split("  ", 1)
        assert expected == hashlib.sha256((assets / filename).read_bytes()).hexdigest()
    print("FULL_TOOLBOX_ARCHIVE_QA_OK")


if __name__ == "__main__":
    main()
