# Enable Massier Startup Configuration
# This script configures Massier to run automatically on Windows startup

Write-Host "Configuring Massier to run on startup..." -ForegroundColor Green

# Get the current directory
$installPath = $PSScriptRoot

# Enable startup using massier command
Write-Host "Enabling startup configuration..." -ForegroundColor Cyan
$result = & "$installPath\massier_87.exe" startup enable

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nStartup configuration enabled successfully!" -ForegroundColor Green
    Write-Host "Massier will now run automatically when Windows starts." -ForegroundColor Cyan
    Write-Host "`nYou can verify with:" -ForegroundColor Yellow
    Write-Host "  massier startup status" -ForegroundColor White
} else {
    Write-Host "`nFailed to enable startup configuration." -ForegroundColor Red
}
