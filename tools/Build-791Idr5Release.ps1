[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PreviousAssets,
    [Parameter(Mandatory=$true)][string]$BuildRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repo=Split-Path -Parent $PSScriptRoot
$previous=[IO.Path]::GetFullPath($PreviousAssets)
$build=[IO.Path]::GetFullPath($BuildRoot)
if (Test-Path -LiteralPath $build) { throw 'BuildRoot must be a new directory' }
$assets=Join-Path $build 'release-assets'
New-Item -ItemType Directory -Path $assets | Out-Null
$expectedCore='f3a6d2575772307f8007c70057836df449af1509ba06438333baccb612d55826'
$names=@{
    'SKODA_MU1440_790_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip'='73e0a327dd0a1dd92cdb098c7e27926f94a75fc12491d21181cff839c464c896'
    'VW_MU1102_790_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip'='7c41afcb41fe38ad33714ef263fa7e270a39cbf8fd85cf4390b1f5b67b5d216f'
    'VW_MU1102_791.794_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip'='97cd0de76c0c63fe10b55a5807acec8e62ec37344b8b85a9d604434aa3b29811'
    'VW_MU1367_790_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0.zip'='fd67bb6d0568c3eee0e8145ce9bdcef5d190d8ac8d741baa0949155c6dba2523'
    'VW_MU1367_791.794_CarPlay_Maps_VC_FULL_MIB_Toolbox_3.6.0_MU1367_791.zip'='3da22a34dc9e52a49baa9f5fd740cbcd2a5a0d072b0ab2fe7dba88b6e7ac407f'
    'VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip'='92866c7006ec20d844ed5281f92b2e9c862109db063fea07666d35eedd2c6b72'
}
foreach ($name in ($names.Keys | Sort-Object)) {
    $inputZip=Join-Path $previous $name
    if ((Get-FileHash -LiteralPath $inputZip -Algorithm SHA256).Hash.ToLowerInvariant() -ne $names[$name]) {
        throw "Previous published archive hash mismatch: $name"
    }
    $outputZip=Join-Path $assets $name
    Copy-Item -LiteralPath $inputZip -Destination $outputZip
    $mus=@()
    if ($name -match 'MQB') { $mus=@('MU1102','MU1367') }
    elseif ($name -match '791\.794') {
        $mus=@($(if ($name -match 'MU1102') { 'MU1102' } else { 'MU1367' }))
    }
    if (!$mus.Count) { Write-Host "RETAINED_790 $name"; continue }
    $archive=[IO.Compression.ZipFile]::Open($outputZip,[IO.Compression.ZipArchiveMode]::Update)
    try {
        foreach ($mu in $mus) {
            $core=Join-Path $repo "$mu\791_AID12.3\payload\profiles\$mu\791\core.so"
            if ((Get-FileHash -LiteralPath $core -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedCore) {
                throw "Unexpected new 791 core: $mu"
            }
            $prefix=if ($name -match 'MQB') { 'Custom/carplay_maps_vc/payload' } else { 'CarPlayMapsVC/payload' }
            $entryName="$prefix/profiles/$mu/791/core.so"
            $entry=$archive.GetEntry($entryName)
            if (!$entry) { throw "Missing existing profile in archive: $entryName" }
            $entry.Delete()
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$core,$entryName,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    } finally { $archive.Dispose() }
    Write-Host "UPDATED_791 $name"
}
$hashes=@(Get-ChildItem -LiteralPath $assets -Filter '*.zip' | Sort-Object Name | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(),$_.Name
})
[IO.File]::WriteAllText((Join-Path $assets 'FULL_TOOLBOX_SHA256.txt'),($hashes -join "`n")+"`n",(New-Object Text.UTF8Encoding($false)))
Copy-Item -LiteralPath (Join-Path $repo 'releases\toolbox-plaintext-v1.2.0.md') -Destination (Join-Path $assets 'GITHUB_RELEASE_NOTES.md')
Write-Host "791_IDR5_FULL_TOOLBOX_BUILD_OK $assets"
