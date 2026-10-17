# CarPlay Maps VC v1.1.0

> 历史记录：请安装 [v1.2.1 MQB 修复版](./toolbox-plaintext-v1.2.1.md)。本版导航补丁检查存在车机兼容性问题，M.I.B. 自动刷写包已撤下，下文仅保留历史说明。
>
> Historical record: use [the v1.2.1 MQB fix](./toolbox-plaintext-v1.2.1.md). The old navigation check has a target-compatibility defect; M.I.B. automatic-flashing packages are withdrawn. The text below is historical only.

## 中文更新说明

本次更新集成导航前置补丁，并增加 MU1440 斯柯达方向盘滑轮缩放。其他功能与 v1.0.0 保持一致，仍提供完整 MQB Toolbox 和五个固定固件/仪表组合的完整 M.I.B. Toolbox 压缩包，解压到 SD 卡根目录即可使用。

### 自动安装 navigation 导航补丁

未安装导航补丁时，CarPlay 开始导航可能出现“已在移动设备中激活”，随后仪表地图被关闭。安装器现在检查 Java 启动加载配置和补丁类：

- 缺少补丁时自动安装独立的 `CarPlayNavigationPatch.jar`，并添加加载入口。
- 已有完整补丁时保留原文件与加载行，包括合并了信息映射的 JAR。
- 支持原厂没有 `-Xbootclasspath/p:` 补丁行的 `lsd.sh`。
- 覆盖安装不重复添加入口；恢复时只撤销本项目添加的补丁，保留安装前已有补丁。
- 不完整的第三方 CarPlay 类覆盖补丁会安全停止，避免覆盖其他功能。

这个补丁只解除外部导航导致原厂地图关闭的限制，不包含导航文字、转向指引、音乐或封面信息映射。

### MU1440 斯柯达滑轮控制

- 上拨一格：地图放大。
- 下拨一格：地图缩小。
- 滑轮按压：不映射。
- 仍仅支持 MU1440 Škoda 的 790 仪表组合。其他固件的控制 JAR 不变。

手机地图 App 必须支持缩放指令。高德地图已验证可用，百度地图不支持该操作。

### 安装与验证

安装入口不变。MQB 安装成功后手动完整重启；M.I.B. 由原 SWDL 流程负责重启。请核对固件、仪表并使用 `FULL_TOOLBOX_SHA256.txt` 校验下载。不要混用不同 profile 的 M.I.B. 包。

本次通过五个组合的离线首次安装、重复安装、原厂恢复、已有/缺失/损坏导航补丁和错误固件停止测试，以及滑轮信号回放测试。各组合仍需独立实车验证；离线测试不代表已完成本次更新的所有实车测试。

30 fps 显示基线、790 原有导航位置、视频传输、工具箱入口、免费开源范围和独立付费支持内容均未修改。

## English release notes

This release integrates the navigation prerequisite patch and adds MU1440 Skoda steering-wheel roller zoom. All other behavior remains as in v1.0.0. Downloads still contain a complete MQB Toolbox and five complete, profile-locked M.I.B. Toolbox distributions, ready to extract to the SD-card root.

### Automatic navigation patch

Without the prerequisite patch, starting CarPlay navigation may display “activated on the mobile device” and close the cluster map. The installer now checks Java boot loading and patch classes:

- Installs a separate `CarPlayNavigationPatch.jar` and its boot entry when missing.
- Preserves an existing complete patch, including JARs merged with metadata mapping.
- Supports factory `lsd.sh` without an existing `-Xbootclasspath/p:` patch entry.
- Avoids duplicate entries on reinstall; restoration removes only the patch added by this project and preserves pre-existing patches.
- Stops safely for incomplete third-party CarPlay class overrides instead of overwriting other functions.

This patch removes the external-navigation map shutdown restriction. It does not add navigation text, maneuver guidance, music metadata or cover art.

### MU1440 Skoda roller control

- One upward detent: zoom in.
- One downward detent: zoom out.
- Roller press: not mapped.
- MU1440 Skoda remains restricted to the 790 cluster profile. Other firmware control JARs are unchanged.

The phone map app must accept zoom commands. Amap was verified; Baidu Maps does not support this operation.

### Installation and verification

Entry points are unchanged. Perform a full reboot after a successful MQB installation; M.I.B. retains its normal SWDL reboot workflow. Check the firmware and cluster, verify downloads against `FULL_TOOLBOX_SHA256.txt`, and never mix M.I.B. profiles.

Offline tests cover all five profiles: first install, reinstall, factory restoration, existing/missing/damaged navigation patches and wrong-firmware stops. Roller event replay tests also passed. Each profile still needs independent live validation; offline results are not a claim that every combination has been tested on a vehicle for this release.

The 30 fps display baseline, existing 790 navigation position, video transport, toolbox entry points, free open-source scope and separate paid-support contents are unchanged.

## Third party component notice

The bundled navigation JAR is copied byte-for-byte from the navigation patch already distributed with the [MQB MIB2 Toolbox](https://github.com/jilleb/mib2-toolbox). It is not replaced by a music/navigation metadata component. Upstream toolbox files and license notices remain intact; the repository GPLv3 notice does not relicense third-party or vendor components.

MU1440 roller source, compilation stubs and host tests are included under `src/steering-mu1440`. Stubs are build-only and are not packed into the runtime JAR. Navigation detection and installation source is under `src/toolbox`, with offline tests under `tests`.
