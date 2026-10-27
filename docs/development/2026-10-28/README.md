# 虚拟座舱开发进度

[English](README_EN.md) · [2.0安装包](https://github.com/omonob/MHI2-Carplay-Maps/releases/tag/toolbox-plaintext-v2.0.0) · [测试数据](evidence/mirror-performance-summary.json)

截至2026年10月28日，本轮已公开原生菜单、可记忆的独立开关、AID 10.5与AID 12.3镜像输出、画布缩放和导航卡片代码。无线CarPlay目前公开的是测试开关及读取接口，不是可连接手机的无线接收程序。本次补充开发记录和图片，不改变已发布的2.0安装包。

这份公开记录不包含CarPlay导航信息映射、歌曲信息、封面模块或其测试画面，也不包含设备身份、访问凭据、密钥、完整车机日志及受许可SDK。

## 菜单开发

最终采用App-Connect设置下的独立“虚拟座舱设置”入口，保留原厂MirrorLink页面。早期直接改名MirrorLink的方式会在入场动画中露出原来的页面，因此没有作为最终方案。

![原生虚拟座舱设置入口](images/native-vc-entry.png)

这是一张菜单开发阶段的MU1102原生HMI离线截图，展示独立入口；不是完整2.0安装包的实车验收照片。图片未经改绘。

| 控制项 | 已接入的公开功能 | 边界 |
| --- | --- | --- |
| 虚拟座舱地图 | CarPlay Maps／原车地图，保存设置 | AA地图引擎未随包提供 |
| 虚拟座舱镜像 | CarPlay镜像开关，地图优先、冲突置灰 | AA与CarLife引擎未随包提供 |
| 无线CarPlay | 开关、持久化、只读接口，默认关闭 | 不启动或停止真实无线服务 |
| 导航卡片 | AID 10.5：中间／左／右／关；AID 12.3：开／关 | AID 12.3最终小卡参数仍为实验状态 |

选项使用原厂连续滚动列表，没有“上一页／下一页”。MU1102参考布局的选项条宽1139像素、模型高94像素、行距91像素；模型高度和灰色可见背景高度不是同一概念。这些是当前皮肤的参考尺寸，不是其他品牌或分辨率的通用常量。

![原厂油耗选项作为下拉样式参考](images/oem-dropdown-reference.png)

以上也是原生HMI离线截图，用于说明菜单背景、分隔线和间距的参考来源。

设置文件使用CRC、待提交文件和上一份有效副本；读取损坏主文件时可回退到有效副本，不能悄悄重置用户设置。31种语言的标签保留，包含简繁中文、英文、日文、韩文等；实际字形依赖固件字体。三个固件分别编译过菜单接口，主机测试覆盖保存、恢复、冲突处理和独立进程重读。

代码：[菜单入口与生命周期](../../../src/vc-menu/src/org/omonob/cpmap/ui/IndependentMenuViewBridge.java)、[联动设置](../../../src/vc-menu/src/org/omonob/cpmap/ui/ProjectionSettings.java)、[卡片设置](../../../src/vc-menu/src/org/omonob/cpmap/ui/CardPositionSettings.java)。

## 镜像与画布缩放

AID 10.5固定输出1010×376，AID 12.3固定输出800×480。内容画布可以缩放、留黑边或裁剪，但送往仪表的编码画面尺寸不跟着改变。镜像模式下，大众按键或斯柯达滑轮控制画布；回到地图模式后仍控制手机地图。

下面是原生渲染器的离线棋盘格输出，两张图均是100%画布设置。输入是1264×640、4 fps的合成测试源，没有连接手机，只用于验证尺寸、画布和退出恢复，不能用作手机性能测试。

![AID 10.5固定1010乘376输出的离线测试](images/mirror-aid10.5-fixture.png)

![AID 12.3固定800乘480输出的离线测试](images/mirror-aid12.3-fixture.png)

AID 10.5的卡片核心保持已上机黄金基线，Waze仍有已知卡片布局问题。AID 12.3实验小卡安全区为`(220,65,360,348)`，底边413；最终参数尚待实车确认，共享安全区可能同时影响地图布局。

代码：[镜像渲染器](../../../src/vc-runtime/mirror/carplay_mirror_receiver.c)、[固定输出约束](../../../src/vc-runtime/mirror/cpvc_output_contract.h)、[按键路由](../../../src/vc-menu/src/org/omonob/cpmap/ui/ClusterCommandRouter.java)、[卡片声明](../../../src/card-core/source/navigation_card_scene.h)。

## 帧率与延迟测试

2026年10月27日的MU1102／AID 10.5实机记录显示，渲染计数和原生编码器馈送节拍并不相同。先移除周期性的尺寸查询后，仍能观察到编码器10帧的限制；随后才增加只在AID 10.5 CarPlay镜像模式下生效的30帧请求转换。

| 测量位置 | 修订前记录 | 能说明什么 |
| --- | --- | --- |
| 解码画面采集 | 8个连续窗口，25.70～28.69 fps | 车机内部采集速度，不是仪表显示帧率 |
| GLES渲染计数 | 最后25个约1秒窗口，每窗口28～30帧，均值28.84帧 | 不是精确按秒归一的物理仪表FPS |
| 采集到渲染的软件阶段 | 25个窗口的平均值48～57 ms，窗口均值的算术平均52.8 ms | 不包含手机输入、后续编码、MOST及仪表显示 |
| 原生编码器馈送 | 80次馈送，探针运行8002 ms，实测10.000 fps | 编码器输入节拍，不是物理面板计数 |
| 30帧请求修订后 | 上机、重启回读和用户热开关反馈通过 | 尚缺同口径的编码器数字复测与端到端延迟测量 |

![软件阶段时间统计](evidence/software-stage-timing.svg)

![修订前编码器馈送周期](evidence/encoder-feed-period.svg)

这里的`avg_ms`从解码后画面进入采集队列的时间戳计算到EGL交换返回。52.8 ms不是“手机触控到仪表亮屏”的总延迟，也不是每一帧的延迟或P95。30 fps是修订后的请求目标，不写成“已实测稳定30帧”。地图的40／60 fps协商声明也不能与这条镜像编码链混为一谈。

公开数据仅保留数值白名单，去掉进程号、绝对运行时间、网络地址和完整日志：[采集窗口CSV](evidence/decoded-capture-intervals.csv)、[渲染窗口CSV](evidence/renderer-windows.csv)、[编码器馈送CSV](evidence/encoder-feed-before-r4.csv)。原始日志哈希留在[统计JSON](evidence/mirror-performance-summary.json)中。可使用[导出脚本](../../../tools/Summarize-MirrorEvidence.ps1)处理同格式记录。

## 无线CarPlay进度

### 已公开的接口

菜单开关默认关闭，保存到独立的`wireless-carplay.state`，保留上一份有效副本。Java写入和原生C读取使用相同格式；接口损坏且没有有效备份时按关闭处理。48项主机测试及独立JVM重读已通过。开关变化没有无线服务副作用。

代码：[Java保存接口](../../../src/vc-menu/src/org/omonob/cpmap/ui/WirelessCarPlaySettings.java)、[C读取接口](../../../src/vc-runtime/runtime/cpvc_wireless_settings.h)、[最小读取示例](../../../examples/wireless-state-reader.c)。

### 无线链路研究

独立的MU1102无线研究在2026年8月25日记录了以下结果；2026年10月11日又有一次仅RAM的2.4 GHz／40 MHz驱动状态回读。这些是无线链路证据，不是完整无线CarPlay会话验收。

| 项目 | 已记录结果 | 范围 |
| --- | --- | --- |
| PHY协商 | 2.4 GHz、40 MHz时150 Mbps | 协商速率，不是净吞吐 |
| TCP内存吞吐 | 终端到车机两次180秒测试：57.1、54.7 Mbps | 指定信道、环境和测试方法的结果 |
| 5 GHz实验 | 该测试机未得到可用、可连接的BSS | 不外推为所有MHI2硬件的统一结论 |
| 原生无线CarPlay | 公开2.0包没有完整无线接收服务 | 不能因开关可保存或带宽足够而宣称已连接成功 |

后续仍需把发现、会话建立、手机关联、音视频接入和断开恢复连成一条可重复验证的链路。受许可的通信SDK、MFi认证材料、密钥及原厂整库不在本次公开资料中，也不会打包进2.0安装器。

## 下一步验证

- 同一设备、同一操作下复测30帧请求修订后的编码器馈送和最终画面。
- 用同步时钟或高速摄像测手机输入到仪表显示的端到端延迟。
- 补做AID 12.3最终卡片参数和完整公开包的实车确认。
- 无线服务接入后，再验证手机首次连接、重连及开关行为；此前保持“接口可用、服务未发布”的状态说明。

图片来自已留存的离线测试，均未经改绘；图表由脱敏数值生成。未附实车地图照片，避免公开个人位置。图片来源与哈希见[素材清单](evidence/image-provenance.json)。
