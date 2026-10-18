$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio C++ Build Tools were not found (vswhere.exe is missing).'
}
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vsRoot) {
    throw 'Visual Studio C++ Build Tools are not installed.'
}
$devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path -LiteralPath $devCmd)) { throw "Missing compiler setup script: $devCmd" }

$testDir = Join-Path $env:TEMP ("melee-frame-clock-test-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDir | Out-Null
try {
    $source = Join-Path $repoRoot 'src\pc\frame_clock.c'
    $test = Join-Path $repoRoot 'src\pc\tests\frame_clock_test.c'
    $include = Join-Path $repoRoot 'src\pc'
    $exe = Join-Path $testDir 'frame_clock_test.exe'
    $cmd = '"{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /W4 /WX /std:c11 /I"{1}" "{2}" "{3}" /Fe:frame_clock_test.exe && frame_clock_test.exe' -f `
        $devCmd, $include, $source, $test
    Push-Location $testDir
    try {
        & $env:ComSpec /d /s /c $cmd
        if ($LASTEXITCODE -ne 0) { throw "Frame clock tests failed (exit $LASTEXITCODE)." }
    }
    finally { Pop-Location }
}
finally {
    Remove-Item -LiteralPath $testDir -Recurse -Force -ErrorAction SilentlyContinue
}
