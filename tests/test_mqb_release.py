"""Verify the MQB-only hotfix changes exactly one installer, no native payloads."""
import argparse
import hashlib
from pathlib import Path
import zipfile

from test_toolbox_install import PROFILES, REPO, package

NAME = "VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip"
INSTALLER = "Custom/carplay_maps_vc/payload/common/install-carplay-maps.sh"


def files(path):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == len(set(archive.namelist())), "duplicate ZIP entries"
        return {n: archive.read(n) for n in archive.namelist() if not n.endswith("/")}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("previous_archive", type=Path)
    parser.add_argument("build_root", type=Path)
    args = parser.parse_args()
    assets = args.build_root / "release-assets"
    assert {p.name for p in assets.iterdir()} == {NAME, "FULL_TOOLBOX_SHA256.txt", "GITHUB_RELEASE_NOTES.md"}
    old = files(args.previous_archive)
    new = files(assets / NAME)
    assert old.keys() == new.keys(), "SD file inventory changed"
    changed = {n for n in old if old[n] != new[n]}
    assert changed == {INSTALLER}, changed
    assert new[INSTALLER] == (REPO / "src/toolbox/install-carplay-maps.sh").read_bytes()
    assert b"\r" not in new[INSTALLER]
    assert b"tr -c" in new[INSTALLER]
    assert b"NAVIGATION_PAYLOAD_INVALID" not in new[INSTALLER]
    assert b"TOOLBOX-1.2.1" in new[INSTALLER]
    for mu, cluster, _ in PROFILES:
        source = package(mu, cluster)
        for relative in ("install.sh", "payload/common/install-carplay-maps.sh"):
            assert (source / relative).read_bytes() == new[INSTALLER], relative
        core_path = f"Custom/carplay_maps_vc/payload/profiles/{mu}/{cluster}/core.so"
        assert new[core_path] == (source / f"payload/profiles/{mu}/{cluster}/core.so").read_bytes()
        print("PASS unchanged native core", mu, cluster)
    record = (assets / "FULL_TOOLBOX_SHA256.txt").read_text().strip()
    assert record == hashlib.sha256((assets / NAME).read_bytes()).hexdigest() + "  " + NAME
    assert (assets / "GITHUB_RELEASE_NOTES.md").read_bytes() == (REPO / "releases/toolbox-plaintext-v1.2.1.md").read_bytes()
    assert not any("MIB_Toolbox" in n or n.startswith("CarPlayMapsVC/") for n in new)
    print("MQB_ONLY_ARCHIVE_CRC_SCOPE_AND_CHECKSUM_OK", len(new), "files; one installer changed")


if __name__ == "__main__":
    main()
