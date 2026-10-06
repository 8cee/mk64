param(
    [string]$UE_ROOT = $env:UE_ROOT,
    [string]$Configuration = "Development"
)

$ErrorActionPreference = "Stop"

if (-not $UE_ROOT) {
    throw "Set UE_ROOT to Unreal Engine 5.5, for example C:\\Program Files\\Epic Games\\UE_5.5"
}

$Project = Join-Path $PSScriptRoot "MK64Remake.uproject"
$UAT = Join-Path $UE_ROOT "Engine\\Build\\BatchFiles\\RunUAT.bat"
$Out = Join-Path $PSScriptRoot "Build\\Windows"

if (!(Test-Path $Project)) { throw "Missing project: $Project" }
if (!(Test-Path $UAT)) { throw "RunUAT.bat not found: $UAT" }

if (Test-Path $Out) { Remove-Item $Out -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Out | Out-Null

& $UAT BuildCookRun `
    -project="$Project" `
    -noP4 `
    -utf8output `
    -platform=Win64 `
    -clientconfig=$Configuration `
    -build `
    -cook `
    -map=/Engine/Maps/Entry `
    -stage `
    -pak `
    -archive `
    -archivedirectory="$Out"

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$Exe = Get-ChildItem $Out -Recurse -Filter "MK64Remake.exe" | Select-Object -First 1
if (!$Exe) { throw "Build completed but MK64Remake.exe was not found." }

Write-Host ""
Write-Host "PLAYABLE EXE: $($Exe.FullName)" -ForegroundColor Green
Write-Host "Controls: W/S accelerate-brake, A/D steer, F11 options."
