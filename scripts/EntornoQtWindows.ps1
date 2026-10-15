function Find-Qt6MinGwRoot {
    param([string]$RequestedRoot)

    $candidates = [System.Collections.Generic.List[string]]::new()
    if ($RequestedRoot) { $candidates.Add($RequestedRoot) }
    foreach ($name in @('QT_ROOT', 'QTDIR')) {
        $value = [Environment]::GetEnvironmentVariable($name)
        if ($value) { $candidates.Add($value) }
    }
    if ($env:Qt6_DIR -and (Test-Path -LiteralPath $env:Qt6_DIR)) {
        $root = (Resolve-Path $env:Qt6_DIR).Path
        for ($i = 0; $i -lt 3; $i++) { $root = Split-Path -Parent $root }
        $candidates.Add($root)
    }
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmake) { $candidates.Add((Split-Path -Parent (Split-Path -Parent $qmake.Source))) }

    $searchRoots = @('C:\Qt', "$env:USERPROFILE\Qt", "$env:LOCALAPPDATA\Programs\Qt")
    foreach ($searchRoot in $searchRoots) {
        if (Test-Path -LiteralPath $searchRoot) {
            Get-ChildItem -LiteralPath $searchRoot -Directory -ErrorAction SilentlyContinue |
                ForEach-Object {
                    $kit = Join-Path $_.FullName 'mingw_64'
                    if (Test-Path -LiteralPath $kit) { $candidates.Add($kit) }
                }
        }
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if (-not (Test-Path -LiteralPath $candidate)) { continue }
        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        if ((Test-Path (Join-Path $resolved 'lib/cmake/Qt6/Qt6Config.cmake')) -and
            (Test-Path (Join-Path $resolved 'bin/qmake.exe')) -and
            (Test-Path (Join-Path $resolved 'bin/windeployqt.exe')) -and
            (Test-Path (Join-Path $resolved 'lib/cmake/Qt6Widgets/Qt6WidgetsConfig.cmake')) -and
            (Test-Path (Join-Path $resolved 'lib/cmake/Qt6Test/Qt6TestConfig.cmake'))) {
            $configText = Get-Content -LiteralPath (Join-Path $resolved 'mkspecs/qconfig.pri') -Raw -ErrorAction SilentlyContinue
            $architecture = [regex]::Match($configText, '(?m)^QT_ARCH\s*=\s*(\S+)').Groups[1].Value
            if ($architecture -eq 'x86_64') { return $resolved }
        }
    }

    throw 'Qt 6 con kit MinGW x64 no está instalado o no se encontró. Instala Qt 6 para MinGW x64 y vuelve a ejecutar. Guía oficial: https://www.qt.io/download-qt-installer-oss'
}

function Find-QtMinGwCompiler {
    param([Parameter(Mandatory)][string]$QtRoot)

    $qtVersionDir = Split-Path -Parent $QtRoot
    $qtInstallRoot = Split-Path -Parent $qtVersionDir
    $toolDirectories = @()
    $toolsRoot = Join-Path $qtInstallRoot 'Tools'
    if (Test-Path -LiteralPath $toolsRoot) {
        $toolDirectories += Get-ChildItem -LiteralPath $toolsRoot -Directory -ErrorAction SilentlyContinue
    }
    $gxx = Get-Command g++ -ErrorAction SilentlyContinue
    if ($gxx) { $toolDirectories += Get-Item (Split-Path -Parent (Split-Path -Parent $gxx.Source)) }

    $qconfig = Join-Path $QtRoot 'mkspecs/qconfig.pri'
    $expected = $null
    if (Test-Path -LiteralPath $qconfig) {
        $configText = Get-Content -LiteralPath $qconfig -Raw
        $major = [regex]::Match($configText, '(?m)^QT_GCC_MAJOR_VERSION\s*=\s*(\d+)').Groups[1].Value
        $minor = [regex]::Match($configText, '(?m)^QT_GCC_MINOR_VERSION\s*=\s*(\d+)').Groups[1].Value
        if ($major -and $minor) { $expected = "$major.$minor" }
    }
    foreach ($tool in $toolDirectories) {
        $compiler = Join-Path $tool.FullName 'bin/g++.exe'
        $make = Join-Path $tool.FullName 'bin/mingw32-make.exe'
        if ((Test-Path -LiteralPath $compiler) -and (Test-Path -LiteralPath $make)) {
            $compilerVersion = (& $compiler --version | Select-Object -First 1)
            if ($expected -and $compilerVersion -notmatch "\b$([regex]::Escape($expected))\.") {
                continue
            }
            return [pscustomobject]@{
                Home = $tool.FullName
                Bin = Join-Path $tool.FullName 'bin'
                Compiler = $compiler
                Make = $make
                Version = $compilerVersion
            }
        }
    }

    throw 'No se encontró MinGW compatible con el kit Qt seleccionado. Instala el compilador MinGW ofrecido para ese kit de Qt; no combines Qt MinGW con MSVC.'
}

function Invoke-CheckedNative {
    param([Parameter(Mandatory)][string]$FilePath, [string[]]$ArgumentList = @())
    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Falló '$FilePath' (código $LASTEXITCODE). Revisa el mensaje anterior y confirma que Qt/MinGW coinciden."
    }
}
