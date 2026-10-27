[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$PreviousArchive,[Parameter(Mandatory=$true)][string]$BuildRoot,[ValidateSet('2.0.0','2.0.1')][string]$Version='2.0.1')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$repo=Split-Path -Parent $PSScriptRoot
$previous=[IO.Path]::GetFullPath($PreviousArchive)
$build=[IO.Path]::GetFullPath($BuildRoot)
if(Test-Path -LiteralPath $build){throw 'BuildRoot must be new'}
if((Get-FileHash -LiteralPath $previous).Hash -notin @('06955ADE05E461B26EC2FDB78631378B690FEFB64B1FFA672B802CAF55061AB5','BE9EA428732A70D615E99E02043D70436E1A8BE1DA3C5812833C5D3D076B8412')){throw 'Expected exact published full MQB base'}
$assets=Join-Path $build 'release-assets';New-Item -ItemType Directory -Path $assets|Out-Null
$name='VW_SKODA_ALL_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A_v'+$Version+'.zip'
$output=Join-Path $assets $name
Copy-Item -LiteralPath $previous -Destination $output
$zip=[IO.Compression.ZipFile]::Open($output,[IO.Compression.ZipArchiveMode]::Update)
function Replace-Entry([string]$entry,[string]$file){
 $old=$zip.GetEntry($entry);if($old){$old.Delete()}
 [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,$file,$entry,[IO.Compression.CompressionLevel]::Optimal)|Out-Null
}
try{
 $common=Join-Path $repo 'MU1102/791_AID12.3/payload/common'
 foreach($file in Get-ChildItem -LiteralPath $common -File){Replace-Entry ('Custom/carplay_maps_vc/payload/common/'+$file.Name) $file.FullName}
 foreach($profile in @(@('MU1102','791'),@('MU1102','790'),@('MU1367','791'),@('MU1367','790'),@('MU1440','790'))){
  $mu=$profile[0];$cluster=$profile[1];$brand=if($mu -eq 'MU1440'){'MU1440_SKODA'}else{$mu};$cd=if($cluster -eq '790'){'790_AID10.5'}else{'791_AID12.3'}
  $specific=Join-Path $repo ($brand+'/'+$cd+'/payload/profiles/'+$mu+'/'+$cluster)
  foreach($file in Get-ChildItem -LiteralPath $specific -File){Replace-Entry ('Custom/carplay_maps_vc/payload/profiles/'+$mu+'/'+$cluster+'/'+$file.Name) $file.FullName}
 }
 Replace-Entry 'Toolbox/scripts/cpmap_restore_core.sh' (Join-Path $repo 'src/toolbox/restore-carplay-maps.sh')
 Replace-Entry 'Toolbox/GEM/carplay-maps-vc.esd' (Join-Path $repo 'src/toolbox/carplay-maps-vc.esd')
 Replace-Entry 'CARPLAY_MAPS_VC_README.md' (Join-Path $repo ('releases/toolbox-plaintext-v'+$Version+'.md'))
}finally{$zip.Dispose()}
$hash=(Get-FileHash -LiteralPath $output).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $assets 'FULL_TOOLBOX_SHA256.txt'),"$hash  $name`n",[Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $repo ('releases/toolbox-plaintext-v'+$Version+'.md')) -Destination (Join-Path $assets 'GITHUB_RELEASE_NOTES.md')
Write-Output ('V2_FULL_MQB_RELEASE_BUILT '+$assets)
