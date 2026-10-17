# MQB navigation installer validation for v1.2.1

Date: 2026-10-16. Scope: MQB installation only; no head-unit connection, deployment or reboot was performed in this packaging pass.

## Failure reproduction

`tests/test_qnx_navigation.py --baseline-archive <published-v1.2.0-MQB.zip> --baseline-only` reproduces `CPMAP_INSTALL_FAIL=NAVIGATION_PAYLOAD_INVALID` for MU1102/791, MU1102/790, MU1367/791, MU1367/790 and MU1440/790. The original bundled JAR passes ZIP CRC validation. A text-only grep shim rejects binary input to model the target utility boundary; it is not a QNX emulator.

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
