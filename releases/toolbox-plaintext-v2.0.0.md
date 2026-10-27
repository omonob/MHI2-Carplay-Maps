# CarPlay Maps VC 2.0

## 中文

免费开源2.0将虚拟座舱地图、CarPlay镜像和导航卡片接入原车App-Connect里的独立“虚拟座舱设置”页面，不占用或改名MirrorLink设置。仅提供MQB完整工具箱安装，不恢复已撤下的M.I.B.自动刷写方式。

### 四项设置

1. 虚拟座舱地图：CarPlay Maps与原车地图切换。
2. 虚拟座舱镜像：CarPlay镜像开启／关闭。地图优先，同一来源的地图和镜像不能同时开启。
3. 无线CarPlay：测试开关和状态读取接口，默认关闭。实际无线连接服务尚未发布，打开本选项不会让车机具备无线CarPlay。
4. 导航卡片：AID 10.5为中间、靠左、靠右、关闭；AID 12.3为开启、关闭。

共用菜单保留Android Auto和CarLife的名称，但本包没有这两种引擎，对应选项置灰。音乐信息、封面、独立导航文字映射和启动动画不在本次免费包范围内。

设置写入持久存储，保留校验和与上一份有效设置；正常重启后自动读取。原厂连续滚动选项列表、选项条宽度与间距保留。菜单包含31种语言，含简繁中文、英文、日文、韩文、德文、法文、俄文等；实际字体显示取决于对应固件的语言字体，不随包分发原厂字体。

### 固件与仪表

| 固件 | AID 12.3输出 | AID 10.5输出 |
| --- | --- | --- |
| MU1102 | 800×480 | 1010×376 |
| MU1367 | 800×480 | 1010×376 |
| MU1440 Škoda | 不支持 | 1010×376 |

镜像画布缩放只改变画布内的内容大小，不改变送往仪表的固定输出分辨率。镜像模式下，原缩放按键改为控制画布；地图模式仍控制手机地图。MU1440保留斯柯达滑轮方向。

AID 10.5卡片核心与已上机验证的黄金基线逐字节一致；Waze卡片仍有已知布局问题。AID 12.3使用实验参数`(220,65,360,348)`：相对上一扁卡版本顶部上移2像素，下边界保持413，不采用导致大卡片的高度368。这组最终参数仅完成离线测试，仍需实车确认；共享安全区可能同时影响地图布局。

AID 12.3地图流保留40 fps协商声明及每5帧请求一次IDR的策略。AID 10.5卡片基线为60 fps协商上限。AID 10.5镜像仅在镜像模式下把原厂10帧编码请求改为30帧；这些参数不是所有手机、应用和仪表的实测帧率保证。

### 安装和恢复

车辆停稳并持续供电，完全断开CarPlay。先保存本机系统备份。将完整ZIP解压到SD卡根目录，进入MQB Toolbox → Customization → CarPlay Maps VC，选择真实仪表AID 10.5或AID 12.3。工具箱精确识别固件，保留有效导航共存补丁，缺少时自动补装。

看到`CPMAP_INSTALL_OK`后，按工具箱流程完整重启车机，再进入App-Connect设置中的“虚拟座舱设置”。不要在安装失败时连接手机测试。恢复原厂入口会恢复安装前保存的启动文件和JAR；用户设置单独保留。

公共包不修改密码、不写入设备绑定密文，也不安装付费授权守护。已安装加密版本的车机请使用对应迁移工具，公共安装器不会直接拆除其文件。

需要帮助，请提供固件版本、仪表型号、日志和照片，勿公开密码、密钥或包含身份数据的完整备份。联系：[omonob@126.com](mailto:omonob@126.com) · [Telegram](https://t.me/omonob)。

## English

Free open-source 2.0 adds an independent Virtual Cockpit settings page under the factory App-Connect settings. The original MirrorLink page is retained. Distribution remains the full MQB Toolbox package only; the retired M.I.B. automatic flashing installer is not included.

### Settings

- Virtual Cockpit Maps: CarPlay Maps or original maps.
- Virtual Cockpit Mirroring: CarPlay screen mirroring on or off. Maps take priority; a source cannot use maps and mirroring simultaneously.
- Wireless CarPlay: a test-only saved switch and read interface, off by default. No wireless service has been released or bundled; enabling the switch does not provide wireless CarPlay.
- Navigation card: Center, Left, Right or Off for AID 10.5; On or Off for AID 12.3.

Android Auto and CarLife names remain in the shared menu vocabulary, but their engines are not included and their choices are disabled. Album art, music metadata, separate navigation-text mapping and startup animations are outside this public package.

CRC-checked preferences and the previous valid copy survive normal reboots. Native continuous scrolling, row widths and spacing are preserved. Labels cover 31 locales, including English, Chinese and Japanese; glyph rendering depends on the firmware's installed fonts. Factory fonts are not redistributed.

### Profiles and validation

MU1102 and MU1367 each have separate 800×480 (AID 12.3) and 1010×376 (AID 10.5) profiles. MU1440 Škoda supports AID 10.5 only. Mirroring always renders to the instrument's exact output dimensions; canvas zoom never resizes the encoder output. Steering zoom controls the canvas in CP mirroring and the phone map otherwise. Škoda roller direction is retained.

The AID 10.5 native card core is byte-identical to the on-unit golden baseline. Waze retains a known card-layout issue. The AID 12.3 experimental safe area is `(220,65,360,348)`: two pixels above the preceding flat-card candidate with bottom 413 unchanged. Height 368, which produced a large card, is not used. The final AID 12.3 tuple has offline validation only, and the shared safe area can also affect map framing.

AID 12.3 retains a 40 fps map-stream declaration and an IDR request every five frames. The AID 10.5 card baseline declares a 60 fps ceiling. Only in AID 10.5 CP-mirror mode, the original 10 fps encoding request is translated to 30 fps. These settings are not a universal measured-FPS guarantee.

All three firmware menu APIs and all five supported installer profiles have offline builds and tests. These checks do not substitute for validation of the complete 2.0 package on every vehicle.

### Installation and rollback

Keep the vehicle stationary on stable power and disconnect CarPlay completely. Back up this unit first. Extract the full ZIP to the SD-card root, open MQB Toolbox → Customization → CarPlay Maps VC, and select the actual AID 10.5 or AID 12.3 instrument. The installer identifies the exact supported firmware and preserves a valid navigation coexistence patch, installing it automatically when missing.

After `CPMAP_INSTALL_OK`, perform a complete head-unit reboot through the Toolbox procedure. Open App-Connect settings → Virtual Cockpit settings. Do not test with the phone connected after a failed installation. Restore factory files restores the saved startup and JAR files; preferences remain separately stored.

No password replacement, device-bound ciphertext or paid authorization watchdog is included. Encrypted installations require their own migration tool and are not dismantled by this installer.

For support, share the firmware, cluster model, relevant logs and photos, not passwords, keys or identity-bearing full backups. Contact: [omonob@126.com](mailto:omonob@126.com) · [Telegram](https://t.me/omonob).
