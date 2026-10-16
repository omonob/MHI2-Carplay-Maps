# v1.2.0 packaging validation

Date: 2026-10-16. These are offline packaging results, not new vehicle tests.

- Exact input core: 75,099 bytes, SHA-256 `00b48a8fe876e6ff405124b87ed9eace1bcb079220ff7f02b866f42676477a4a`.
- Updated core: 75,099 bytes, SHA-256 `f3a6d2575772307f8007c70057836df449af1509ba06438333baccb612d55826`.
- The only differing bytes are offsets `0x4370` and `0x452c`, both `0x14` to `0x05` in the two `mov r2` instructions.
- The 40 fps display declaration and all other bytes remain unchanged. MU1102/791 and MU1367/791 receive identical updated cores.
- `tests/test_791_idr5.py`: transformation, scope, damaged/incorrect/already-updated input rejection and 48 unrelated runtime files unchanged passed. All original 790 profile files remain identical to the prior commit.
- `tests/test_toolbox_install.py`: all five profiles passed first installation, repeated installation, restoration, existing/missing/incomplete navigation-patch cases, wrong firmware and failed-commit rollback fixtures.
- `tests/test_791_release_archives.py`: all six ZIPs passed full reads and CRC, matching file lists and exact allowed changed-file sets. The two 791 M.I.B. ZIPs differ in one core each, MQB differs in two cores, and the three 790 ZIP files are byte-identical to v1.1.0.
- All six download hashes are listed in `releases/CarPlay_Maps_VC_Full_Toolboxes/FULL_TOOLBOX_SHA256.txt` and the matching Release attachment.

The update reuses the existing published binary and applies a checked parameter transformation. No proprietary QNX compiler, licensing component or head-unit connection was used. No vehicle was modified or rebooted during packaging.
