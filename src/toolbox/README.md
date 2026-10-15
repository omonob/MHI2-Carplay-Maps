# Navigation prerequisite integration

`install-carplay-maps.sh` is the canonical installer copied to the five standalone profiles and six full toolbox archives. `restore-carplay-maps.sh` adds restoration of the separate navigation JAR and is used by the standalone packages and MQB restore entry.

The navigation prerequisite is the unchanged upstream MQB Toolbox `Modifications/Lsd/NavActiveIgnore.jar`, distributed here as `CarPlayNavigationPatch.jar`:

- Upstream: https://github.com/jilleb/mib2-toolbox/blob/master/Modifications/Lsd/NavActiveIgnore.jar
- Size: 27,179 bytes.
- SHA-256: `a4803f7fd008152aca19d4e00b8b45789ac18dccf4cf405d129ce02297982ffe`.
- Nine classes, upstream classfile major 46 (Java 1.2); no recompilation or bytecode conversion.

Detection examines active prepend-classpath entries and the seven navigation-ignore class entry names in the JAR ZIP directory. A filename alone is not sufficient. This is an offline prerequisite check, not screen-message recognition or a claim that the running HMI has been inspected. Complete existing patches are kept. An incomplete active override of `DSICarplayProxy` stops for review rather than risking another mapping component.

The installer extends the existing `-Xbootclasspath/p:` option, or creates one before `$J9` on stock scripts. It does not create multiple prepend options. Original startup state and navigation-JAR presence are retained for factory restoration. Failed activation uses an immediate pre-update snapshot so an already-installed version is not replaced by the older factory baseline during rollback.

Only MU1440 selects the optional profile-specific roller control JAR. All other profiles retain their previous control JAR. Firmware gates, native libraries, 30 fps release display behavior and installation entry points are unchanged.

导航补丁来自工具箱原有组件，按原字节分发，不加入信息映射功能。检测依据是启动加载行和 JAR 内补丁类，不是识别仪表提示截图。已有完整补丁保持原样；补装使用单独文件，并在原厂恢复时撤销。MU1440 以外的方向盘控制 JAR 与原版保持一致。

## MU1427

MU1427 (`lsd.jxe` 52,751,492 bytes) is supported with the 791/794 AID 12.3 cluster only.

- **Navigation patch:** the generic `NavActiveIgnore.jar` above stops CarPlay from starting on MU1427 (`NoSuchMethodError` in the CarPlay HMI target). MU1427 therefore uses its own build, `MU1427/791_AID12.3/payload/common/CarPlayNavigationPatch.jar`, made from MU1427's `lsd.jxe` classes with the tooling in `MU1427/NavActiveIgnore/`.
  - The installer prefers `payload/profiles/<MU>/<cluster>/CarPlayNavigationPatch.jar` when present.
  - On MU1427 it detects the patch by its own class set, including the helper class `de/vw/mib/navactiveignore/NavActiveIgnore.class`.
  - It stops with `GENERIC_NAVIGATION_PATCH_BREAKS_MU1427_CARPLAY` if the generic jar is already active.
- **`core.so`:** the MU1427 `core.so` fixes the alt-screen AES key offset for AirPlay 210.81. See `MU1427/README.md`.
- **MQB Toolbox build:** `tools/Build-ToolboxUpdate.ps1` adds the MU1427 `core.so` and navigation patch under `payload/profiles/MU1427/791/`. From `src/toolbox/mqb/` it also adds:
  - the menu scripts that detect MU1427 (`cpmap_common.sh`, and `cpmap_install_791.sh` / `cpmap_install_790.sh`, where 790 refuses MU1427);
  - the toolbox "Ignore navigation-active" scripts, which select `Modifications/Lsd/MU1427/NavActiveIgnore.jar` on MU1427, edit `lsd.sh` in place and coexist with the CarPlay Maps VC prepend line.
- **M.I.B. 3.6.0:** there is no MU1427 package.
