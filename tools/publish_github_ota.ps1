# Build firmware, create git tag, push, and let GitHub Actions publish the Release assets.
# Prerequisites:
#   - git remote origin pointing at the public GitHub repo
#   - gh auth login   (or git credentials that can push tags)
#   - OTA_GITHUB_OWNER/REPO in include/ota_github.h match that repo
#
# Usage:
#   .\tools\publish_github_ota.ps1 -Version 0.1.1

param(
    [Parameter(Mandatory = $true)][string]$Version
)

$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
$env:Path = "C:\Program Files\Git\cmd;C:\Program Files\Git\bin;" + $env:Path

if ($Version -notmatch '^\d+\.\d+\.\d+$') {
    throw "Version must look like 0.1.1 (no leading v)"
}

$VersionHeader = Join-Path $Root "include\version.h"
$header = Get-Content $VersionHeader -Raw
$header = [regex]::Replace($header, '#define FW_VERSION\s+".*"', "#define FW_VERSION `"$Version`"")
Set-Content -Path $VersionHeader -Value $header -NoNewline

Write-Host "[1/4] FW_VERSION -> $Version"

$Pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
Write-Host "[2/4] Local build check..."
& $Pio run -e esp32-2432S028 -d $Root
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Push-Location $Root
try {
    $status = git status --porcelain
    if (-not $status -and -not (Test-Path (Join-Path $Root ".git"))) {
        throw "Not a git repository. Run: git init && git remote add origin <url>"
    }

    Write-Host "[3/4] Commit version bump (if needed) and tag v$Version"
    git add include/version.h
    $pending = git status --porcelain include/version.h
    if ($pending) {
        git commit -m "Bump firmware version to $Version"
    }

    $tag = "v$Version"
    $existing = git tag -l $tag
    if ($existing) { throw "Tag $tag already exists" }
    git tag -a $tag -m "OTA release $Version"

    Write-Host "[4/4] Push commit + tag (GitHub Actions will attach firmware.bin + version.json)"
    git push origin HEAD
    git push origin $tag

    $remote = git remote get-url origin
    Write-Host ""
    Write-Host "Pushed $tag to $remote"
    Write-Host "After Actions finishes, devices will see:"
    Write-Host "  https://github.com/<owner>/<repo>/releases/latest/download/version.json"
} finally {
    Pop-Location
}
