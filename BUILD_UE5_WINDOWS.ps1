param(
    [string]$UE_ROOT = $env:UE_ROOT,
    [string]$Configuration = "Development"
)
$ErrorActionPreference = "Stop"
if (-not $UE_ROOT) { throw "Set UE_ROOT to your Unreal Engine 5 installation, e.g. C:\Program Files\Epic Games\UE_5.5" }
$Project = Join-Path $PSScriptRoot "MK64Remake.uproject"
$UAT = Join-Path $UE_ROOT "Engine\Build\BatchFiles\RunUAT.bat"
if (!(Test-Path $UAT)) { throw "RunUAT.bat not found under UE_ROOT: $UE_ROOT" }
$Out = Join-Path $PSScriptRoot "Build\Windows"
& $UAT BuildCookRun -project="$Project" -noP4 -platform=Win64 -clientconfig=$Configuration -build -cook -allmaps -stage -pak -archive -archivedirectory="$Out"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Exe = Get-ChildItem $Out -Recurse -Filter "MK64Remake.exe" | Select-Object -First 1
if (!$Exe) { throw "Build completed but MK64Remake.exe was not found." }
Write-Host "PLAYABLE EXE: $($Exe.FullName)"
