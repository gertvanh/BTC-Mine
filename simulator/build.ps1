$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$PoolSim = Join-Path $Root "..\..\Pool-display-simulator"

function Find-Tool($names, $candidates) {
    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) { return $c }
    }
    foreach ($n in $names) {
        $p = Get-Command $n -ErrorAction SilentlyContinue
        if ($p) { return $p.Source }
    }
    return $null
}

$MingwCandidates = @(
    (Join-Path $PoolSim "tools\mingw64\bin\gcc.exe"),
    (Join-Path $Root "tools\mingw64\bin\gcc.exe"),
    "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe"
)
$CmakeCandidates = @(
    (Join-Path $PoolSim "tools\cmake\bin\cmake.exe"),
    "C:\Program Files\CMake\bin\cmake.exe",
    (Join-Path $Root "tools\cmake\bin\cmake.exe")
)

$Gcc = Find-Tool @("gcc") $MingwCandidates
$Cmake = Find-Tool @("cmake") $CmakeCandidates

if (-not $Gcc -or -not $Cmake) {
    $setup = Join-Path $PoolSim "setup_tools.ps1"
    if (Test-Path $setup) {
        Write-Host "[INFO] Tools ontbreken. Pool-display-simulator setup_tools.ps1 draaien..."
        & $setup
        $Gcc = Find-Tool @("gcc") $MingwCandidates
        $Cmake = Find-Tool @("cmake") $CmakeCandidates
    }
}
if (-not $Gcc -or -not $Cmake) { throw "MinGW of CMake ontbreekt. Installeer CMake + MinGW, of run Pool-display-simulator\setup_tools.ps1" }

$SdlDll = Join-Path $PoolSim "SDL2-2.30.12\x86_64-w64-mingw32\bin\SDL2.dll"
if (-not (Test-Path $SdlDll)) {
    throw "SDL2 niet gevonden op $SdlDll. Verwacht naast BTC-Mine: Pool-display-simulator\SDL2-2.30.12"
}

$MingwBin = Split-Path $Gcc -Parent
$CmakeBin = Split-Path $Cmake -Parent
$env:PATH = "$MingwBin;$CmakeBin;$env:PATH"
Write-Host "[OK] GCC: $Gcc"
Write-Host "[OK] CMake: $Cmake"

$BuildDir = Join-Path $Root "build"
if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null }
Push-Location $BuildDir
try {
    & cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "CMake configuratie mislukt." }
    & cmake --build . --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build mislukt." }
    Write-Host ""
    Write-Host "Build geslaagd: $BuildDir\btc_mine_sim.exe"
    if ($args -contains "-Run") {
        & ".\btc_mine_sim.exe"
    }
} finally {
    Pop-Location
}
