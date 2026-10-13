# MU1440 Skoda roller control

This is the roller-control implementation used for the MU1440 790 profile in toolbox v1.1.0. Existing Volkswagen button behavior and native display payloads are unchanged. It passively observes `updateEncoder2(4, 40, +1/-1, 0, 1)` and sends `zoom-in` or `zoom-out` to the existing loopback control port 7032. Roller presses and other encoder IDs are ignored. Original DSI listeners still receive the events.

Build with JDK 8:

```powershell
.\build.ps1 -OutputJar C:\Build\CarPlayClusterControls.jar
```

The compiler targets Java 1.4, classfile major 48. Compilation stubs are excluded from the runtime JAR. `TestSkodaRollerBridge` checks valid upward/downward detents and rejection of unverified input. On the vehicle, a phone map app must support zoom commands.

本实现仅用于 MU1440 790 组合。滑轮上拨放大、下拨缩小，按压不映射；不吞掉原厂 DSI 事件。大众按键逻辑与视频载荷不变。编译目标为 Java 1.4，构建用桩类不会装入运行 JAR。车端效果还取决于手机地图 App 是否支持缩放。
