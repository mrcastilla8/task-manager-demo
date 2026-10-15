param(
    [string]$QtRoot,
    [string]$BuildDirectory = 'build-portable-release',
    [string]$OutputDirectory = 'dist'
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'EntornoQtWindows.ps1')

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not [Environment]::Is64BitOperatingSystem) {
    throw 'El paquete ConcurrentSimulator-Windows-x64 requiere Windows x64.'
}
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) { throw 'Falta CMake 3.20+. Instálalo desde https://cmake.org/download/.' }
$qt = Find-Qt6MinGwRoot $QtRoot
$toolchain = Find-QtMinGwCompiler $qt
$qmake = Join-Path $qt 'bin/qmake.exe'
$qtVersion = (& $qmake -query QT_VERSION).Trim()
$qtConfig = Join-Path $qt 'lib/cmake/Qt6/Qt6Config.cmake'
if (-not (Test-Path -LiteralPath $qtConfig)) { throw "Qt6Config.cmake no existe bajo el kit detectado: $qt" }
Write-Host "Qt $qtVersion (MinGW x64): $qt" -ForegroundColor Cyan
Write-Host "Compilador: $($toolchain.Version)" -ForegroundColor Cyan
Write-Host "CMake: $((& $cmake.Source --version | Select-Object -First 1))" -ForegroundColor Cyan

