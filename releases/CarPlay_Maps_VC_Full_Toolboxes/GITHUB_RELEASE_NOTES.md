# CarPlay Maps VC — Complete Toolbox Release / 完整工具箱发布

Free and open-source community project by **omonob (QCDWJ)**.<br>
由 **omonob（QCDWJ）** 开发维护的免费开源社区项目。

## 中文

本 Release 提供可直接解压到 SD 卡根目录使用的**完整工具箱**，不是单独插件。

### 功能

- CarPlay 地图视频投屏到原厂液晶仪表。
- 方向盘短按放大/缩小，长按切换地图。
- 791 AID 12.3：40 fps，导航居中。
- 790 AID 10.5：40 fps，未导航居中、导航偏右。
- 明文运行目录：<code>/mnt/app/carplaymaps</code>。

### 不包含

- 音乐信息、封面、导航文字、车道引导或启动动画。
- 设备绑定加密、K1/K2、密码或 <code>shadow</code> 修改。
- NAVDB 文件操作或 250 ms 守护进程。

### 两种安装方式

1. **MQB Toolbox V4.2A**
   - 一个完整包支持 MU1102、MU1367、MU1440。
   - 在 <code>Customization → CarPlay Maps VC</code> 中选择 791 或 790。
   - 自动识别 MU；安装完成后由用户执行一次完整重启。

2. **M.I.B. Toolbox 3.6.0**
   - 五个独立完整包：MU1102/791、MU1102/790、MU1367/791、MU1367/790、MU1440/790 Škoda。
   - 只能选择唯一匹配的压缩包，不要混合不同 profile。
   - 插卡后由 SWDL 自动执行，M.I.B. 负责正常更新与重启流程。

安装前请核对固件和仪表，保持稳定供电，并使用 <code>FULL_TOOLBOX_SHA256.txt</code> 校验下载文件。

## English

This Release contains **complete toolbox distributions** that can be extracted directly to the SD-card root. They are not standalone add-on archives.

### Features

- CarPlay map video on the factory virtual cockpit.
- Short steering-wheel press for zoom; long press for map switching.
- 791 AID 12.3: 40 fps, navigation centered.
- 790 AID 10.5: 40 fps, non-navigation centered, navigation right.
- Plaintext runtime under <code>/mnt/app/carplaymaps</code>.

### Excluded

- Music metadata, cover art, navigation text, lane guidance and startup animation.
- Device-bound encryption, K1/K2, password or <code>shadow</code> changes.
- NAVDB operations and the 250 ms watchdog.

### Two installation routes

1. **MQB Toolbox V4.2A**
   - One complete package for MU1102, MU1367 and MU1440.
   - Select 791 or 790 under <code>Customization → CarPlay Maps VC</code>.
   - The MU is detected automatically. Perform one full unit reboot after installation.

2. **M.I.B. Toolbox 3.6.0**
   - Five profile-locked packages: MU1102/791, MU1102/790, MU1367/791, MU1367/790 and MU1440/790 Skoda.
   - Select exactly one matching archive and never merge profiles.
   - SWDL starts the automatic flow; M.I.B. owns its normal update and reboot sequence.

Confirm the firmware and cluster, maintain stable power, and verify downloads with <code>FULL_TOOLBOX_SHA256.txt</code>.

For the compatibility matrix, architecture, rollback behavior, development stack, GPLv3 notice and detailed instructions, see the [project README](https://github.com/omonob/MHI2-Carplay-Maps#readme).

This independent project is not affiliated with or endorsed by Volkswagen AG, Skoda Auto or Apple Inc. Third-party toolboxes and components retain their original licenses and ownership.
