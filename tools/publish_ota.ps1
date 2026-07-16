# Publish firmware.bin + version.json for HTTP pull-OTA.
# Usage:
#   .\tools\publish_ota.ps1 -Version 0.1.1 -DestDir \\server\share\btc-mine
#   .\tools\publish_ota.ps1 -Version 0.1.1 -DestDir C:\ota\btc-mine -FirmwareUrl http://192.168.1.3/btc-mine/firmware.bin

param(
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$DestDir,
    [string]$FirmwareUrl = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
$env:Path = "C:\Program Files\Git\cmd;C:\Program Files\Git\bin;" + $env:Path
$Pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"

Write-Host "[1/3] Build firmware..."
& $Pio run -e esp32-2432S028 -d $Root
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$Bin = Join-Path $Root ".pio\build\esp32-2432S028\firmware.bin"
if (-not (Test-Path $Bin)) { throw "firmware.bin not found: $Bin" }

New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
Copy-Item $Bin (Join-Path $DestDir "firmware.bin") -Force

if (-not $FirmwareUrl) {
    $FirmwareUrl = "http://192.168.1.3/btc-mine/firmware.bin"
}

$manifest = @"
{
  "version": "$Version",
  "url": "$FirmwareUrl"
}
"@
Set-Content -Path (Join-Path $DestDir "version.json") -Value $manifest -Encoding UTF8

Write-Host "[2/3] Wrote $DestDir\firmware.bin"
Write-Host "[3/3] Wrote $DestDir\version.json -> $Version"
Write-Host "Bump FW_VERSION in include\version.h to $Version before the next local build if needed."
Write-Host "Done."
