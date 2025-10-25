# Massier Uninstallation Script
# This script removes Massier from your PATH

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Requesting administrator privileges..." -ForegroundColor Yellow
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

Write-Host "Uninstalling Massier..." -ForegroundColor Yellow

# Get the current directory
$installPath = $PSScriptRoot

# Stop Massier if running
Write-Host "Stopping Massier if running..." -ForegroundColor Cyan
& "$installPath\massier_87.exe" stop

# Disable startup if configured
Write-Host "Removing startup configuration..." -ForegroundColor Cyan
& "$installPath\massier_87.exe" startup disable

# Get current user PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# Remove from PATH
if ($currentPath -like "*$installPath*") {
    $pathArray = $currentPath -split ";" | Where-Object { $_ -ne $installPath }
    $newPath = $pathArray -join ";"
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "Removed $installPath from PATH." -ForegroundColor Green
} else {
    Write-Host "Massier directory was not in PATH." -ForegroundColor Yellow
}

# Remove batch file
$batchPath = "$installPath\massier.bat"
if (Test-Path $batchPath) {
    Remove-Item $batchPath
    Write-Host "Removed massier.bat" -ForegroundColor Green
}

# Remove VBScript file
$vbsPath = "$installPath\massier_silent.vbs"
if (Test-Path $vbsPath) {
    Remove-Item $vbsPath
    Write-Host "Removed massier_silent.vbs" -ForegroundColor Green
}

# Remove executable
$exePath = "$installPath\massier_87.exe"
if (Test-Path $exePath) {
    Remove-Item $exePath
    Write-Host "Removed massier_87.exe" -ForegroundColor Green
}

Write-Host "`nUninstallation complete!" -ForegroundColor Green
Write-Host "You can now delete the Massier_87 folder if desired." -ForegroundColor Cyan
