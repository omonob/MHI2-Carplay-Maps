# MQB navigation installer validation for v1.2.2

Date: 2026-10-19. No head-unit connection, deployment or reboot in this pass.

## Reproduced root cause

The exact published v1.2.1 ZIP, SHA-256 `f72d14a82518c1a27754c842d362fc29c9df1e8c355ed4b6acd4e8bb73b0dd21`, rejects its intact navigation JAR on all five profiles when `cksum` is unavailable: `NAVIGATION_PAYLOAD_CHECKSUM_MISMATCH_REEXTRACT_SD`. Target-tool absence was previously misclassified as SD corruption. The older text-only binary-grep failure remains covered by the reproduction test. The old installer also checked the SD patch before inspecting an existing complete patch and blocked incomplete custom overrides rather than completing the prerequisite.

## Passed v1.2.2 checks

- `tests/test_nav_checker.py`: native source compiled on the host both normally and with undefined-behavior/bounds sanitizers; valid payload, complete/partial/duplicate entries, text masquerading as ZIP, truncated/malformed offsets and lengths, corruption, byte comparison, and deterministic invalid-input cases pass.
- `tests/test_toolbox_install.py`: five supported profiles; install/reinstall/restore, complete/merged/partial/missing navigation patch, wrong-firmware and unsupported-startup rejection, and activation-failure rollback.
- `tests/test_navigation_repair.py`: all five profiles; an installed complete patch works with missing/damaged unused SD patch, partial patch completion leaves foreign JAR bytes unchanged, comments/duplicate standard classpath assignments normalize to one active prepend, a stale owned reference is repaired/removed as appropriate, first backup survives repeat install, and the RCC path dispatches the checker to MMX in the fixture. A missing genuinely required SD patch still fails without changing factory files.
- `tests/test_qnx_navigation.py --baseline-archive <v1.2.1.zip>`: reproduces the old failure and passes new installation/reinstallation/restoration with text-only grep and unavailable `cksum`, `tr`, `strings`, `unzip`; rejects same-size/truncated required SD payloads before activation.
- `tests/test_qnx_navigation.py --release-archive <v1.2.2.zip>`: all five profiles and seven states pass using the installer actually extracted from the final MQB archive.
- `tests/test_mqb_release.py`: 326-file ZIP, CRC/read checks, no duplicate entries; exactly one existing installer changed and one checker added. All 324 other existing files, including cores, bridge, supervisor, control/navigation JARs and toolbox files, remain byte-identical to v1.2.1. All standalone installer/helper copies match the archive.
- `tests/test_mqb_docs.py`: author attribution and all six paid-support/license/safety sections unchanged; MQB-only distribution and public description policy preserved.
- PowerShell scripts parse under Windows PowerShell 5.1; MQB archive construction succeeds there. Shell fixture syntax checks pass. `git diff --check` passes.

## ARM checker and final artifact

Built with approved open GCC 4.9.4 in `qnx65-armv7:4.9`, without original SDK compiler/activation tools. Verified ELF32 little-endian ARM, EABI5, interpreter `/usr/lib/ldqnx.so.2`, only `libc.so.3` needed. Checker size: 9,977 bytes; SHA-256 `06609d9e533ea44a3a2be6ee15db20a48d277926d15f0d6fcc5f37a73a0c7b02`.

Final `VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A_v1.2.2.zip` SHA-256: `06955ade05e461b26ec2fdb78631378b690fefb64b1ffa672b802caf55061ab5`.

The navigation JAR is unchanged: SHA-256 `a4803f7fd008152aca19d4e00b8b45789ac18dccf4cf405d129ce02297982ffe`, 27,179 bytes, standard CRC32 `b6c892dc`. The helper checks existing class directory entries, not third-party bytecode semantics or runtime HMI behavior. Host fixtures and cross-build ABI inspection do not claim fresh QNX or live-vehicle execution. Actual missing/corrupt files, unsupported firmware, unsupported startup layouts and write failures are deliberately not suppressed.

---

# Historical validation for v1.2.1 (superseded)

Date: 2026-10-16. Scope: MQB installation only; no head-unit connection, deployment or reboot was performed in this packaging pass.

## Failure reproduction

`tests/test_qnx_navigation.py --baseline-archive <published-v1.2.0-MQB.zip> --baseline-only` reproduces `CPMAP_INSTALL_FAIL=NAVIGATION_PAYLOAD_INVALID` for MU1102/AID 12.3, MU1102/AID 10.5, MU1367/AID 12.3, MU1367/AID 10.5 and MU1440/AID 10.5. The original bundled JAR passes ZIP CRC validation. A text-only grep shim rejects binary input to model the target utility boundary; it is not a QNX emulator.

The old installation code passed the binary JAR directly to grep. The fix retains the exact upstream JAR and its checksum, translates binary surroundings to text with QNX-compatible tr operands, and searches that text rather than the binary file. This addresses the reproduced false-negative without disabling payload validation.

## Passed regression checks

- `tests/test_toolbox_install.py`: all five supported profiles, initial/repeat install, existing complete or merged patch, missing referenced patch, wrong firmware, unsupported startup, incomplete custom override, failed activation rollback and factory restore.
- `tests/test_qnx_navigation.py`: all five profiles under the text-only grep restriction; includes automatic repair of a damaged project-owned navigation JAR and rejection of a truncated or same-size-corrupted SD payload.
- `tests/test_qnx_navigation.py --release-archive <v1.2.1-MQB.zip>`: the same installation/restoration cases run from the actually packaged installer, not a separate repository copy.
- `tests/test_mqb_release.py <v1.2.0-MQB.zip> <build-root>`: full ZIP CRC/read checks, no duplicate entries, all five cores match the repository and previous release, exactly one changed archive file, and one ZIP plus checksum/notes only.
- `tests/test_mqb_docs.py`: current downloads and build route are MQB-only; author attribution and all six paid-support, licensing and safety sections are unchanged; no detailed refresh-request cadence is added to public descriptions.
- `git diff --check`: clean.

## Package scope

The complete MQB archive contains 325 files. Only `Custom/carplay_maps_vc/payload/common/install-carplay-maps.sh` changes against the published v1.2.0 archive. The other 324 files, including every native core, video bridge, supervisor, control JAR and navigation JAR, remain byte-identical.

v1.2.1 MQB ZIP SHA-256: `f72d14a82518c1a27754c842d362fc29c9df1e8c355ed4b6acd4e8bb73b0dd21`.

The shipped navigation JAR remains SHA-256 `a4803f7fd008152aca19d4e00b8b45789ac18dccf4cf405d129ce02297982ffe`, POSIX cksum `3499852279`, length `27179`. Unknown incomplete overrides from other projects are deliberately not overwritten. These checks do not claim fresh live verification on the reported vehicle.
