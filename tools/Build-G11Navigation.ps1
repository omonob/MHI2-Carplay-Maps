[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$Mu1433Jar,
 [Parameter(Mandatory=$true)][string]$BuildRoot,
 [string]$Jdk='C:\Program Files\Java\jdk-17\bin'
)
$ErrorActionPreference='Stop';Set-StrictMode -Version Latest
if(Test-Path -LiteralPath $BuildRoot){throw 'Use a fresh output folder'}
$repo=Split-Path -Parent $PSScriptRoot;$source=Join-Path $repo 'src/g11-navigation'
$root=[IO.Path]::GetFullPath($BuildRoot);$tools=Join-Path $root 'tools'
New-Item -ItemType Directory -Path $tools -Force|Out-Null
$exports=@('--add-exports','java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED','--add-exports','java.base/jdk.internal.org.objectweb.asm.tree=ALL-UNNAMED')
& "$Jdk\javac.exe" @exports -d $tools (Join-Path $source 'BuildG11Navigation.java') (Join-Path $source 'AuditG11Navigation.java')
if($LASTEXITCODE){throw 'Navigation build/audit tool failed'}
foreach($profile in ,@('MU1433',$Mu1433Jar)){
 $mu=$profile[0];$stock=[IO.Path]::GetFullPath($profile[1]);$dir=Join-Path $root $mu
 $classes=Join-Path $dir 'classes';$tests=Join-Path $dir 'tests'
 New-Item -ItemType Directory -Path $classes,$tests -Force|Out-Null
 & "$Jdk\javac.exe" -encoding UTF-8 -g:none -source 7 -target 7 -Xlint:-options -cp $stock -d $classes (Join-Path $source 'G11NavigationRequestFilter.java')
 if($LASTEXITCODE){throw ('Exact stock API helper failed '+$mu)}
 $jar=Join-Path $dir 'CarPlayMapsG11Navigation.jar'
 & "$Jdk\java.exe" @exports -cp $tools BuildG11Navigation $stock $classes $jar
 if($LASTEXITCODE){throw ('Exact navigation build failed '+$mu)}
 & "$Jdk\java.exe" @exports -cp $tools AuditG11Navigation $stock $jar
 if($LASTEXITCODE){throw ('Unexpected original bytecode changes '+$mu)}
 & "$Jdk\javac.exe" -encoding UTF-8 -source 7 -target 7 -Xlint:-options -cp ($jar+';'+$stock) -d $tests (Join-Path $source 'TestG11Navigation.java')
 if($LASTEXITCODE){throw ('Navigation tests failed compilation '+$mu)}
 & "$Jdk\java.exe" -Xverify:all -cp ($tests+';'+$jar+';'+$stock) TestG11Navigation
 if($LASTEXITCODE){throw ('Navigation behavior failed '+$mu)}
 Get-FileHash -LiteralPath $jar -Algorithm SHA256
}
Write-Output 'G11_NAVIGATION_REPRODUCIBLE_BUILD_PASS exact_original_ABI=1 real_unit_tested=0'
