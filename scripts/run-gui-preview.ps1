param(
    [string]$BuildDirectory = 'build-gui-qt',
    [string]$QtDirectory = 'build-tools/Qt/6.8.3/mingw_64',
    [string]$CompilerDirectory = 'build-tools/Qt/Tools/mingw1310_64'
)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $PSScriptRoot
$qtBin = Join-Path $projectDirectory "$QtDirectory/bin"
$compilerBin = Join-Path $projectDirectory "$CompilerDirectory/bin"
$candidates = @(
    (Join-Path $projectDirectory "$BuildDirectory/gui/Debug/simulator_gui.exe"),
    (Join-Path $projectDirectory "$BuildDirectory/gui/simulator_gui.exe")
)
$executable = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if ([string]::IsNullOrEmpty($executable)) {
    throw "No existe $executable. Compila el prototipo siguiendo gui/README.md."
}
$originalPath = $env:PATH
try {
    $env:PATH = "$qtBin;$compilerBin;$originalPath"
    & $executable
} finally {
    $env:PATH = $originalPath
}
