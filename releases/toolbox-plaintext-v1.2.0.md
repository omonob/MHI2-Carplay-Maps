# CarPlay Maps VC v1.2.0

作者 / Author: [omonob（汽车大玩家）](https://github.com/omonob)

## 中文更新说明

本次将已经在开发测试中使用的 **791 仪表 40 FPS 显示优化** 加入免费开源工具箱。保持 800×480 和导航居中，适用于原支持矩阵中的 MU1102、MU1367 的 791/794 AID12.3 包；不新增固件或仪表支持范围。

### 本次变化

- 优化 791 视频显示参数，保持原有分辨率、导航位置和控制方式。
- 791 核心原本已声明 40 fps；同步纠正主页过时的 30 fps 标注。
- 更新 MU1102/791、MU1367/791 完整 M.I.B. 包，以及通用 MQB 包中对应的两个 791 核心。
- 附带可复现的参数变更脚本、离线测试及 SHA-256 校验文件。

40 fps 是副屏协商声明，实际帧率取决于手机、地图内容和仪表消费能力。本次不承诺零延迟或所有场景彻底消除花屏。

### 保持不变

790 的三个 M.I.B. 压缩包按 v1.1.0 原字节保留。790 核心、位置参数、桥接器、supervisor、认证组件、导航前置补丁、方向盘控制、固件白名单和安装入口均不修改。MU1440 斯柯达仍仅支持 790，滑轮放大缩小保留。

不加入音乐、导航文字或车道信息映射、启动动画、NAVDB 操作、设备绑定加密或 250 ms 守护。本仓库原有免费开源范围与独立付费支持说明不变。

### 下载和安装

- `VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip`：通用完整 MQB Toolbox，自动识别 MU，菜单选择 791 或 790。
- `VW_MU1102_791.794_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip`：MU1102＋791/794。
- `VW_MU1367_791.794_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_791.zip`：MU1367＋791/794。
- 另外三个 790 包保持原文件，继续提供下载。

解压到 SD 卡根目录，不要套一层文件夹。原安装入口不变：MQB 安装成功后手动完整重启，M.I.B. 保持原 SWDL 自动流程。选择匹配固件和仪表的包，不要混合 profile；下载后用 `FULL_TOOLBOX_SHA256.txt` 校验。旧版本 Release 保留。

### 验证边界

本次优化来自此前的开发测试配置。封包验证包括核心哈希与变更范围校验、其余运行文件保持不变、各固件安装/重复安装/原厂恢复测试、ZIP 完整读取及校验。封包的离线检查不等于本次已在每个 MU、iPhone 和地图 App 上重新完成实车测试。

## English release notes

This release brings the previously exercised **791 40 FPS display optimization** into the free open-source toolbox distributions. The 800x480 canvas and navigation-centered layout remain unchanged. It applies to the existing MU1102 and MU1367 791/794 AID12.3 profiles; compatibility is not expanded.

### Changes

- Optimizes the 791 video display parameters while preserving resolution, navigation placement and controls.
- Corrects stale top-level 30 fps documentation: the existing 791 core already advertised 40 fps.
- Updates the two fixed 791 M.I.B. distributions and their two 791 cores in the universal MQB toolbox.
- Includes a reproducible parameter transformation, offline tests and SHA-256 checksums.

40 fps is a negotiated ceiling, not a measured display-consumption guarantee. This release does not promise zero latency or elimination of corruption in every condition.

### Unchanged

All three 790 M.I.B. archives are retained byte for byte from v1.1.0. The 790 cores/layout, video bridge, supervisor, authentication components, navigation prerequisite, steering controls, firmware gates and installation entry points are unchanged. MU1440 Skoda remains 790-only with the existing roller zoom support.

No music/navigation metadata, lane guidance, startup animation, NAVDB operations, device-bound encryption or 250 ms guard is added. The free open-source scope and separate optional paid-support information remain unchanged.

### Download and installation

Use the complete universal MQB toolbox or the complete M.I.B. archive matching your exact MU/cluster. Extract directly to the SD-card root. MQB still requires a full manual reboot after successful installation; M.I.B. retains its SWDL flow. Do not mix profiles. Verify downloads against `FULL_TOOLBOX_SHA256.txt`. Earlier releases remain available.

### Validation boundary

The optimization comes from earlier development tests. This packaging pass checks core hashes and the parameter-change scope, unchanged unrelated runtime files, firmware install/reinstall/factory-restore fixtures, full archive reads and checksums. Offline packaging validation does not claim a new live test on every MU, iPhone and map-app combination.
