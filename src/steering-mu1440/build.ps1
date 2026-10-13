[CmdletBinding()]
param(
    [string]$OutputJar = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$jdk8 = 'C:\Program Files\Java\jdk1.8.0_202\bin'
$javac = Join-Path $jdk8 'javac.exe'
$jarTool = Join-Path $jdk8 'jar.exe'
$java = Join-Path $jdk8 'java.exe'
$classes = Join-Path $PSScriptRoot 'build\classes'
$testClasses = Join-Path $PSScriptRoot 'build\test-classes'

if ([string]::IsNullOrWhiteSpace($OutputJar)) {
    $packageRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $OutputJar = Join-Path $packageRoot 'MapControl\CarPlayClusterControls-SkodaRoller.jar'
}

if (!(Test-Path -LiteralPath $javac -PathType Leaf) -or
    !(Test-Path -LiteralPath $jarTool -PathType Leaf) -or
    !(Test-Path -LiteralPath $java -PathType Leaf)) {
    throw "JDK 8 toolchain is incomplete: $jdk8"
}

foreach ($path in @($classes, $testClasses)) {
    if (Test-Path -LiteralPath $path) {
        $buildRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'build')) + [IO.Path]::DirectorySeparatorChar
        if (![IO.Path]::GetFullPath($path).StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Build cleanup is outside the source build directory: $path"
        }
        Remove-Item -LiteralPath $path -Recurse -Force
    }
    New-Item -ItemType Directory -Path $path | Out-Null
}

$stubSources = @(
    'stubs\de\esolutions\fw\comm\core\IReplyService.java',
    'stubs\de\esolutions\fw\comm\dsi\keypanel\DSIKeyPanelReply.java',
    'stubs\de\esolutions\fw\comm\dsi\keypanel\impl\DSIKeyPanelReplyService.java',
    'stubs\de\esolutions\fw\dsi\base\AbstractDispatcher.java',
    'stubs\org\dsi\ifc\base\DSIListener.java',
    'stubs\org\dsi\ifc\keypanel\DSIKeyPanelListener.java'
) | ForEach-Object { Join-Path $PSScriptRoot $_ }

$productionSources = @(
    'src\de\esolutions\fw\dsi\keypanel\DSIKeyPanelDispatcher.java',
    'src\de\vw\mib\carplay\clustercontrol\Mhi2ClusterControlBridge.java',
    'src\de\vw\mib\carplay\clustercontrol\Mhi2ClusterKeyState.java'
) | ForEach-Object { Join-Path $PSScriptRoot $_ }

& $javac -encoding UTF-8 -source 1.4 -target 1.4 -Xlint:-options `
    -d $classes @stubSources @productionSources
if ($LASTEXITCODE -ne 0) { throw "Production compilation failed: $LASTEXITCODE" }

$outputDirectory = Split-Path -Parent $OutputJar
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
if (Test-Path -LiteralPath $OutputJar) { Remove-Item -LiteralPath $OutputJar -Force }
Push-Location $classes
try {
    & $jarTool cf $OutputJar `
        'de\esolutions\fw\dsi\keypanel' `
        'de\vw\mib\carplay\clustercontrol'
    if ($LASTEXITCODE -ne 0) { throw "JAR creation failed: $LASTEXITCODE" }
}
finally {
    Pop-Location
}

$testSource = Join-Path $PSScriptRoot 'tests\TestSkodaRollerBridge.java'
& $javac -encoding UTF-8 -source 1.4 -target 1.4 -Xlint:-options `
    -cp $classes -d $testClasses $testSource
if ($LASTEXITCODE -ne 0) { throw "Test compilation failed: $LASTEXITCODE" }
& $java -cp "$classes;$testClasses" TestSkodaRollerBridge
if ($LASTEXITCODE -ne 0) { throw "Host-side bridge test failed: $LASTEXITCODE" }

$hash = (Get-FileHash -LiteralPath $OutputJar -Algorithm SHA256).Hash
$length = (Get-Item -LiteralPath $OutputJar).Length
Write-Host "SKODA_ROLLER_JAR_BUILD_OK bytes=$length sha256=$hash"
