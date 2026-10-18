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
$testDir = Join-Path $env:TEMP ("melee-renderer-restart-test-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDir | Out-Null
try {
    $source = Join-Path $repoRoot 'src\pc\renderer_restart.c'
    $test = Join-Path $repoRoot 'src\pc\tests\renderer_restart_test.c'
    $include = Join-Path $repoRoot 'src\pc'
    $exe = Join-Path $testDir 'renderer_restart_test.exe'
    $compile = '"{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /W4 /WX /std:c11 /I"{1}" "{2}" "{3}" /Fe:"{4}"' -f `
        $devCmd, $include, $source, $test, $exe
    Push-Location $testDir
    try {
        & $env:ComSpec /d /s /c $compile
        if ($LASTEXITCODE -ne 0) { throw "Renderer restart test build failed (exit $LASTEXITCODE)." }
        & $exe --test-invalid-backend
        if ($LASTEXITCODE -ne 0) { throw "Invalid-backend test failed (exit $LASTEXITCODE)." }
        & $exe --test-repeated-arguments
        if ($LASTEXITCODE -ne 0) { throw "Repeated renderer arguments test failed (exit $LASTEXITCODE)." }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Successful handoff test failed (exit $LASTEXITCODE)." }
        & $exe --test-child-fail
        if ($LASTEXITCODE -ne 0) { throw "Failed-child fallback test failed (exit $LASTEXITCODE)." }
    }
    finally { Pop-Location }
}
finally {
    Remove-Item -LiteralPath $testDir -Recurse -Force -ErrorAction SilentlyContinue
}
