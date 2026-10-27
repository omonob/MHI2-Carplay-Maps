param([Parameter(Mandatory=$true)][string]$MenuBuild,[Parameter(Mandatory=$true)][string]$NativeBuild)
$ErrorActionPreference='Stop';$repo=Split-Path -Parent $PSScriptRoot
$profiles=@(@('MU1102','791'),@('MU1102','790'),@('MU1367','791'),@('MU1367','790'),@('MU1440','790'))
foreach($profile in $profiles){
 $mu=$profile[0];$cluster=$profile[1]
 $brand=if($mu -eq 'MU1440'){'MU1440_SKODA'}else{$mu}
 $clusterDir=if($cluster -eq '790'){'790_AID10.5'}else{'791_AID12.3'}
 $pkg=Join-Path $repo ($brand+'/'+$clusterDir);$common=Join-Path $pkg 'payload/common';$specific=Join-Path $pkg ('payload/profiles/'+$mu+'/'+$cluster)
 New-Item -ItemType Directory -Path $specific -Force|Out-Null
 foreach($script in @('install-carplay-maps.sh','restore-carplay-maps.sh','dio_manager.wrapper.sh')){Copy-Item -LiteralPath (Join-Path $repo ('src/toolbox/'+$script)) -Destination (Join-Path $common $script)}
 Copy-Item -LiteralPath (Join-Path $repo 'src/toolbox/install-carplay-maps.sh') -Destination (Join-Path $pkg 'install.sh')
 Copy-Item -LiteralPath (Join-Path $repo 'src/toolbox/restore-carplay-maps.sh') -Destination (Join-Path $pkg 'restore-carplay-maps.sh')
 foreach($name in @('altscreen-most-bridge','carplay-cluster-supervisor','libcpvc-mirror-tap.so','libcpvc-dmdt-query.so','cpvc-mirror-canvas-790','cpvc-mirror-canvas-791')){Copy-Item -LiteralPath (Join-Path $NativeBuild ('common/'+$name)) -Destination (Join-Path $common $name)}
 foreach($name in @('VCMenu.jar','VCMirrorRate.jar','CarPlayClusterControls.jar')){Copy-Item -LiteralPath (Join-Path $MenuBuild ($mu+'/'+$name)) -Destination (Join-Path $specific $name)}
 Copy-Item -LiteralPath (Join-Path $specific 'CarPlayClusterControls.jar') -Destination (Join-Path $common 'CarPlayClusterControls.jar')
 Copy-Item -LiteralPath (Join-Path $NativeBuild ($mu+'/'+$cluster+'/core.so')) -Destination (Join-Path $specific 'core.so')
}
Write-Output 'V2_FIVE_PROFILES_STAGED'
