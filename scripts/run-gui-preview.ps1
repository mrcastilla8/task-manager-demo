param(
    [string]$BuildDirectory = 'build-gui-qt',
    [string]$QtDirectory = 'build-tools/Qt/6.8.3/mingw_64',
    [string]$CompilerDirectory = 'build-tools/Qt/Tools/mingw1310_64'
)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $PSScriptRoot
$qtBin = Join-Path $projectDirectory "$QtDirectory/bin"
$compilerBin = Join-Path $projectDirectory "$CompilerDirectory/bin"
$executable = Join-Path $projectDirectory "$BuildDirectory/gui/simulator_gui_preview.exe"
if (-not (Test-Path -LiteralPath $executable)) {
    throw "No existe $executable. Compila el prototipo siguiendo gui/README.md."
}
$originalPath = $env:PATH
try {
    $env:PATH = "$qtBin;$compilerBin;$originalPath"
    & $executable
} finally {
    $env:PATH = $originalPath
}
