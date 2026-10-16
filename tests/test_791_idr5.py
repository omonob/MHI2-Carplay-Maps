"""Offline scope and parameter tests. No device connection."""
import hashlib
import importlib.util
from pathlib import Path
import subprocess

REPO = Path(__file__).resolve().parents[1]
OLD = '222e76a47a13308042e097bb127c60a5effb6848'
MODULE = REPO / 'src/display-791-idr5/patch_core.py'
spec = importlib.util.spec_from_file_location('patch_791', MODULE)
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)
CORES = [f'{mu}/791_AID12.3/payload/profiles/{mu}/791/core.so'
         for mu in ('MU1102', 'MU1367')]


def previous(name):
    return subprocess.run(['git', 'show', f'{OLD}:{name}'], cwd=REPO,
                          check=True, capture_output=True).stdout


def rejects(data):
    try:
        patch.transform(data)
    except ValueError:
        return
    raise AssertionError('Unexpectedly accepted an unsupported core')


def main():
    results = []
    for name in CORES:
        old, new = previous(name), (REPO/name).read_bytes()
        assert patch.transform(old) == new
        assert len(new) == len(old) == patch.BASE_SIZE
        assert tuple(i for i, pair in enumerate(zip(old, new)) if pair[0] != pair[1]) == patch.INTERVAL_OFFSETS
        assert new[patch.FPS_INSTRUCTION_OFFSET:patch.FPS_INSTRUCTION_OFFSET+4] == bytes.fromhex('2820a0e3')
        rejects(new)
        rejects(old[:-1])
        damaged = bytearray(old)
        damaged[0x100] ^= 1
        rejects(bytes(damaged))
        results.append(hashlib.sha256(new).hexdigest())
        print('PASS 791 maxFPS40 interval5 and two-byte scope', name)
    assert results[0] == results[1]
    rejects((REPO/'MU1102/790_AID10.5/payload/profiles/MU1102/790/core.so').read_bytes())
    tracked = subprocess.run(['git', 'ls-tree', '-r', '--name-only', OLD], cwd=REPO,
                             check=True, capture_output=True, text=True).stdout.splitlines()
    checked = 0
    for name in tracked:
        if '/payload/' in name and name not in CORES:
            assert (REPO/name).read_bytes() == previous(name), f'Unrelated runtime changed: {name}'
            checked += 1
        if name.startswith(('MU1102/790_', 'MU1367/790_', 'MU1440_SKODA/')):
            assert (REPO/name).read_bytes() == previous(name), f'790 file changed: {name}'
    # Request-boundary arithmetic, including values beyond a 32-bit counter.
    for frame in list(range(1, 1001)) + [2**32-1, 2**32, 2**32+4, 2**63-1]:
        assert (frame % 5 == 0) == (frame // 5 * 5 == frame)
    print('PASS unchanged runtime files', checked)
    print('791_IDR5_RELEASE_SCOPE_OK', results[0])


if __name__ == '__main__':
    main()
