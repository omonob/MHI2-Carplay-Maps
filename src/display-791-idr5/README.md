# 791 40 FPS display optimization

This update preserves the existing public 791 native core and applies a limited display-parameter adjustment. It does not import an encrypted/private installer, another profile, a diagnostic queue experiment, metadata mapping or startup animation.

## Exact baseline

- Public commit: `222e76a47a13308042e097bb127c60a5effb6848`.
- Input files: `MU1102/791_AID12.3/payload/profiles/MU1102/791/core.so` and the equivalent MU1367/791 core. The two files are identical.
- Input size: 75,099 bytes.
- Input SHA-256: `00b48a8fe876e6ff405124b87ed9eace1bcb079220ff7f02b866f42676477a4a`.
- The existing native display declaration already uses **800x480 and maxFPS=40**. Earlier top-level documentation saying 30 fps was stale.

## Minimal reproducible transformation

`patch_core.py` checks the entire input SHA-256 and ELF header before writing anything. The implementation and exact parameter transformation remain available in that source file.

| File offset | Purpose |
| --- | --- |
| `0x4370` | Divisor supplied to the 64-bit remainder check for periodic requests |
| `0x452c` | Matching interval value passed to the periodic-request log |

Only the immediate byte at each offset changes. File size, layout, symbols, dependencies, geometry, map controls, video decryption and shared-ring behavior remain unchanged. The `maxFPS=40` instruction at `0x2a70` is explicitly checked and not modified. This is a parameter transformation of an already-published executable, not a new compilation; embedded build metadata remains that of the baseline. SHA-256 identifies the new result.

The tool does not change encoded H.264 frame contents. `maxFPS=40` is a declaration, not proof of a measured 40 displayed frames per second.

## Reproduce and verify

Obtain the exact baseline core from the commit above, then run:

```text
python src/display-791-idr5/patch_core.py baseline-core.so core-idr5.so
python tests/test_791_idr5.py
```

No QNX compiler, licence change, device access or installation is performed. Unknown binaries, 790 cores, damaged inputs and an already-updated core are rejected as transformation inputs. Tests additionally compare all unchanged profile files against the previous public commit.

The display optimization was previously exercised in the author's test series. Packaging checks for this release are offline; they do not claim a new live test on every MU/phone combination. 790 and MU1440 Skoda remain unchanged.

中文：791 保持已有的 800×480、40 fps 声明和导航居中，不导入私有加密版或低延迟探针。工具先核对整个旧文件哈希，再进行限定范围的参数调整，其他内容保持不变。实现代码和校验记录继续保留，便于复现与审查。
