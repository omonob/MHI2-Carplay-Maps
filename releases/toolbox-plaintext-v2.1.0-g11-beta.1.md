# CarPlay Maps VC 2.1.0 G11 beta 1

## 中文

这是传统 **完整 MQB Toolbox 测试包**，不是独立安装器，也不是已完成实车验收的稳定版。原有 2.0.1 稳定版继续保留。

新增支持：

| 固件 | 精确 lsd.jxe 字节数 | 中控原生 UI | 仪表输出 |
| --- | ---: | --- | --- |
| Škoda MHI2_ER_SKG11_K3343 / MU1433 | 52,751,492 | 自身固件 800×480 | AID 10.5，1010×376 |
| SEAT MHI2_ER_SEG11_P4709 / MU1447 | 55,712,574 | 自身固件 800×480 | AID 10.5，1010×376 |

只针对上述精确固件制作，不凭 G11、屏幕尺寸、包名后缀或左右舵判断兼容性。MU0033 不包含在本次支持中；未知固件和新 G11 的 AID 12.3 组合会在写入前拒绝。

免费功能为 CarPlay 仪表地图／屏幕镜像，以及导航小卡片（中间、靠左、靠右、关闭）。原厂 App-Connect 设置中增加独立“虚拟座舱设置”，沿用各品牌自己的原生组件、尺寸和字体；保留 31 种语言和可记忆设置。不是将大众大屏 UI 移植到 G11 小屏。

MU1433 的投屏会话结构采用逐条核对原固件后确认的布局，并使用专用导航共存适配；MU1447 保留其自身布局，不能因为同属 G11 就直接复制 0627 参数。现有 MU1102、MU1367 和 MU1440 的五组核心及菜单载荷不变。

不包含独立导航文字、音乐信息、专辑封面、付费加密程序或启动动画。无线 CarPlay 只有默认关闭的保存／读取测试接口，实际无线服务仍未发布。Android Auto／CarLife 引擎不包含，相关选项保持不可用。

### 验证范围与已知限制

- 发布源代码已针对两款原固件 API 重新编译，并通过主机端设置、联动策略、31 语言、独立进程读回及按键路由测试。
- 两款原生核心已独立重建两次并比对，固定输出 1010×376；声明、异步传输、错误 ACK、会话切换和布局偏移测试通过。
- 本地历史候选在两款自身固件的 QNX 虚拟机上完成英文原生菜单点击、保存和重启后读取。最终发布 JAR 另做主机回归；不将历史虚拟机结果冒充本包的实车验收。
- 尚未完成真实 G11 车机的 SD 安装向导、手机视频、MOST／仪表解码、热切换、长时间重连及实测帧率／延迟验收。声明上限不是实测帧率保证。Waze 卡片仍有已知布局问题。

### 安装

先完整备份同一台车机，车辆停稳、稳定供电，完全断开 CarPlay。校验 ZIP 的 SHA-256，解压到 SD 卡根目录。进入 **MQB Toolbox → Customization → CarPlay Maps VC → Install AID 10.5 (AUTO MU)**。请勿在新 G11 上选择 AID 12.3。

安装器保留有效的导航共存补丁，缺失时补齐本项目适配；不删除不属于本项目的补丁。看到 `CPMAP_INSTALL_OK` 后通过工具箱流程完整重启。失败后不要连接手机测试。恢复入口使用本机首次备份，独立保存的用户设置保留。已有加密版本请使用其专用迁移工具。

本次公开下载仅为完整 MQB 包；独立安装入口仍是本地实验，不在本次发布中。

## English

This is a **full traditional MQB Toolbox prerelease**, not a standalone installer or a vehicle-accepted stable release. Version 2.0.1 remains available as the stable release.

Adds exact Škoda **MHI2_ER_SKG11_K3343 / MU1433** (lsd.jxe 52,751,492 bytes) and SEAT **MHI2_ER_SEG11_P4709 / MU1447** (55,712,574 bytes). Both use their own 800×480 native head-unit UI and **AID 10.5, 1010×376** instrument output. MU0033 is excluded. G11, display size, archive suffix or steering side alone is not a compatibility check; unknown firmware and G11/AID 12.3 combinations are rejected before writes.

Free features: CarPlay cluster maps/screen mirroring and Center/Left/Right/Off navigation cards. The independent Virtual Cockpit settings page uses each brand's original components and geometry, with 31 locales and persistent preferences. MU1433 receives an exact-ABI navigation coexistence adapter and audited session layout; MU1447 retains its different layout. The five existing MU1102/MU1367/MU1440 core and menu payloads are unchanged.

No separate navigation-text, music, artwork, paid encryption/watchdog or startup-animation feature is included. Wireless CarPlay is an off-by-default saved test interface only; the actual wireless service is not released. AA/CarLife engines are not included.

Both exact API builds, host persistence/policy/locale/wheel tests, two independent native builds, fixed-resolution declaration and asynchronous transport tests pass. Earlier candidates also passed native English UI interaction and restart/persistence in each firmware's QNX VM. The final rebuilt JARs have separate host regression coverage; neither VM result nor host tests are physical-vehicle acceptance.

Physical G11 SD installation, phone video, MOST/cluster decoding, hot switching, long runs/reconnects and measured FPS/latency remain unverified. Negotiated ceilings are not measured-FPS promises. Waze retains a known card-layout issue.

Back up the same unit, maintain stable power with the vehicle stationary and disconnect CarPlay. Verify the ZIP SHA-256, extract to the SD root and select **MQB Toolbox → Customization → CarPlay Maps VC → Install AID 10.5 (AUTO MU)**. After `CPMAP_INSTALL_OK`, perform a full reboot through the Toolbox. Do not connect a phone after failure. Restore uses the unit's first saved factory files and retains separate preferences. Encrypted installations require their own migration tool.

Only the full MQB distribution is published. The standalone installer remains a local experiment.

Contact: [omonob@126.com](mailto:omonob@126.com) · [Telegram](https://t.me/omonob).
