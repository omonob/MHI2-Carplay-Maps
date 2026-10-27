# MU1367 and AID 10.5 version 2.0.1

Exact firmware gate: 55840541 bytes. Output: 1010×376.

Navigation card: Center, Left, Right, Off. The original card rectangles are retained; Waze has a known layout issue. With the card Off, the iPhone 17 Pro Max pure-map view is `(202,16,606,344)` normally and `(42,16,606,344)` when CarPlay owns navigation. The new ownership-event integration still needs a phone-connected test; parameters are phone-specific.

This standalone file set is not the full Toolbox. Ordinary users should download the full MQB ZIP from Releases. On MMX, developers may run `/bin/ksh ./run-install.sh` from this directory. Back up the unit and disconnect CarPlay first. A complete reboot is required after `CPMAP_INSTALL_OK`; the script does not reboot automatically.

VC maps, CP mirroring, persistent native settings and navigation cards are included. Wireless CarPlay is a saved test interface only, off by default; no wireless service is bundled. Android Auto and CarLife engines, music metadata, album art and startup animations are not included.

这是独立项目文件，不是完整工具箱。请先备份本机并断开 CarPlay；安装成功后完整重启。菜单设置可记忆，无线 CarPlay 只是默认关闭的测试接口，尚未发布无线连接服务。

[Full bilingual instructions](../../README.md) · [2.0.1 release notes](../../releases/toolbox-plaintext-v2.0.1.md)
