param([Parameter(Mandatory=$true)][string]$MenuBuild,[Parameter(Mandatory=$true)][string]$NativeBuild)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
$repo=Split-Path -Parent $PSScriptRoot
$profiles=@(@('MU1102','791'),@('MU1102','790'),@('MU1367','791'),@('MU1367','790'),@('MU1440','790'))
# Deliberately avoid restaging the shared bridge, supervisor, mirror renderer,
# steering code and AID 12.3 native cores during this narrow fix.
foreach($profile in $profiles){
 $mu=$profile[0];$cluster=$profile[1];$brand=if($mu -eq 'MU1440'){'MU1440_SKODA'}else{$mu}
 $folder=if($cluster -eq '790'){'790_AID10.5'}else{'791_AID12.3'}
 $package=Join-Path $repo ($brand+'/'+$folder)
 $specific=Join-Path $package ('payload/profiles/'+$mu+'/'+$cluster)
 foreach($name in @('VCMirrorRate.jar','CarPlayClusterControls.jar')){
  if((Get-FileHash -LiteralPath (Join-Path $specific $name)).Hash -ne (Get-FileHash -LiteralPath (Join-Path $MenuBuild ($mu+'/'+$name))).Hash){throw ('Unrelated JAR changed: '+$mu+'/'+$name)}
 }
 Copy-Item -LiteralPath (Join-Path $MenuBuild ($mu+'/VCMenu.jar')) -Destination (Join-Path $specific 'VCMenu.jar')
 if($cluster -eq '790'){
  $core=Join-Path $NativeBuild ($mu+'.so')
  if((Get-FileHash -LiteralPath $core).Hash -ne '97CCB829F996B74305099CA54E6F4A693A17381B391D170C1227F44E49FF0769'){throw 'Unexpected AID 10.5 core'}
  Copy-Item -LiteralPath $core -Destination (Join-Path $specific 'core.so')
 }elseif((Get-FileHash -LiteralPath (Join-Path $specific 'core.so')).Hash -ne 'B3D28142EAC16D220E639BD0D8056E595E4D4BBF90A9D63D2E3C402C62B870B0'){throw 'AID 12.3 native core changed'}
 $installer=Join-Path $repo 'src/toolbox/install-carplay-maps.sh'
 Copy-Item -LiteralPath $installer -Destination (Join-Path $package 'install.sh')
 Copy-Item -LiteralPath $installer -Destination (Join-Path $package 'payload/common/install-carplay-maps.sh')
}
Write-Output 'AID10_CENTER_THREE_CORES_FIVE_MENUS_STAGED_UNRELATED_RUNTIME_UNCHANGED'
