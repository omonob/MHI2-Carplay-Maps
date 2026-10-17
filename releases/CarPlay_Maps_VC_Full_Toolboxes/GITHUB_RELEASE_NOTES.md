# CarPlay Maps VC v1.2.1 MQB installer fix

作者 / Author: [omonob（汽车大玩家）](https://github.com/omonob)

## 中文

本次修复有效导航补丁被误判为 `NAVIGATION_PAYLOAD_INVALID` 的安装问题，**只保留 MQB Toolbox 完整包**，撤下 M.I.B. 自动刷写包。

### 导航补丁安装

旧安装器直接在二进制 JAR 中搜索类名，依赖了电脑搜索工具的二进制处理能力。新安装器先核对随包补丁的长度和 POSIX 校验值，再将二进制中的可打印字符串转换为文本后识别类名，避免车机文本工具误判。

- 没有 navigation 补丁：自动补装并加入唯一的启动加载项，无需用户另外下载。
- 已有完整导航补丁，包括与信息映射合并的 JAR：保留原文件，不重复添加。
- 本项目独立导航 JAR 的副本损坏或不完整：重装时自动修复。
- SD 卡上的随包补丁确实损坏：在修改原厂文件前停止，并提示重新解压 SD 卡。不会把损坏补丁强行装入车机。
- 其他项目的未知、不完整 CarPlay 类覆盖仍会停止，避免破坏其信息映射功能。

### 安装和更新

只下载 `VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip`，核对 `FULL_TOOLBOX_SHA256.txt` 后，将内容直接解压到 SD 卡根目录。完整替换旧包文件，避免继续执行旧安装脚本。

在 `Customization → CarPlay Maps VC` 中选择 791 或 790。支持范围仍为 MU1102、MU1367 的 791/790，以及 MU1440 斯柯达的 790。安装成功显示 `CPMAP_INSTALL_OK` 后手动完整重启，再连接手机测试。安装器不自动重启。

不再提供 M.I.B. 自动刷写安装入口。旧版说明和 Git 标签仅作历史记录，不应继续用于安装。

### 保持不变与验证

791 的 40 FPS 显示优化、790 显示参数、滑轮/按键控制、视频桥接和运行组件均保持 v1.2.0 原字节。ZIP 内只修改一个安装脚本，不加入音乐或导航文字映射、启动动画、设备绑定加密或守护进程。作者署名、开源许可、免责声明和独立付费支持说明保留。

离线测试覆盖五个固件/仪表组合、首次安装、重装、导航补丁保留/补装/修复、恢复原厂、损坏载荷和失败回滚；同时测试最终 ZIP。文本限定搜索环境是针对 QNX 工具行为的兼容性模型，不代替各台车机的实车验证。

## English

This release fixes a valid navigation prerequisite JAR being rejected as `NAVIGATION_PAYLOAD_INVALID`. **The complete MQB Toolbox is the only supported distribution; M.I.B. automatic-flashing packages are withdrawn.**

The old installer searched binary JAR data directly with a text utility. The fixed installer checks the bundled patch's POSIX checksum and size, then converts printable archive strings into text before class inspection. It does not depend on GNU binary-grep behavior or require a separate patch download.

- Missing prerequisite: install the included patch and one startup reference automatically.
- Existing complete patch, including a merged metadata JAR: preserve the file and avoid duplicate startup entries.
- Damaged standalone navigation JAR owned by this project: repair during reinstall.
- Corrupt bundled SD-card patch: stop before factory-file changes and ask for a fresh extraction.
- Unknown incomplete CarPlay overrides from other projects: stop for review rather than overwrite unrelated mapping code.

Download only `VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip`, verify `FULL_TOOLBOX_SHA256.txt`, and replace the old SD files with the archive contents at the card root. Select 791 or 790 under `Customization → CarPlay Maps VC`. The compatibility matrix remains MU1102 and MU1367 with 791/790, plus MU1440 Skoda with 790 only. After `CPMAP_INSTALL_OK`, perform a full manual reboot and test CarPlay. The installer never reboots automatically.

All video/runtime binaries, the 791 40 FPS optimization, 790 display parameters and steering controls remain byte-identical to v1.2.0. Only one installer changes inside the ZIP. No music/navigation metadata mapping, startup animation, device-bound encryption or watchdog is added. Attribution, licensing, disclaimers and separate optional paid-support terms remain unchanged.

Host-side tests cover all five firmware/cluster combinations, first/repeat install, patch preservation/installation/repair, factory restore, corrupted payloads, activation rollback and the final archive. A text-only grep shim reproduces the old archive's failure; it models target utility behavior and is not a substitute for live testing of every unit. Historical tags and notes are retained for reference, not installation.
