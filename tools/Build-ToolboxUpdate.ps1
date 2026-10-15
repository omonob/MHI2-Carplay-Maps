[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PreviousReleaseBundle,
    [Parameter(Mandatory=$true)][string]$BuildRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repo = Split-Path -Parent $PSScriptRoot
$build = [IO.Path]::GetFullPath($BuildRoot)
if (Test-Path -LiteralPath $build) { throw 'BuildRoot must be a new directory.' }
New-Item -ItemType Directory -Path $build | Out-Null
[IO.Compression.ZipFile]::ExtractToDirectory($PreviousReleaseBundle, (Join-Path $build 'previous'))
$old = Join-Path $build 'previous\FULL_TOOLBOX_RELEASES'
$out = Join-Path $build 'release-assets'
New-Item -ItemType Directory -Path $out | Out-Null
$profiles = @(
    @('MU1102','790','MU1102\790_AID10.5','VW_MU1102_790_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip','2DECFD0CF4AE9C6D2AF2265B8B78B41E3D8F43BB12E309F848B6079B5BF23D50'),
    @('MU1102','791','MU1102\791_AID12.3','VW_MU1102_791.794_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip','33A516028EABDB5C49C05E15CE1358FB8465B3A4EDA7A72D54619921D09F9EA2'),
    @('MU1367','790','MU1367\790_AID10.5','VW_MU1367_790_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip','44C1234B94096B365490846F6A54442F00E4F80AF8D4FDBAFA1B7ABF634D1110'),
    @('MU1367','791','MU1367\791_AID12.3','VW_MU1367_791.794_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_791.zip','61FB51E17D30CB38DB07E03BBA407B0370932FF3686F6A0DAA6E2960934300A5'),
    @('MU1440','790','MU1440_SKODA\790_AID10.5','SKODA_MU1440_790_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip','E25213A472C9D099DFB5B3411CD1481D1159FFB330AA241BB78D312882CA7C5A')
)
function Verify-Previous([string]$Path, [string]$Expected) {
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Expected) {
        throw "Previous published archive mismatch: $Path"
    }
}
function Copy-File([string]$Source, [string]$Target) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $Target) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Target
}
function Update-Payload([string]$Package, [string]$Payload, [bool]$CommonControl) {
    Copy-File "$repo\src\toolbox\install-carplay-maps.sh" "$Payload\common\install-carplay-maps.sh"
    Copy-File "$Package\payload\common\CarPlayNavigationPatch.jar" "$Payload\common\CarPlayNavigationPatch.jar"
    if ($CommonControl) {
        Copy-File "$Package\payload\common\CarPlayClusterControls.jar" "$Payload\common\CarPlayClusterControls.jar"
    }
}
foreach ($profile in $profiles) {
    $mu, $cluster, $relative, $name, $expected = $profile
    $suffix = if ($mu -eq 'MU1440') { 'MU1440_790_SKODA' } else { "${mu}_${cluster}" }
    $zip = Join-Path $old "CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_${suffix}.zip"
    Verify-Previous $zip $expected
    $root = Join-Path $build "MIB_${suffix}"
    [IO.Compression.ZipFile]::ExtractToDirectory($zip, $root)
    $pkg = Join-Path $repo $relative
    Update-Payload $pkg "$root\CarPlayMapsVC\payload" ($mu -eq 'MU1440')
    Copy-File "$repo\src\toolbox\install-carplay-maps.sh" "$root\CarPlayMapsVC\install.sh"
    [IO.Compression.ZipFile]::CreateFromDirectory($root, (Join-Path $out $name), [IO.Compression.CompressionLevel]::Optimal, $false)
    Write-Host "BUILT $name"
}
$mqbZip = Join-Path $old 'CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip'
Verify-Previous $mqbZip '183815CE0FDAD198ED8A621114F0505580D8577A7DEF17A9AF693C965262C478'
$mqb = Join-Path $build 'MQB_ALL'
[IO.Compression.ZipFile]::ExtractToDirectory($mqbZip, $mqb)
$mqbPayload = "$mqb\Custom\carplay_maps_vc\payload"
Update-Payload "$repo\MU1102\791_AID12.3" $mqbPayload $false
Copy-File "$repo\MU1440_SKODA\790_AID10.5\payload\common\CarPlayClusterControls.jar" "$mqbPayload\profiles\MU1440\790\CarPlayClusterControls.jar"
Copy-File "$repo\src\toolbox\restore-carplay-maps.sh" "$mqb\Toolbox\scripts\cpmap_restore_core.sh"
# MU1427 (791 only): firmware-specific core.so and navigation patch, MQB menu
# detection, and the MU1427 jar for the toolbox "Ignore navigation-active" entry.
$mu1427 = "$repo\MU1427\791_AID12.3\payload"
Copy-File "$mu1427\profiles\MU1427\791\core.so" "$mqbPayload\profiles\MU1427\791\core.so"
Copy-File "$mu1427\common\CarPlayNavigationPatch.jar" "$mqbPayload\profiles\MU1427\791\CarPlayNavigationPatch.jar"
Copy-File "$mu1427\common\CarPlayNavigationPatch.jar" "$mqb\Modifications\Lsd\MU1427\NavActiveIgnore.jar"
foreach ($menuScript in 'cpmap_common.sh','cpmap_install_791.sh','cpmap_install_790.sh','install_nav_active_ignore.sh','uninstall_nav_active_ignore.sh') {
    Copy-File "$repo\src\toolbox\mqb\$menuScript" "$mqb\Toolbox\scripts\$menuScript"
}
[IO.Compression.ZipFile]::CreateFromDirectory($mqb, (Join-Path $out 'VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip'), [IO.Compression.CompressionLevel]::Optimal, $false)
$hashes = @(Get-ChildItem -LiteralPath $out -Filter '*.zip' | Sort-Object Name | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $_.Name
})
[IO.File]::WriteAllText((Join-Path $out 'FULL_TOOLBOX_SHA256.txt'), ($hashes -join "`n") + "`n", (New-Object Text.UTF8Encoding($false)))
Copy-File "$repo\releases\toolbox-plaintext-v1.1.0.md" "$out\GITHUB_RELEASE_NOTES.md"
Write-Host "FULL_TOOLBOX_UPDATE_BUILD_OK $out"
