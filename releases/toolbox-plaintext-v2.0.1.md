# CarPlay Maps VC 2.0.1 — AID 10.5 map centering

## 中文

修正 MU1102、MU1367、MU1440 的 AID 10.5 纯地图居中声明。iPhone 17 Pro Max 使用：普通状态 `(202,16,606,344)`；CarPlay 取得导航所有权后切到 `(42,16,606,344)`。输出始终为 **1010×376**，没有缩小或拉伸编码画面。

此修订针对“导航卡片关闭”时的地图，不能与菜单里的“卡片居中”混为一谈。原来的靠左、靠右、中间卡片安全区保持不变；卡片开启时仍沿用各自已定义的布局。Waze 的已知卡片问题未在此修复。

同时去除纯地图切换对未公开信息模块状态文件的依赖。菜单 JAR 中的最小钩子只观察系统 CarPlay 的导航所有权标志，并在 RAM 保存经过 CRC 校验、绑定当前接收进程的 0/1 状态；不输出导航文字、转向信息、歌曲或封面，也不修改无线服务。原有 400 ms 去抖、READY 与单命令串行发送保护保留。

这套参数来自此前单独验证过的 iPhone 17 Pro Max 配置，不应冒充 iPhone 16e 或所有手机的通用参数。三套固件分别编译，生产声明构造、异步传输及安装夹具测试通过；**新整合的导航所有权通路尚待手机上机验证**，不把离线结果写成实车通过。

AID 12.3 的两个原生核心保持不变。菜单、镜像、无线测试开关、31 种语言和持久设置继续保留；无线接收服务仍未发布。音乐和独立导航信息映射仍不包含在公开包中。

安装前备份本机、保持稳定供电并完全断开 CarPlay。使用本次完整 MQB Toolbox ZIP，核对随包 SHA-256；安装成功后完整重启。首次测试 iPhone 17 Pro Max 时，将“导航卡片”设为“关闭”，分别比较未导航、开始导航、退出导航。请勿把其他手机的画面偏移直接当成安装失败。

[完整使用与恢复说明](https://github.com/omonob/MHI2-Carplay-Maps#readme) · [开发记录和图片](https://github.com/omonob/MHI2-Carplay-Maps/blob/main/docs/development/2026-10-28/README.md)

## English

Corrects the **AID 10.5** pure-map declaration for MU1102, MU1367 and MU1440. The retained iPhone 17 Pro Max geometry is `(202,16,606,344)` normally and `(42,16,606,344)` when CarPlay owns navigation. The encoded output stays **1010×376**.

This applies when Navigation Card is **Off**. It is not the compact Center card option. Existing Left, Right and Center card rectangles are retained; their layouts are not overridden by this pure-map adjustment. The known Waze card-layout issue is unchanged.

A minimal hook observes the factory CarPlay navigation-ownership event. It writes only a CRC-checked, receiver-process-bound boolean in RAM, removing the public core's dependency on the unpublished information module's state file. No navigation text, maneuver metadata, music, artwork or wireless service is implemented by this hook. Existing 400 ms debounce, READY gating and serialized command handling remain.

These are phone-specific parameters, not universal iPhone 16e/all-phone settings. The three firmware APIs are built separately and offline declaration, asynchronous transport and installer fixtures are tested. **The newly integrated ownership-event path still needs a phone-connected bench/vehicle test.** The earlier parameter calibration is not acceptance of this newly integrated package.

Both AID 12.3 native cores are unchanged. Persistent settings and all 31 locales remain. Wireless CarPlay remains a test switch/read interface, not a released wireless receiver. Separate navigation-information and music/artwork mapping are excluded.

Back up the unit, maintain stable power and disconnect CarPlay before installing the complete MQB Toolbox ZIP. Check its SHA-256 and reboot completely after installation. For the first iPhone 17 Pro Max test, set Navigation Card to Off and compare idle, active navigation and navigation stopped.
