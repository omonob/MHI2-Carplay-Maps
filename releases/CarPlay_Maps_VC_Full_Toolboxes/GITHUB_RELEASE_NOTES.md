# CarPlay Maps VC - Full Toolbox Release

Free open-source CarPlay Maps VC project by **omonob (QCDWJ)**.

These packages contain complete, ready-to-use toolbox distributions. Extract one selected archive directly to the root of an SD card.

## Full MQB Toolbox

`CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip` contains the complete MQB Coding MIB2 Toolbox with a new **CarPlay Maps VC** screen under **Customization**.

- The toolbox detects MU1102, MU1367 or MU1440 from the exact `lsd.jxe` size.
- The user selects 791 AID 12.3 or 790 AID 10.5.
- MU1440 Skoda accepts the 790 package only.

## Full M.I.B. Toolbox 3.6.0

Five complete M.I.B. 3.6.0 packages are provided. Each one is locked to a single firmware and instrument combination:

- MU1102 / 791 AID 12.3
- MU1102 / 790 AID 10.5
- MU1367 / 791 AID 12.3
- MU1367 / 790 AID 10.5
- MU1440 / 790 AID 10.5 Skoda

The automatic SWDL flow runs the original M.I.B. backup and launcher installation first. CarPlay Maps VC is installed only after those steps succeed.

## CarPlay Maps scope

- CarPlay map video projection.
- Steering-wheel zoom in, zoom out and long-press map switching.
- Plaintext runtime under `/mnt/app/carplaymaps`.
- No music or cover-art data.
- No navigation text or lane-guidance data.
- No startup animation.
- No device-bound encryption, NAVDB payload or 250 ms watchdog.

Do not combine files from different M.I.B. profile archives. Live vehicle validation is required for each firmware and instrument combination.
