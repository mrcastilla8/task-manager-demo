param([string]$QtRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'EntornoQtWindows.ps1')

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not [Environment]::Is64BitOperatingSystem) {
    throw 'ConcurrentSimulator requiere Windows de 64 bits para este flujo MinGW x64.'
}
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    throw 'No se encontró CMake. Instala CMake 3.20 o posterior desde https://cmake.org/download/ y vuelve a ejecutar este archivo.'
}
$qt = Find-Qt6MinGwRoot $QtRoot
$toolchain = Find-QtMinGwCompiler $qt
$qtVersion = (& (Join-Path $qt 'bin/qmake.exe') -query QT_VERSION).Trim()
Write-Host "Qt ${qtVersion}: $qt" -ForegroundColor Cyan
Write-Host "Compilador compatible: $($toolchain.Version)" -ForegroundColor Cyan
Write-Host "CMake: $((& $cmake.Source --version | Select-Object -First 1))" -ForegroundColor Cyan

$env:PATH = "$($toolchain.Bin);$(Join-Path $qt 'bin');$env:PATH"
$build = Join-Path $projectRoot 'build-windows-mingw'
$configure = @(
    '-S', $projectRoot, '-B', $build,
    '-G', 'MinGW Makefiles',
    "-DCMAKE_CXX_COMPILER=$($toolchain.Compiler)",
    "-DCMAKE_MAKE_PROGRAM=$($toolchain.Make)",
    "-DCMAKE_PREFIX_PATH=$qt",
    '-DCMAKE_BUILD_TYPE=Debug',
    '-DBUILD_TESTING=ON',
    '-DSIMULATOR_WARNINGS_AS_ERRORS=ON',
    '-DSIMULATOR_BUILD_GUI_PREVIEW=ON'
)
Invoke-CheckedNative $cmake.Source $configure
Invoke-CheckedNative $cmake.Source @('--build', $build, '--config', 'Debug', '--parallel', '2')
$exe = Join-Path $build 'gui/simulator_gui.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw "CMake terminó pero no se encontró el ejecutable esperado: $exe" }
Write-Host 'Iniciando ConcurrentSimulator. Cierra la ventana para terminar.' -ForegroundColor Green
& $exe
if ($LASTEXITCODE -ne 0) { throw "La aplicación terminó con código $LASTEXITCODE." }