$env:PATH = "$($toolchain.Bin);$(Join-Path $qt 'bin');$env:PATH"
$buildPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $BuildDirectory))
$rootPrefix = $projectRoot.TrimEnd('\') + '\'
if (-not $buildPath.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildDirectory debe permanecer dentro del repositorio.'
}
$argsCmake = @(
    '-S', $projectRoot, '-B', $buildPath,
    '-G', 'MinGW Makefiles',
    "-DCMAKE_CXX_COMPILER=$($toolchain.Compiler)",
    "-DCMAKE_MAKE_PROGRAM=$($toolchain.Make)",
    "-DCMAKE_PREFIX_PATH=$qt",
    '-DCMAKE_BUILD_TYPE=Release',
    '-DCMAKE_CXX_STANDARD=17',
    '-DBUILD_TESTING=OFF',
    '-DSIMULATOR_WARNINGS_AS_ERRORS=ON',
    '-DSIMULATOR_BUILD_GUI_PREVIEW=ON'
)
Invoke-CheckedNative $cmake.Source $argsCmake
Invoke-CheckedNative $cmake.Source @('--build', $buildPath, '--config', 'Release', '--parallel', '2')

$exe = Join-Path $buildPath 'gui/simulator_gui.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw "No se generó el ejecutable Release: $exe" }
$outputPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
if (-not $outputPath.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputDirectory debe permanecer dentro del repositorio.'
}
$stage = Join-Path $outputPath 'ConcurrentSimulator-Windows-x64'
if ([IO.Path]::GetFullPath($stage) -ne (Join-Path $outputPath 'ConcurrentSimulator-Windows-x64')) {
    throw 'La carpeta de staging no coincide con la ruta de salida esperada.'
}
if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination $stage
$windeployqt = Join-Path $qt 'bin/windeployqt.exe'
Invoke-CheckedNative $windeployqt @('--release', '--no-translations', (Join-Path $stage 'simulator_gui.exe'))

# windeployqt copia el runtime MinGW que detecta; verificar y completar las DLL
# compartidas de este compilador sin copiar el SDK ni el árbol MinGW completo.
foreach ($dll in @('libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
    $runtime = Join-Path $toolchain.Bin $dll
    if (-not (Test-Path -LiteralPath $runtime)) { throw "Falta runtime requerido de MinGW: $runtime" }
    if (-not (Test-Path -LiteralPath (Join-Path $stage $dll))) {
        Copy-Item -LiteralPath $runtime -Destination $stage
    }
}
$platformPlugin = Join-Path $stage 'platforms/qwindows.dll'
if (-not (Test-Path -LiteralPath $platformPlugin)) {
    throw 'windeployqt no incluyó platforms/qwindows.dll; el paquete no se puede iniciar en Windows.'
}

$licenses = Join-Path $stage 'licencias'
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
$repoLicenses = Join-Path $projectRoot 'licenses'
if (Test-Path -LiteralPath $repoLicenses) {
    Copy-Item -Path (Join-Path $repoLicenses '*') -Destination $licenses -Recurse -Force
}
$qtLicenseSources = @(
    (Join-Path (Split-Path -Parent $qt) 'licenses'),
    (Join-Path (Split-Path -Parent (Split-Path -Parent $qt)) 'licenses')
)
foreach ($qtLicenseSource in $qtLicenseSources | Select-Object -Unique) {
    if (Test-Path -LiteralPath $qtLicenseSource) {
        Copy-Item -Path (Join-Path $qtLicenseSource '*') -Destination $licenses -Recurse -Force
    }
}
$mingwLicenseSource = Join-Path $toolchain.Home 'licenses'
if (Test-Path -LiteralPath $mingwLicenseSource) {
    Copy-Item -LiteralPath $mingwLicenseSource -Destination (Join-Path $licenses 'MinGW') -Recurse -Force
}
$qtSbom = Join-Path $qt 'sbom'
if (Test-Path -LiteralPath $qtSbom) {
    $sbomFiles = Get-ChildItem -LiteralPath $qtSbom -File |
        Where-Object { $_.Name -match '^(qtbase|qtsvg)-.*\.spdx' }
    if ($sbomFiles) {
        $qtSbomOutput = Join-Path $licenses 'Qt-SBOM'
        New-Item -ItemType Directory -Force -Path $qtSbomOutput | Out-Null
        Copy-Item -LiteralPath $sbomFiles.FullName -Destination $qtSbomOutput
    }
}
$projectReadme = Join-Path $projectRoot 'README.md'
if (Test-Path -LiteralPath $projectReadme) { Copy-Item -LiteralPath $projectReadme -Destination (Join-Path $stage 'LEEME.md') }
$projectLicense = Join-Path $projectRoot 'LICENSE'
if (Test-Path -LiteralPath $projectLicense) { Copy-Item -LiteralPath $projectLicense -Destination $stage }
$notices = @"
ConcurrentSimulator para Windows x64
Qt $qtVersion, kit MinGW x64; compilador $($toolchain.Version)

Qt se distribuye bajo los términos de sus licencias aplicables (incluida LGPLv3
para componentes ofrecidos bajo LGPL, además de avisos de terceros). Consulte
https://www.qt.io/licensing/ y https://doc.qt.io/qt-6/licensing.html. Se incluye
el SBOM de qtbase cuando está disponible en el kit de Qt.

El runtime GCC/libstdc++ está sujeto a GPLv3 con GCC Runtime Library Exception;
MinGW-w64 y otros componentes conservan sus propios avisos. Consulte las copias
incluidas en licencias/MinGW y https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html.

Este repositorio no contiene un archivo LICENSE que defina una licencia del
código propio. Este paquete local no asigna ni implica una licencia al proyecto.
Antes de publicar una descarga, confirme la licencia del proyecto y que los
avisos de Qt y del compilador incluidos correspondan exactamente al kit usado.
"@
Set-Content -LiteralPath (Join-Path $stage 'AVISOS-DE-LICENCIAS.txt') -Value $notices -Encoding UTF8

$zip = Join-Path $outputPath 'ConcurrentSimulator-Windows-x64.zip'
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
$bytes = (Get-Item -LiteralPath $zip).Length
Write-Host "ZIP: $zip" -ForegroundColor Green
Write-Host "Tamaño: $bytes bytes" -ForegroundColor Green
Write-Host "SHA-256: $hash" -ForegroundColor Green
Write-Host "Validado: ejecutable Release, plugin Windows y DLL MinGW requeridas." -ForegroundColor Green
