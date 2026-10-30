[CmdletBinding()]
param(
 [string]$Mu1102Jar='',
 [string]$Mu1367Jar='',
 [string]$Mu1440Jar='',
 [string]$Mu1433Jar='',
 [string]$Mu1447Jar='',
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [string]$Jdk='C:\Program Files\Java\jdk-17\bin'
)
$ErrorActionPreference='Stop'; Set-StrictMode -Version Latest
$repo=Split-Path -Parent $PSScriptRoot
$module=Join-Path $repo 'src/vc-menu'
if(Test-Path -LiteralPath $BuildRoot){throw 'Use a fresh build directory'}
New-Item -ItemType Directory -Path $BuildRoot | Out-Null
$tools=Join-Path $BuildRoot 'tools';New-Item -ItemType Directory -Path $tools | Out-Null
$toolSources=@('BuildNativeHook.java','BuildControlHook.java','BuildMirrorRateTest.java','TestNavigationOwnershipHook.java') | ForEach-Object {Join-Path $module ('tools/'+$_)}
$asmExports=@('--add-exports','java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED','--add-exports','java.base/jdk.internal.org.objectweb.asm.tree=ALL-UNNAMED','--add-exports','java.base/jdk.internal.org.objectweb.asm.util=ALL-UNNAMED')
& "$Jdk\javac.exe" @asmExports -encoding UTF-8 -d $tools @toolSources
if($LASTEXITCODE){throw 'Build tool compilation failed'}
$allSources=@(Get-ChildItem -LiteralPath (Join-Path $module 'src') -Filter '*.java' -Recurse | Select-Object -ExpandProperty FullName)
$profiles=[ordered]@{MU1102=$Mu1102Jar;MU1367=$Mu1367Jar;MU1440=$Mu1440Jar;MU1433=$Mu1433Jar;MU1447=$Mu1447Jar}
$selected=@($profiles.Keys|Where-Object{$profiles[$_]})
if(!$selected.Count){throw 'Supply at least one exact original firmware JAR.'}
$reports=@()
foreach($mu in $selected){
 $stock=[IO.Path]::GetFullPath($profiles[$mu]);if(!(Test-Path -LiteralPath $stock)){throw ('Missing user-supplied original '+$mu)}
 $dir=Join-Path $BuildRoot $mu;$classes=Join-Path $dir 'classes';$tests=Join-Path $dir 'tests'
 $nativeUi=$true
 $sources=@($allSources|Where-Object {$nativeUi -or ($_ -notmatch '(IndependentMenuViewBridge|ProjectionMenuViewBridge|CpmapG11ListAdapter)\.java$')})
 New-Item -ItemType Directory -Path $dir,$classes,$tests -Force|Out-Null
 & "$Jdk\javac.exe" -encoding UTF-8 -g:none -source 7 -target 7 -Xlint:-options -cp $stock -d $classes @sources
 if($LASTEXITCODE){throw ('Exact firmware API compilation failed '+$mu)}
 $menu=Join-Path $dir 'VCMenu.jar'
 $hookArgs=@($stock,$classes,$menu);if(!$nativeUi){$hookArgs+='--no-native-ui'}
 & "$Jdk\java.exe" --add-exports java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED -cp $tools BuildNativeHook @hookArgs
 if($LASTEXITCODE){throw ('Native lifecycle hook failed '+$mu)}
 & "$Jdk\java.exe" @asmExports -cp $tools TestNavigationOwnershipHook $stock $menu
 if($LASTEXITCODE){throw ('Original CarPlay callback scope changed '+$mu)}
 $rate=Join-Path $dir 'VCMirrorRate.jar'
 & "$Jdk\java.exe" --add-exports java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED -cp $tools BuildMirrorRateTest $stock $classes $rate (Join-Path $dir 'ChangeDataRateSequence.original.class')
 if($LASTEXITCODE){throw ('Mirror cadence ABI failed '+$mu)}
 # Immutable published inputs, never a previously staged/routed output.
 $skoda=$mu -in @('MU1433','MU1440')
 $controlName=if($skoda){'Skoda-Controls-v1.2.2.jar'}else{'VW-Controls-v1.2.2.jar'}
 $control=Join-Path $module ('baseline/'+$controlName)
 $controlHash=if($skoda){'8BD1C88F6E00DF44A8ACA8302AC1E0F2A49EF0679788B07FBBCCFA1F014970AC'}else{'C8CEAAD026C2EFA20EC190260CB0BA7C982CCF7F1B3754C31C1930FA461AC032'}
 if((Get-FileHash -LiteralPath $control).Hash -ne $controlHash){throw 'Original published controls baseline mismatch'}
 & "$Jdk\java.exe" --add-exports java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED -cp $tools BuildControlHook $control (Join-Path $dir 'CarPlayClusterControls.jar') $menu
 if($LASTEXITCODE){throw ('Wheel canvas routing failed '+$mu)}
 $testNames=@('CardPositionSettingsTest','Card791OptionsTest','MapSwitchSettingsTest','ProjectionSettingsTest','WirelessCarPlaySettingsTest','ProjectionPolicyTest','FeatureFlowTest','PublicProfileTest','CarPlayNavigationOwnershipTest')
 $testSources=$testNames | ForEach-Object {Join-Path $module ('tests/'+$_+'.java')}
 foreach($cli in @('CardStateCli','ProjectionStateCli','MapSwitchStateCli','WirelessStateCli')){$testSources+=Join-Path $module ('tests/'+$cli+'.java')}
 & "$Jdk\javac.exe" -encoding UTF-8 -source 7 -target 7 -Xlint:-options -cp ($menu+';'+$stock) -d $tests @testSources
 if($LASTEXITCODE){throw ('Test compilation failed '+$mu)}
 foreach($test in $testNames){
  & "$Jdk\java.exe" -Xverify:all '-Dcpvc.offline=1' '-Dcpmap.ui.cluster=790' -cp ($tests+';'+$menu+';'+$stock) $test (Join-Path $dir ('fixture-'+$test))
  if($LASTEXITCODE){throw ($mu+' '+$test+' failed')}
 }
 $reloadCp=$tests+';'+$menu+';'+$stock
 foreach($scenario in @(@('CardStateCli','0','position=0 generation=1'),@('WirelessStateCli','1','enabled=1 generation=1'))){
  $fixture=Join-Path $dir ('separate-jvm-'+$scenario[0])
  $first=@(& "$Jdk\java.exe" -Xverify:all -cp $reloadCp $scenario[0] $fixture $scenario[1]);if($LASTEXITCODE -ne 0 -or ($first -join '') -notmatch $scenario[2]){throw 'Separate writer failed'}
  $second=@(& "$Jdk\java.exe" -Xverify:all -cp $reloadCp $scenario[0] $fixture);if($LASTEXITCODE -ne 0 -or ($second -join '') -notmatch ($scenario[2]+' source=saved')){throw 'Separate JVM lost saved settings'}
 }
 Write-Output ($mu+' SEPARATE_JVM_CARD_AND_WIRELESS_RELOAD_PASS')
 $routing=if($skoda){'SkodaCanvasRoutingTest'}else{'ActualControlHookTest'}
 $routingCp=(Join-Path $dir 'CarPlayClusterControls.jar')+';'+$menu+';'+$stock
 & "$Jdk\javac.exe" -encoding UTF-8 -source 7 -target 7 -Xlint:-options -cp $routingCp -d $tests (Join-Path $module ('tests/'+$routing+'.java'))
 if($LASTEXITCODE){throw ('Real wheel test compilation failed '+$mu)}
 & "$Jdk\java.exe" -Xverify:all -cp ($tests+';'+$routingCp) $routing (Join-Path $dir 'fixture-real-wheel')
 if($LASTEXITCODE){throw ('Real wheel map/canvas routing failed '+$mu)}
 $reports+=@([ordered]@{firmware=$mu;baseline_sha256=(Get-FileHash -LiteralPath $stock).Hash;menu_sha256=(Get-FileHash -LiteralPath $menu).Hash;api_compile='pass';hook_count=$(if($nativeUi){4}else{0});native_ui=$nativeUi;locales=31;host_tests='pass';live_ui='not_vehicle_tested'})
}
$reports|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $BuildRoot 'menu-build-report.json') -Encoding UTF8
Write-Output ('V2_EXACT_FIRMWARE_MENU_BUILD_AND_HOST_TESTS_PASS count='+$reports.Count)
