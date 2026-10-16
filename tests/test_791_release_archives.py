"""All six toolbox ZIPs must differ only in selected 791 cores."""
import argparse
import hashlib
from pathlib import Path
import zipfile

REPO = Path(__file__).resolve().parents[1]
CORE_SHA = 'f3a6d2575772307f8007c70057836df449af1509ba06438333baccb612d55826'


def contents(path):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None, path
        files = [n for n in archive.namelist() if not n.endswith('/')]
        assert len(files) == len(set(files)), 'Duplicate ZIP member'
        assert all(not n.startswith('/') and '..' not in n.split('/') for n in files)
        return {n: hashlib.sha256(archive.read(n)).hexdigest() for n in files}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('previous_assets', type=Path)
    parser.add_argument('build_root', type=Path)
    args = parser.parse_args()
    assets = args.build_root/'release-assets'
    zips = sorted(assets.glob('*.zip'))
    assert len(zips) == 6
    unchanged = updated = 0
    for new_zip in zips:
        old_zip = args.previous_assets/new_zip.name
        old, new = contents(old_zip), contents(new_zip)
        assert old.keys() == new.keys(), 'Toolbox file list changed'
        delta = {n for n in old if old[n] != new[n]}
        if 'MQB' in new_zip.name:
            expected = {f'Custom/carplay_maps_vc/payload/profiles/{mu}/791/core.so'
                        for mu in ('MU1102','MU1367')}
        elif '791.794' in new_zip.name:
            mu = 'MU1102' if 'MU1102' in new_zip.name else 'MU1367'
            expected = {f'CarPlayMapsVC/payload/profiles/{mu}/791/core.so'}
        else:
            expected = set()
            assert old_zip.read_bytes() == new_zip.read_bytes(), '790 ZIP bytes changed'
        assert delta == expected, (new_zip.name, delta ^ expected)
        for name in delta:
            assert new[name] == CORE_SHA
        if delta: updated += 1
        else: unchanged += 1
        print('PASS full archive CRC and exact scope', new_zip.name, len(delta))
    assert updated == unchanged == 3
    records = (assets/'FULL_TOOLBOX_SHA256.txt').read_text().splitlines()
    assert len(records) == 6
    for record in records:
        expected, name = record.split('  ', 1)
        assert expected == hashlib.sha256((assets/name).read_bytes()).hexdigest()
    assert (assets/'GITHUB_RELEASE_NOTES.md').read_bytes() == (REPO/'releases/toolbox-plaintext-v1.2.0.md').read_bytes()
    print('791_FULL_TOOLBOX_ARCHIVE_QA_OK updated=3 unchanged=3')


if __name__ == '__main__':
    main()
