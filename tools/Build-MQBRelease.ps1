[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PreviousArchive,
    [Parameter(Mandatory=$true)][string]$BuildRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$repo = Split-Path -Parent $PSScriptRoot
$previous = [IO.Path]::GetFullPath($PreviousArchive)
$build = [IO.Path]::GetFullPath($BuildRoot)
$name = 'VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A.zip'
if (Test-Path -LiteralPath $build) { throw 'BuildRoot must be a new directory.' }
if ((Get-FileHash -LiteralPath $previous -Algorithm SHA256).Hash.ToLowerInvariant() -ne '49910839baaa1afe60fda4b3cb07d1a3a0b511a23e9066da5555f32d0cf604b2') {
    throw 'Expected the unchanged published v1.2.0 MQB archive.'
}
$assets = Join-Path $build 'release-assets'
New-Item -ItemType Directory -Path $assets | Out-Null
$output = Join-Path $assets $name
Copy-Item -LiteralPath $previous -Destination $output
$archive = [IO.Compression.ZipFile]::Open($output, [IO.Compression.ZipArchiveMode]::Update)
try {
    $entryName = 'Custom/carplay_maps_vc/payload/common/install-carplay-maps.sh'
    $entry = $archive.GetEntry($entryName)
    if (!$entry) { throw 'Expected MQB installer path is missing.' }
    $entry.Delete()
    [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, (Join-Path $repo 'src\toolbox\install-carplay-maps.sh'), $entryName, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
} finally { $archive.Dispose() }
$hash = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $assets 'FULL_TOOLBOX_SHA256.txt'), "$hash  $name`n", [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $repo 'releases\toolbox-plaintext-v1.2.1.md') -Destination (Join-Path $assets 'GITHUB_RELEASE_NOTES.md')
if (@(Get-ChildItem -LiteralPath $assets -File).Count -ne 3) { throw 'MQB release must contain exactly one ZIP, one checksum and one notes file.' }
Write-Host "MQB_ONLY_RELEASE_BUILD_OK $assets"
