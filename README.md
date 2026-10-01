<div align="center">

# CarPlay Maps VC for MHI2

**Bring the factory CarPlay map video to Volkswagen and Skoda virtual cockpits.**<br>
**让大众与斯柯达 MHI2 原厂 CarPlay 地图画面显示在液晶仪表上。**

[![Release](https://img.shields.io/github/v/release/omonob/MHI2-Carplay-Maps?display_name=tag&style=flat-square)](https://github.com/omonob/MHI2-Carplay-Maps/releases/latest)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue?style=flat-square)](LICENSE)
![Platform](https://img.shields.io/badge/platform-MHI2%20%2F%20QNX%206.5-2f6f9f?style=flat-square)
![Profiles](https://img.shields.io/badge/profiles-MU1102%20%7C%20MU1367%20%7C%20MU1440-green?style=flat-square)

[中文说明](#中文说明) · [English](#english) · [下载 Downloads](https://github.com/omonob/MHI2-Carplay-Maps/releases/latest)

**作者 / Author: [omonob (汽车大玩家)](https://github.com/omonob)**

</div>

---

## 中文说明

### 项目简介

CarPlay Maps VC 是一个面向大众与斯柯达 MHI2 车机的免费开源项目。它把 CarPlay 的地图视频画面送到原厂液晶仪表，并提供方向盘地图控制。

当前公开版本以经过验证的 V70 显示与控制逻辑为固定基线，提供两种完整工具箱：

- **MQB Toolbox 交互安装版**：工具箱自动识别 MU 固件，用户只需选择仪表型号。
- **M.I.B. Toolbox 自动安装版**：每个压缩包固定对应一个 MU 与仪表组合，插卡后通过 SWDL 自动执行。

> [!IMPORTANT]
> 本项目只实现 CarPlay 地图视频投屏与方向盘地图控制。它不是音乐信息、封面、导航文字、车道引导或启动动画补丁。

### 功能范围

#### 已包含

- CarPlay 地图视频投屏到原厂液晶仪表。
- 方向盘短按：地图放大或缩小。
- 方向盘长按：切换地图视图。
- 按固件精确识别 MU1102、MU1367 与 MU1440。
- 安装前保存可恢复的原厂 <code>dio_manager</code>、<code>lsd.sh</code> 与相关 JAR 状态。
- 支持状态检查、重复安装和恢复原厂文件。
- 运行文件以明文形式放在 <code>/mnt/app/carplaymaps</code>。

#### 明确不包含

- CarPlay 音乐信息或封面图像。
- 导航文字、道路名称、转向提示或车道引导。
- 仪表启动动画。

### 固定显示方案

| 仪表 | 显示基线 | 方向盘控制 |
| --- | --- | --- |
| 791 AID 12.3 | 30 fps，导航居中 | 短按缩放，长按切换地图 |
| 790 AID 10.5 | 30 fps，未导航居中、导航偏右 | 短按缩放，长按切换地图 |

### 支持矩阵

安装程序通过 <code>/ifs/lsd.jxe</code> 的精确文件大小识别固件。未知大小或不支持的仪表组合会在修改原厂文件前停止。

| 固件 | lsd.jxe 大小 | 791/794 AID 12.3 | 790 AID 10.5 |
| --- | ---: | :---: | :---: |
| MU1102 | 55,915,411 bytes | ✅ | ✅ |
| MU1367 | 55,840,541 bytes | ✅ | ✅ |
| MU1440 Škoda | 55,840,933 bytes | ❌ | ✅ |

> [!WARNING]
> MU1440 仅支持 Škoda 790 仪表包。不要混用不同固件或不同仪表的 M.I.B. 压缩包。

### 下载

所有文件均发布在 [GitHub Releases](https://github.com/omonob/MHI2-Carplay-Maps/releases/latest)。每个 ZIP 都包含**完整工具箱**，不是单独插件；解压后应直接得到 SD 卡根目录结构。

如果你已经有自己的工具箱或需要手动集成，请直接使用仓库中的独立项目文件。它们按照固件和仪表分开存放，**不包含完整工具箱**：

| 固件 / 仪表 | 独立文件目录 |
| --- | --- |
| MU1102 / 791/794 AID 12.3 | [`MU1102/791_AID12.3`](./MU1102/791_AID12.3/) |
| MU1102 / 790 AID 10.5 | [`MU1102/790_AID10.5`](./MU1102/790_AID10.5/) |
| MU1367 / 791/794 AID 12.3 | [`MU1367/791_AID12.3`](./MU1367/791_AID12.3/) |
| MU1367 / 790 AID 10.5 | [`MU1367/790_AID10.5`](./MU1367/790_AID10.5/) |
| MU1440 Škoda / 790 AID 10.5 | [`MU1440_SKODA/790_AID10.5`](./MU1440_SKODA/790_AID10.5/) |

每个目录只带该组合所需的安装脚本、公共运行文件、专用 `core.so`、状态检查和原厂恢复脚本。普通用户请优先下载 Releases 中的完整工具箱。

#### MQB Toolbox

| 文件 | 用途 |
| --- | --- |
| [CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip) | 一个包支持 MU1102、MU1367、MU1440；菜单中选择 791 或 790 |

#### M.I.B. Toolbox 3.6.0

| 文件 | 固件 / 仪表 |
| --- | --- |
| [MU1102 / 791/794](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1102_791.zip) | MU1102 + 791/794 AID 12.3 |
| [MU1102 / 790](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1102_790.zip) | MU1102 + 790 AID 10.5 |
| [MU1367 / 791/794](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_791.zip) | MU1367 + 791/794 AID 12.3 |
| [MU1367 / 790](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_790.zip) | MU1367 + 790 AID 10.5 |
| [MU1440 / 790 Škoda](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1440_790_SKODA.zip) | MU1440 Škoda + 790 AID 10.5 |

下载后请使用 [FULL_TOOLBOX_SHA256.txt](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/FULL_TOOLBOX_SHA256.txt) 校验文件。

PowerShell 校验示例：

    Get-FileHash .\CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip -Algorithm SHA256

### 可选付费支持

本仓库公开的 CarPlay Maps VC 地图投屏与方向盘控制部分**永久免费**，作者不会把开源包作为收费产品销售。下载、使用以及 GPLv3 赋予的权利均不要求购买付费选项。

如果你需要额外功能或希望支持作者继续开发，可以选择独立的付费服务：**¥399 RMB（约 US$60）**。美元金额按[欧洲央行 2026-09-30 参考汇率](https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html)估算，实际支付金额会随汇率变化。

<p align="center">
  <a href="https://www.goofish.com/item?id=1076254792602"><img src="./assets/optional-paid-support-goofish.png" alt="CarPlay Maps VC optional paid support" width="620"></a>
</p>

<p align="center"><strong><a href="https://www.goofish.com/item?id=1076254792602">查看闲鱼付费选项 / Open the paid option on Goofish</a></strong></p>

付费选项包含：

1. 大众、斯柯达全部车型专属启动 Logo 定制。
2. CarPlay 导航信息映射到仪表。
3. CarPlay 歌曲信息映射到仪表。
4. 791/794 仪表流畅度优化。
5. 790 仪表导航居中优化。
6. 原车无线 CarPlay 的后续研究与可能支持；该功能仍在评估中，不代表当前已经可用，也不承诺固定交付时间。

> [!NOTE]
> 付费选项是与本仓库开源内容分开的扩展功能和技术服务，并不是开源软件的授权费。购买前请通过商品页面确认自己的固件、仪表以及具体支持范围。

### 两种安装方法

| 项目 | MQB Toolbox | M.I.B. Toolbox |
| --- | --- | --- |
| 操作方式 | 在工具箱菜单中手动选择 | 插卡后通过 SWDL 自动执行 |
| 固件识别 | 自动识别 MU | 每个 ZIP 已锁定一个 MU |
| 仪表选择 | 用户选择 791 或 790 | 下载时选择固定仪表包 |
| 重启方式 | 安装器不自动重启，完成后手动完整重启 | M.I.B./SWDL 负责正常更新与重启流程 |
| 适合场景 | 已经使用 MQB Toolbox，希望可视化选择 | 希望使用独立、固定配置的自动安装包 |

#### 方法 A：MQB Toolbox 交互安装

1. 下载 <code>CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip</code>。
2. 将压缩包内容直接解压到 SD 卡根目录，不要额外套一层同名文件夹。
3. 将 SD 卡插入车机，进入 MQB Toolbox。
4. 打开 <code>Customization → CarPlay Maps VC</code>。
5. 根据仪表选择：
   - <code>Install 791 AID 12.3 (AUTO MU)</code>
   - <code>Install 790 AID 10.5 (AUTO MU)</code>
6. 工具箱会先识别 MU；固件或仪表组合不匹配时会停止。
7. 出现 <code>CPMAP_INSTALL_OK</code> 后，退出工具箱并对车机执行一次完整重启。
8. 车机启动后连接 iPhone，检查 CarPlay 地图与方向盘缩放/切换功能。

MQB 菜单还提供：

- <code>Show CarPlay Maps VC status</code>：查看当前安装状态。
- <code>Restore factory files</code>：恢复安装时保存的原厂文件；恢复后同样需要完整重启。

#### 方法 B：M.I.B. Toolbox 自动安装

1. 在五个 M.I.B. ZIP 中选择**唯一匹配**当前 MU 与仪表的版本。
2. 将所选压缩包内容直接解压到一张独立 SD 卡的根目录。
3. 不要把两个不同 profile 的文件混在同一张卡中。
4. 插入 SD 卡并启动/重启车机，M.I.B. 的 SWDL 流程会自动开始。
5. 自动流程先执行原 M.I.B. 备份与 Launcher 安装，成功后才安装 CarPlay Maps VC。
6. 更新期间保持稳定供电，不要拔卡或中断车机。
7. M.I.B./SWDL 完成其正常重启后，移除 SD 卡并连接 iPhone 测试。

### 技术实现

<details>
<summary><strong>展开查看运行架构</strong></summary>

1. 安装器读取 <code>/ifs/lsd.jxe</code> 大小，并将其映射到唯一 MU profile。
2. 所有文件先在 RAM/临时目录准备并检查，再提交到最终路径。
3. 原厂 <code>dio_manager</code>、<code>lsd.sh</code> 和原有 JAR 状态被保存为回滚基线。
4. <code>dio_manager</code> 包装器继续启动原厂程序，同时加载当前 MU/仪表对应的 <code>core.so</code>。
5. <code>altscreen-most-bridge</code> 与 <code>carplay-cluster-supervisor</code> 负责 CarPlay AltScreen 到仪表显示链路的运行管理。
6. <code>CarPlayClusterControls.jar</code> 提供方向盘缩放和长按切图控制。
7. 重复安装不会重复插入 Java boot classpath；失败提交会尝试恢复原厂基线。

</details>

### 开发工具与构建方法

- **QNX Neutrino 6.5 / ARMv7 运行目标**：保持与 MHI2 原生运行环境兼容。
- **KornShell / POSIX Shell**：车机端安装、状态检测、恢复与自动化流程。
- **Java/JAR**：方向盘地图控制与 boot classpath 集成。
- **MQB Toolbox GEM/ESD**：英文菜单、自动 MU 识别、安装/状态/恢复入口。
- **M.I.B. 3.6.0 SWDL/Launcher**：五个 profile 锁定的自动安装版本。
- **PowerShell**：完整工具箱组装、发布包、SHA-256 清单和结构化 QA。
- **Git 与 GitHub Releases**：版本管理、发布说明、校验清单和附件分发。

### 构建与验证状态

2026-09-30 发布构建已完成：

- 五个固定 M.I.B. profile 的首次安装路径测试。
- 重复安装不会重复添加 boot classpath。
- 恢复原厂文件后删除项目运行目录并还原原始状态。
- 错误固件和不支持的仪表组合会在写入前停止。
- Shell 语法、M.I.B. 元数据校验值、ELF 头、JAR 完整性检查。
- 工具箱显示文本为英文 ASCII，脚本统一使用 LF 换行。
- 六个完整工具箱 ZIP 全量读取与 SHA-256 校验。

当前发布包仍要求每个固件/仪表组合分别进行实车验证。790 与 791 实拍演示视频将在压缩后另行加入 Release。

### 开源与许可

- 项目维护者与作者：**omonob (QCDWJ)**。
- 本仓库原创的集成脚本、构建逻辑和文档按 [GNU GPL v3](LICENSE) 发布。
- 修改或再发布 GPL 覆盖内容时，请保留作者信息、许可证，并按 GPLv3 提供相应源代码。
- MQB Toolbox、M.I.B. 3.6.0、Volkswagen、Škoda、Apple CarPlay 以及其他第三方组件、名称和商标归各自权利人所有。
- 打包第三方工具箱是为了提供可直接使用的完整 SD 卡结构；原项目文件及其许可证声明应保持不变。
- GPLv3 不会将第三方固件、工具箱或厂商组件重新许可为本项目所有。
- 本项目不发布设备专属密码、私钥、K1/K2、<code>shadow</code>、NAVDB 密文或其他车辆绑定数据。

### 安全提示与免责声明

> [!CAUTION]
> 车机文件写入存在启动失败风险。操作前必须确认固件与仪表型号、保持稳定供电并保留可用恢复方案。不要在车辆行驶时安装或调试。

本项目与 Volkswagen AG、Škoda Auto 或 Apple Inc. 无隶属或授权关系。项目按现状提供，不对错误选择 profile、断电、第三方工具箱变更或不受支持固件造成的损失承担责任。

---

## English

### Overview

CarPlay Maps VC is a free and open-source community project for Volkswagen and Skoda MHI2 systems. It sends the CarPlay map video to the factory virtual cockpit and adds steering-wheel map controls.

The public release uses the validated V70 display/control behavior as a fixed baseline and provides two complete installation routes:

- **MQB Toolbox interactive package**: detects the MU firmware automatically; the user selects the cluster type.
- **M.I.B. Toolbox automatic package**: each archive is locked to one MU/cluster profile and runs through the SWDL workflow.

### Included

- CarPlay map video on the factory virtual cockpit.
- Short steering-wheel press for map zoom in/out.
- Long steering-wheel press for map-view switching.
- Exact MU1102, MU1367 and MU1440 firmware gates.
- Factory-file rollback evidence captured during installation.
- Status, repeat-install and factory-restore paths.
- Plaintext runtime under <code>/mnt/app/carplaymaps</code>.

### Not included

- Music metadata or cover art.
- Navigation text, road names, maneuver or lane guidance.
- Startup animation.


### Display profiles

| Cluster | Fixed display behavior | Steering-wheel control |
| --- | --- | --- |
| 791/794 AID 12.3 | 30 fps, navigation centered | Short press zoom; long press map switch |
| 790 AID 10.5 | 30 fps, non-navigation centered, navigation right | Short press zoom; long press map switch |

### Compatibility

| Firmware | Exact lsd.jxe size | 791/794 AID 12.3 | 790 AID 10.5 |
| --- | ---: | :---: | :---: |
| MU1102 | 55,915,411 bytes | ✅ | ✅ |
| MU1367 | 55,840,541 bytes | ✅ | ✅ |
| MU1440 Skoda | 55,840,933 bytes | ❌ | ✅ |

Unknown file sizes and unsupported cluster combinations stop before factory files are changed.

### Downloads

Download complete, ready-to-extract toolbox distributions from [GitHub Releases](https://github.com/omonob/MHI2-Carplay-Maps/releases/latest). Extract each archive directly to the SD-card root.

For manual integration into an existing toolbox, use the standalone project files stored directly in this repository. They are separated by firmware and cluster and **do not include a complete toolbox**:

| Firmware / cluster | Standalone files |
| --- | --- |
| MU1102 / 791/794 AID 12.3 | [`MU1102/791_AID12.3`](./MU1102/791_AID12.3/) |
| MU1102 / 790 AID 10.5 | [`MU1102/790_AID10.5`](./MU1102/790_AID10.5/) |
| MU1367 / 791/794 AID 12.3 | [`MU1367/791_AID12.3`](./MU1367/791_AID12.3/) |
| MU1367 / 790 AID 10.5 | [`MU1367/790_AID10.5`](./MU1367/790_AID10.5/) |
| MU1440 Skoda / 790 AID 10.5 | [`MU1440_SKODA/790_AID10.5`](./MU1440_SKODA/790_AID10.5/) |

Each directory contains only the matching installer, shared runtime files, profile-specific `core.so`, status script, and factory-restore script. Most users should download the full toolbox archives from Releases.

- [Full MQB Toolbox V4.2A](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip)
- [Full M.I.B. 3.6.0 — MU1102 / 791/794](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1102_791.zip)
- [Full M.I.B. 3.6.0 — MU1102 / 790](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1102_790.zip)
- [Full M.I.B. 3.6.0 — MU1367 / 791/794](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_791.zip)
- [Full M.I.B. 3.6.0 — MU1367 / 790](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_790.zip)
- [Full M.I.B. 3.6.0 — MU1440 / 790 Skoda](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1440_790_SKODA.zip)
- [SHA-256 manifest](https://github.com/omonob/MHI2-Carplay-Maps/releases/download/toolbox-plaintext-v1.0.0/FULL_TOOLBOX_SHA256.txt)

### Optional paid support

The open-source CarPlay Maps VC map-video and steering-control portion in this repository remains **free of charge**. The author does not sell the open-source package as a paid product, and downloading, using, or exercising the rights granted by GPLv3 does not require a purchase.

Users who want additional functions or wish to support continued development may choose a separate paid service: **¥399 RMB (approximately US$60)**. The USD figure is an estimate based on the [ECB reference rates for 30 September 2026](https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html); the actual converted amount may vary.

<p align="center">
  <a href="https://www.goofish.com/item?id=1076254792602"><img src="./assets/optional-paid-support-goofish.png" alt="CarPlay Maps VC optional paid support" width="620"></a>
</p>

[**View the optional paid service on Goofish**](https://www.goofish.com/item?id=1076254792602)

The paid option includes:

1. Exclusive model-specific startup logo customization for all Volkswagen and Skoda vehicle models.
2. CarPlay navigation information mapped to the instrument cluster.
3. CarPlay music information mapped to the instrument cluster.
4. Smoothness optimization for 791/794 clusters.
5. Navigation-centering optimization for the 790 cluster.
6. Ongoing research and possible future support for factory wireless CarPlay. This feature is still under evaluation, is not currently guaranteed to be available, and has no fixed delivery date.

> [!NOTE]
> This is a separate feature and technical-support service, not a license fee for the open-source software. Confirm the exact firmware, cluster and supported scope on the product page before purchasing.

### Installation route A: MQB Toolbox

1. Download the full MQB Toolbox archive.
2. Extract its contents directly to the SD-card root.
3. Insert the card and open MQB Toolbox.
4. Go to <code>Customization → CarPlay Maps VC</code>.
5. Select the 791 or 790 installer.
6. The toolbox detects the MU and stops on an incompatible selection.
7. After <code>CPMAP_INSTALL_OK</code>, exit the toolbox and perform one full unit reboot.
8. Connect the iPhone and verify map video and steering-wheel controls.

The MQB menu also includes status and factory-restore actions. A full reboot is required after restoration.

### Installation route B: M.I.B. Toolbox

1. Download exactly one archive matching the MU and cluster.
2. Extract that archive directly to the root of a dedicated SD card.
3. Never merge files from two profile archives.
4. Insert the card and start/restart the unit.
5. SWDL performs the original M.I.B. backup and Launcher installation before CarPlay Maps VC.
6. Keep stable power and do not remove the card during the update.
7. Allow M.I.B./SWDL to complete its normal reboot, then remove the card and test CarPlay.

### Development stack

- QNX Neutrino 6.5 / ARMv7 runtime target.
- KornShell and POSIX shell for install, status, restore and automation.
- Java/JAR integration for steering-wheel map controls.
- MQB Toolbox GEM/ESD menu integration.
- M.I.B. 3.6.0 SWDL/Launcher integration.
- PowerShell for complete-toolbox assembly, archives, SHA-256 manifests and QA.
- Git and GitHub Releases for versioning and distribution.

### Build and verification status

The 2026-09-30 release build passed offline first-install, repeat-install, factory-restore and wrong-profile-stop tests for all five package profiles. Shell syntax, M.I.B. metadata checksums, ELF headers, JAR integrity, ASCII/LF requirements, complete ZIP reads and SHA-256 hashes were checked.

Each firmware/cluster combination still requires independent live-vehicle validation. Compressed 790 and 791 demonstration videos will be added in a later Release update.

### Open-source notice

- Maintainer and author: **omonob (QCDWJ)**.
- Original integration scripts, build logic and documentation in this repository are licensed under [GNU GPL v3](LICENSE).
- GPL-covered redistribution must preserve attribution and license terms and provide corresponding source as required by GPLv3.
- MQB Toolbox, M.I.B. 3.6.0, Volkswagen, Skoda, Apple CarPlay and other third-party components, names and trademarks remain the property of their respective owners.
- Bundling a complete toolbox is for a ready-to-use SD-card layout; upstream files and license notices must remain intact.
- GPLv3 does not relicense third-party firmware, toolbox or vendor components.
- No unit-specific passwords, private keys, K1/K2, <code>shadow</code>, NAVDB ciphertext or vehicle-bound data are published.

### Safety and disclaimer

Writing infotainment files carries a boot-failure risk. Confirm the exact firmware and cluster, maintain stable power and keep a tested recovery route. Do not install or debug while driving.

This independent community project is not affiliated with or endorsed by Volkswagen AG, Skoda Auto or Apple Inc. It is provided as-is, without warranty.

---

<div align="center">

Free and open-source CarPlay Maps VC project by **omonob (QCDWJ)**.

</div>
