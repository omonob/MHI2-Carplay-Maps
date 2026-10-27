# CarPlay Maps VC for MHI2

Free open-source CarPlay maps and screen mirroring for Volkswagen and Škoda virtual cockpits. Author: [omonob 汽车大玩家](https://github.com/omonob).

大众与斯柯达 MHI2 的免费开源 CarPlay 地图和屏幕镜像项目。开源软件本身及 GPLv3 授予的权利不收费。

[Downloads 下载](https://github.com/omonob/MHI2-Carplay-Maps/releases/latest) · [2.0 release notes 中英文更新说明](releases/toolbox-plaintext-v2.0.0.md) · [GPLv3](LICENSE)

[开发进度、图片与帧率／延迟数据](docs/development/2026-10-28/README.md) · [Development report, images and measurements](docs/development/2026-10-28/README_EN.md)

## 中文说明

### 2.0 虚拟座舱设置

App-Connect 设置里增加独立的“虚拟座舱设置”入口，保留原厂 MirrorLink 页面。菜单使用原厂选项条和连续滚动列表，包含 31 种语言，设置在重启后保留。

| 设置 | 功能 |
| --- | --- |
| 虚拟座舱地图 | CarPlay Maps 或原车地图 |
| 虚拟座舱镜像 | CarPlay 屏幕镜像开启／关闭，地图优先，冲突项置灰 |
| 无线 CarPlay | 测试状态接口，默认关闭；实际无线服务未发布 |
| 导航卡片 | AID 10.5：中间、靠左、靠右、关闭；AID 12.3：开启、关闭 |

共用菜单保留 AA／CarLife 名称，但本包不包含对应引擎，相关选项不可选择。镜像缩放按键只调整画布，不改变仪表输出尺寸。独立导航文字、音乐信息、封面和启动动画不包含在此免费包内。

### 支持范围

| 固件 | lsd.jxe 大小 | AID 12.3 | AID 10.5 |
| --- | ---: | --- | --- |
| MU1102 | 55,915,411 bytes | 800×480 | 1010×376 |
| MU1367 | 55,840,541 bytes | 800×480 | 1010×376 |
| MU1440 Škoda | 55,840,933 bytes | 不支持 | 1010×376，保留滑轮方向 |

AID 10.5 卡片核心沿用已上机黄金基线，Waze 卡片仍有已知布局问题。AID 12.3 使用实验安全区 `(220,65,360,348)`，底边保持 413；该最终参数尚待实车确认，不能当作已通过实车验证的黄金版。完整 2.0 包并未在所有车型上验证。

AID 12.3 保留 40 fps 协商声明与每 5 帧请求一次 IDR。AID 10.5 卡片基线声明上限为 60 fps；AID 10.5 CP 镜像模式将原厂 10 fps 编码请求改为 30 fps。协商／请求值不等于所有手机和应用的实测显示帧率。

### 安装

1. 保存本机系统备份，车辆停稳并持续供电，完全断开 CarPlay。
2. 从 Releases 下载完整 MQB Toolbox ZIP，解压到 SD 卡根目录，用随包 SHA-256 文件校验。
3. 进入 MQB Toolbox → Customization → CarPlay Maps VC，按真实仪表选择 AID 10.5 或 AID 12.3。
4. 安装器精确识别固件，已有有效导航共存补丁直接保留；缺少或本项目副本损坏则自动补齐。
5. 看到 `CPMAP_INSTALL_OK` 后，通过工具箱流程完整重启，再打开 App-Connect 设置 → 虚拟座舱设置。

只提供 MQB 完整工具箱，不提供 M.I.B. 自动刷写。安装失败不要连接手机测试。原厂恢复入口恢复保存的启动文件和 JAR，独立保存的用户设置不删除。已有加密版本请使用其专用迁移工具，公共安装器不会拆除它。

完整包以外的五种独立文件位于 [MU1102](MU1102/)、[MU1367](MU1367/)、[MU1440 Škoda](MU1440_SKODA/)，适用于自行集成工具箱的开发者。

### 联系和可选支持

需要未适配固件的免费版本，可联系作者并提供系统备份；备份可能包含身份和授权数据，请私下发送，不要上传到公开 Issue。

邮箱：[omonob@126.com](mailto:omonob@126.com) · Telegram：[omonob](https://t.me/omonob)。

原有额外启动 Logo、导航／歌曲信息映射等可选付费支持与本仓库免费授权分开。[闲鱼付费选项](https://www.goofish.com/item?id=1076254792602)，原标价 ¥399 RMB；具体功能、固件和服务范围购买前联系确认。

## English

### Version 2.0

An independent Virtual Cockpit settings page is added under App-Connect settings without repurposing MirrorLink. It uses native selector rows and continuous scrolling. CRC-checked preferences survive reboots, with a previous valid copy available for recovery. Labels cover 31 locales; rendering uses the firmware's installed fonts.

The four selectors control VC Maps, VC Mirroring, the unreleased Wireless CarPlay test interface, and Navigation Card. Maps take priority over mirroring for the same source. Only the CarPlay engines are included; Android Auto and CarLife choices are disabled. The wireless switch is off by default and does not start a wireless service.

AID 10.5 has Center, Left, Right and Off card choices and a fixed 1010×376 output. AID 12.3 has On and Off and an 800×480 output. Canvas zoom changes content size, never the encoder dimensions. Steering zoom is routed to the canvas in CP mirroring and to the phone map otherwise.

MU1102 and MU1367 support both cluster profiles. MU1440 Škoda supports AID 10.5 only. Exact firmware sizes are listed above; unknown profiles are rejected before factory files are modified.

The AID 10.5 native card core is byte-identical to the on-unit golden baseline; Waze retains a known card-layout issue. The final AID 12.3 safe area `(220,65,360,348)` is an offline-tested experimental parameter, not a vehicle-validated golden release. The complete 2.0 package has not been verified on every vehicle.

AID 12.3 retains a 40 fps stream declaration and an IDR request every five frames. The AID 10.5 card baseline declares a 60 fps ceiling. Only in AID 10.5 CP-mirror mode, the original 10 fps encoder request becomes 30 fps. These are not universal measured-FPS guarantees.

### Install and restore

Back up the unit, maintain stable power with the vehicle stationary, and disconnect CarPlay completely. Extract the full MQB ZIP to the SD-card root. Open MQB Toolbox → Customization → CarPlay Maps VC and select the actual AID 10.5 or AID 12.3 instrument. The installer preserves an existing valid navigation coexistence patch and automatically installs or repairs this project's missing/incomplete copy.

After `CPMAP_INSTALL_OK`, perform a complete head-unit reboot through the Toolbox procedure. Open App-Connect settings → Virtual Cockpit settings. Do not connect the phone after a failed install. Restore factory files restores the saved startup and JAR files, retaining separate user preferences.

No M.I.B. automatic flashing installer, password changes, device-bound ciphertext, paid watchdog, music metadata, album art or startup animation is included. Existing encrypted installations require their own migration tool.

For an unsupported firmware, contact the author about a free adaptation and share the system backup privately, not in a public Issue. Contact [omonob@126.com](mailto:omonob@126.com) or [Telegram](https://t.me/omonob). Optional paid extensions are separate from the public software license; confirm the supported firmware and feature scope before purchasing.

## Build and test

Source is under `src/card-core`, `src/vc-menu`, `src/vc-runtime`, `src/toolbox` and `src/steering-mu1440`. Full OEM firmware, private credentials, device keys and proprietary fonts are not redistributed. Supply your own exact original firmware JARs to compile the minimal UI hooks.

1. Run `tools/Build-V2Menu.ps1` with `-Mu1102Jar`, `-Mu1367Jar`, `-Mu1440Jar` and a fresh `-BuildRoot`. It compiles against each firmware API and runs host persistence, policy, locale and profile tests.
2. Run `tools/Build-V2Native.sh` inside a compatible locally built QNX 6.5 ARMv7 GNU toolchain container with this repository mounted at `/src`. It builds five native cores, two fixed-resolution renderers and the shared runtime, and runs the actual declaration/async-transport tests under sanitizers.
3. Run `tools/Stage-V2Profiles.ps1` with `-MenuBuild` and `-NativeBuild`, then `tools/Build-MQBRelease.ps1` with the unchanged published v1.2.2 full MQB ZIP and a fresh `-BuildRoot`.
4. On a Linux host run `python3 tests/test_v2_install.py` and `python3 tests/test_navigation_repair.py` for fixture-only install, repeat, repair, rollback and restore tests. These scripts do not connect to a vehicle.

[Full 2.0 release notes](releases/toolbox-plaintext-v2.0.0.md) include the exact feature and validation boundaries. The project remains licensed under [GPLv3](LICENSE).
