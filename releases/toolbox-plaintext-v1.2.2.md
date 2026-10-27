# CarPlay Maps VC v1.2.2 - navigation prerequisite installer repair

作者 / Author: [omonob（汽车大玩家）](https://github.com/omonob)

## 中文

修复安装过程中正常导航补丁被反复误判的问题。请完整替换旧 SD 卡文件，不要继续运行旧版安装器。

- 精确识别支持的 MU/仪表组合后自动处理 navigation 前置补丁，再继续投屏安装，不增加人工确认步骤。
- 已有完整补丁：保留并直接继续，不再先强制检查根本不需要使用的 SD 卡导航 JAR。
- 没有补丁、旧引用文件缺失或补丁不完整：自动补装随包的独立导航补丁，并合并启动加载行。其他项目的 JAR 文件不改写；需要补齐时，本项目的七个导航共存类优先加载。
- 本项目独立导航补丁损坏：自动修复。重复安装不重复插入加载项，恢复原厂仍保留原始状态。
- 不再依赖车机的 `cksum`、`tr`、`strings`、`unzip` 或二进制 `grep`。新增约 10 KB 的只读 ARM 校验器，自行检查 ZIP 目录、七个必要类及随包文件完整性。
- 真正不支持的固件、SD 文件缺失/损坏、无法写入或不支持的启动结构仍会明确停止，不会假装安装成功。

只提供 **MQB Toolbox 完整包**：`VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A_v1.2.2.zip`。核对 `FULL_TOOLBOX_SHA256.txt`，完整解压到 SD 卡根目录，在 `Customization → CarPlay Maps VC` 选择 AID 12.3 或 AID 10.5。出现 `CPMAP_INSTALL_OK` 后手动完整重启。安装器不会自动重启。

支持范围保持 MU1102/AID 12.3、MU1102/AID 10.5、MU1367/AID 12.3、MU1367/AID 10.5、MU1440 斯柯达/AID 10.5。AID 12.3 的 40 FPS 显示优化、AID 10.5 参数、方向盘控制及所有原有运行组件不变。不加入音乐/导航文字映射、启动动画、设备绑定数据或守护。署名、许可、免责声明及独立付费支持说明均保留。

验证覆盖缺少车机工具、缺失/完整/合并/部分/损坏导航补丁、初装/重装/恢复、失败回滚，以及最终 ZIP 中的五种组合。检查器通过主机端测试和未定义行为检查，ARM 产物核对 QNX 6.5 ABI；这些不是新的实车执行结果。

## English

Fixes repeated false navigation-prerequisite failures. Replace the complete old SD-card contents; do not keep running an older installer.

- After exact supported-firmware/profile identification, handle the navigation prerequisite automatically and continue installation without another confirmation.
- Keep a complete active patch. Do not demand an unused SD navigation JAR before checking the installed patch.
- Automatically supply the separate bundled patch if missing, partially installed, or referenced but absent. Other projects' JAR files are not rewritten; when completion is necessary, our seven navigation-coexistence classes load first.
- Repair a damaged project-owned navigation JAR, preserve repeat-install idempotency and factory restoration.
- No dependency on target `cksum`, `tr`, `strings`, `unzip`, or binary-grep behavior. A small read-only ARM helper validates ZIP directory records, required classes and bundled-payload integrity directly.
- Unsupported firmware, genuinely missing/damaged required files, write failures and unsupported startup layouts still stop with an explicit error. Failures are not hidden or reported as success.

Only the complete **MQB Toolbox** archive is provided: `VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A_v1.2.2.zip`. Verify `FULL_TOOLBOX_SHA256.txt`, extract to the SD root, and select AID 12.3 or AID 10.5 under `Customization → CarPlay Maps VC`. Perform a full manual reboot after `CPMAP_INSTALL_OK`; no automatic reboot is performed.

Compatibility remains MU1102/AID 12.3, MU1102/AID 10.5, MU1367/AID 12.3, MU1367/AID 10.5 and MU1440 Skoda/AID 10.5. The AID 12.3 40 FPS optimization, AID 10.5 parameters, steering controls and all existing runtime binaries remain unchanged. No metadata mapping, startup animation, device-bound data or watchdog is added. Attribution, licensing, disclaimers and separate paid-support terms remain intact.

Host regression tests cover missing target utilities, absent/complete/merged/partial/damaged patches, installation/reinstallation/restoration, activation rollback and all five profiles extracted from the final archive. The checker is host-tested with undefined-behavior checks and cross-built for the QNX 6.5 ARM ABI. This is not a claim of fresh live-vehicle execution.
