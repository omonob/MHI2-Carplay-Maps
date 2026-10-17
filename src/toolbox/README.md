# Navigation prerequisite integration

`install-carplay-maps.sh` is the canonical installer copied to the five standalone profiles and the single complete MQB Toolbox archive. M.I.B. automatic-flashing distributions and their build entry points are withdrawn. `restore-carplay-maps.sh` restores the separate navigation JAR and is used by the standalone packages and MQB restore entry.

The navigation prerequisite is the unchanged upstream MQB Toolbox `Modifications/Lsd/NavActiveIgnore.jar`, distributed here as `CarPlayNavigationPatch.jar`:

- Upstream: https://github.com/jilleb/mib2-toolbox/blob/master/Modifications/Lsd/NavActiveIgnore.jar
- Size: 27,179 bytes.
- SHA-256: `a4803f7fd008152aca19d4e00b8b45789ac18dccf4cf405d129ce02297982ffe`.
- Nine classes, upstream classfile major 46 (Java 1.2); no recompilation or bytecode conversion.

The bundled JAR is pinned to POSIX `cksum` **3499852279**, length **27179**. Detection examines active prepend-classpath entries and all seven navigation-ignore class entry names. QNX `grep` must not receive binary JAR input: `LC_ALL=C tr -c '\40-\176' '\12'` converts non-printable bytes to newlines before fixed-string text matching. These octal/range operands are documented for [QNX 6.5 tr](https://www.qnx.com/developers/docs/6.5.0SP1/neutrino/utilities/t/tr.html); see also the [QNX binary-grep limitation](https://qdn.qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.utilities/topic/g/grep.html). No target `strings`, `unzip` or GNU `grep -a` dependency is added.

A filename alone is not sufficient. This is an offline prerequisite check, not screen-message recognition or a claim that the running HMI has been inspected. Complete existing patches are kept. An incomplete active override of `DSICarplayProxy` stops for review rather than risking another mapping component. A damaged `CarPlayNavigationPatch.jar` owned by this project is repaired from the intact bundled copy.

The installer extends the existing `-Xbootclasspath/p:` option, or creates one before `$J9` on stock scripts. It does not create multiple prepend options. Original startup state and navigation-JAR presence are retained for factory restoration. Failed activation uses an immediate pre-update snapshot so an already-installed version is not replaced by the older factory baseline during rollback.

Only MU1440 selects the optional profile-specific roller control JAR. All other profiles retain their previous control JAR. Firmware gates, native libraries, display parameters and MQB installation entry points are unchanged.

`tests/test_qnx_navigation.py` uses a text-only grep shim to reproduce `NAVIGATION_PAYLOAD_INVALID` from the published v1.2.0 MQB ZIP. It tests the fixed standalone scripts and the actual new ZIP, including install/reinstall/restore and damaged-payload rejection. This is host-side regression coverage, not execution of QNX itself. `tools/Build-MQBRelease.ps1` builds only the MQB hotfix; `tests/test_mqb_release.py` requires the archive to differ from v1.2.0 in exactly the canonical installer entry and verifies every profile core remains unchanged.

导航补丁来自工具箱原有组件，按原字节分发，不加入信息映射功能。检测依据是启动加载行和 JAR 内补丁类，不是识别仪表提示截图。已有完整补丁保持原样；补装使用单独文件，并在原厂恢复时撤销。MU1440 以外的方向盘控制 JAR 与原版保持一致。
