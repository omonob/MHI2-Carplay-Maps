# NavActiveIgnore for MU1427

This is a firmware-matched rebuild of the MQB Toolbox's "Ignore navigation-active status from smartphone" patch (`Modifications/Lsd/NavActiveIgnore.jar`). It keeps the stock navigation map running in the Virtual Cockpit while CarPlay or Android Auto navigation is active on the head unit.

## Why the toolbox jar breaks CarPlay on MU1427

`NavActiveIgnore.jar` doesn't patch methods. It replaces whole HMI classes through `-Xbootclasspath/p:`, so it only works on the exact `lsd.jxe` it was built from. The generic jar has two problems.

1. **It was built for a newer firmware.**
   - Its `CarPlayModeHandling` has a 6-argument constructor that takes `ExboxGuidanceListenerImpl`.
   - MU1427's `HsmTarget` calls the 5-argument one, so CarPlay target startup throws `NoSuchMethodError` and no CarPlay session can come up.
   - `RequestHandler` and `ExboxGuidanceManagerImpl` likewise reference Exbox-guidance classes that MU1427 doesn't have.
2. **It was recompiled from broken decompiler output.**
   - Every `ldc` int constant is byte-swapped. For example, timer `100012` became `-1400504064`, and property `3010002` became `963062784`.
   - Long constants became `0L`, so timeouts of 5000 ms and 300 ms became 0, and ETA `*1000L` became `*0L`.
   - It also prints `new Exception().printStackTrace()` on hot paths.

## What this build does

It patches MU1427's **own** classes (`/ifs/lsd.jxe`, size 52,751,492 bytes) with ASM. Each change asserts the exact instruction pattern it expects, and everything else stays as the firmware compiled it.

| Class | Change |
|---|---|
| `DSICarplayProxy.requestModeChange` | Strips "native navigation takes ownership" app-states (`appStateID=2, state=true, speechMode=0`) from requests to the iPhone. The call is dropped only if nothing else is left to send. |
| `CarPlayModeHandling.handleModeChanges` | When CarPlay nav becomes active, it no longer raises property 3010002, no longer calls `setActiveNavigation(2)` and no longer calls `stopGuidance(6)`. When HMI nav becomes active, it no longer reclaims navigation ("User initiated"). |
| `ASLNavigationServices.stopGuidance` | Ignores reason 6 (smartphone took navigation). |
| AA `NavigationListener` | `updateGuidanceActive` and `stopGuidance` are no-ops; `isGuidanceActive` returns false. |
| AA `RequestHandler.performNavFocusRequestNotification` | Both focus types are answered as projected. It never stops native guidance and never raises property 3110002. |

The helper class `de.vw.mib.navactiveignore.NavActiveIgnore` is compiled Java-1.2-clean and downgraded to class version 46.

The debug-only `DSIAndroidAuto*Dispatcher` and `ExboxGuidanceManagerImpl` overrides from the original jar are left out on purpose.

## Cluster map stream: left stock (rev 3, 2026-10-12)
The original jar also promoted the cluster's stream data-rate request from 1 (map hidden) to 2 (map visible). Both attempts to port that broke the cluster map, which stayed stuck on "loading map" even with no phone connected:

- **Rev 1** promoted the rate inside the `ChangeDataRateSequence` constructor. `ChangeDataRate.sequenceDone()` then saw sequence rate 2 ≠ requested rate 1 and restarted the sequence forever.
- **Rev 2** promoted the request where it enters `ChangeDataRate`. That removed the 1→2 transition the stock logic depends on: a later "2" request found the stored rate already at 2 and was ignored, so the map-visible / frame-rate step never ran.

Rev 3 leaves `ChangeDataRate` and `ChangeDataRateSequence` stock. With no phone connected, the jar no longer changes the cluster path at all. The CarPlay/AA patches already stop the HMI from raising its "smartphone navigation active" properties (3010002 and 3110002), which is what made the cluster hide its map. Validated in the car on 2026-10-13: the OEM map loads with and without a phone, and CarPlay works. `patchChangeDataRate()` is still in `Patch.java` but is disabled. Don't re-enable it without making the sequence aware of the promotion.

## Files

- `build.sh`: rebuilds the jar from the factory SWDL package. The chain is LZOZ, then IFS, then `lsd.jxe`, then jxe2jar, then the ASM patch, then verification, then the jar.
- `patcher/Patch.java`: the patches. `patcher/Verify.java` runs ASM's `SimpleVerifier` against the MU1427 classpath.
- `extract/`: the `mifs-stage2` LZOZ decompressor and the QNX IFS extractor.
- `test/nai-install-test.sh`: an offline test of the toolbox install and uninstall scripts against the factory MU1427 `lsd.sh`, alone and combined with the CarPlay Maps VC installer in both orders. 24 checks.

## Recovery

If anything goes wrong, run **Uninstall Ignore navigation-active** from the toolbox menu. If the HMI is unusable, delete `/mnt/app/eso/hmi/lsd/jars/NavActiveIgnore.jar` over SSH; J9 skips missing boot classpath entries.
