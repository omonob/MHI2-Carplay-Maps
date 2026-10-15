# MU1427 standalone profile

Firmware: `MHI2_ER_VWG11_K3342_MU1427` (HMI H28.28.210, framework 5.41.10.SR5 MIB2CLU6).
Exact firmware identity: `/ifs/lsd.jxe` = **52,751,492 bytes** (identical on hardware variants 50 and 70).

| Cluster | Profile files | Display baseline |
| --- | --- | --- |
| 791/794 AID 12.3 | [`791_AID12.3`](./791_AID12.3/) | 40 fps, navigation centered |

This directory contains only the CarPlay Maps VC project files. It is not a complete MQB or M.I.B. Toolbox distribution. 790 AID 10.5 is not supported on MU1427, and the installer stops with `MU1427_REQUIRES_791`.

Validated in a vehicle on 2026-10-13 (MU1427, VC 791, iPhone over USB):
- CarPlay starts.
- The CarPlay map shows in the virtual cockpit.
- The OEM map still loads when no phone is connected.
- Steering-wheel controls work.

## What differs from MU1367/791

Installer, control JAR, wrapper, bridge, supervisor and pairing files are the shared ones. Two payload files are MU1427-specific.

### `core.so`: alt-screen AES key offset

MU1427 ships an older AirPlay stack: `libairplay.so` is 857,735 bytes, sourceVersion 210.81.

`core.so` derives the cluster stream's (type 111) AES key and IV with the stock call `AirPlay_DeriveAESKeySHA512ForScreen(session + offset, 16, streamConnectionID, key, iv)`:

- The shared 791 build uses offset `0x1c8`, which matches the `libairplay.so` of MU1102/MU1367.
- MU1427's own receiver passes `session + 0x1c0` for the main screen stream (`AirPlayReceiverSessionSetup`: `add r1, r6, #0x1c0` at 0x231b0, and the derive call at 0x23310).

With `0x1c8`, the iPhone negotiates the stream and sends video, but every frame decrypts to garbage. The video log shows `alt-aes-decrypt-invalid-avcc`, `alt-stream-ready` never appears, and the MOST bridge never starts.

`payload/profiles/MU1427/791/core.so` is the MU1367/791 build with one byte changed:

| | |
| --- | --- |
| File offset | `0x5284` |
| Instruction before | `add r0, r8, #0x1c8` (`e2880f72`) |
| Instruction after | `add r0, r8, #0x1c0` (`e2880f70`) |
| sha256 before | `00b48a8f…` |
| sha256 after | `46da2689…` |

This is the only hard-coded session-struct offset in `core.so`. Everything else is resolved through exported API calls. [`791_AID12.3/patch-core-mu1427.py`](./791_AID12.3/patch-core-mu1427.py) reapplies the patch and verifies both hashes.

### `CarPlayNavigationPatch.jar`: MU1427 build of the navigation prerequisite

The navigation prerequisite installed for the other profiles is the MQB Toolbox `NavActiveIgnore.jar`. It replaces whole HMI classes that were built for a newer firmware:

- Its `CarPlayModeHandling` has a 6-argument constructor, which takes `ExboxGuidanceListenerImpl`.
- MU1427's `HsmTarget` calls the 5-argument one, so the CarPlay HMI target fails with `NoSuchMethodError` and no CarPlay session can start.
- The jar was also recompiled from decompiler output with byte-swapped int constants.

[`NavActiveIgnore/`](./NavActiveIgnore/) rebuilds the patch from MU1427's own `lsd.jxe` classes, with ASM, pattern-asserted and verified. [`NavActiveIgnore/README.md`](./NavActiveIgnore/README.md) covers the patches, the build, and why the original cluster data-rate change is left out.

The canonical installer:
- uses `payload/profiles/<MU>/<cluster>/CarPlayNavigationPatch.jar` when present (the MQB layout), otherwise `payload/common/` (this standalone profile);
- recognizes the MU1427 build by its helper class `de/vw/mib/navactiveignore/NavActiveIgnore.class`;
- stops with `GENERIC_NAVIGATION_PATCH_BREAKS_MU1427_CARPLAY` if the generic jar is active on MU1427.

## Offline verification against the MU1427 SWDL package

| Check | Result |
| --- | --- |
| `libairplay.so` exports every symbol `core.so` resolves | OK |
| `CarPlayClusterControls.jar` override of `DSIKeyPanelDispatcher` vs MU1427 `lsd.jxe` | Compatible |
| Factory `lsd.sh` (no `-Xbootclasspath/p:`; single `$J9 … -jxe` launch) | Patched with one prepend line before `$J9`; restore is byte-identical |
| `/eso/bin/apps/dmdt`, `/dev/mlb/isoTX2`, videoovermost | Present |
| `libcarplay-security-bridge.so` fingerprint | Doesn't match (`HTTPConnectionCreate` moved). The bridge logs `patch_failed_fingerprint` and passes through unchanged. It is observe-only, so this is harmless. |

`tests/test_toolbox_install.py` includes MU1427.

固件识别条件：`/ifs/lsd.jxe` 精确大小为 **52,751,492 bytes**，仅支持 791/794 AID 12.3 仪表。MU1427 使用专用 `core.so`（修正仪表视频流 AES 密钥偏移）和专用导航补丁；工具箱原有的通用 `NavActiveIgnore.jar` 会导致 MU1427 无法启动 CarPlay。目录内仅为 CarPlay Maps VC 独立项目文件，不含完整工具箱。
