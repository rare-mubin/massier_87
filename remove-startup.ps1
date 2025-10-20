# Remove Massier Startup Configuration
# This script only removes Massier from Windows startup (does not uninstall)

Write-Host "Removing Massier from startup..." -ForegroundColor Yellow

# Get the current directory
$installPath = $PSScriptRoot

# Disable startup using massier command
Write-Host "Disabling startup configuration..." -ForegroundColor Cyan
$result = & "$installPath\massier_87.exe" startup disable

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nStartup configuration removed successfully!" -ForegroundColor Green
    Write-Host "Massier will no longer run automatically on Windows startup." -ForegroundColor Cyan
    Write-Host "`nNote: Massier is still installed and can be used manually with:" -ForegroundColor Yellow
    Write-Host "  massier run" -ForegroundColor White
} else {
    Write-Host "`nFailed to remove startup configuration." -ForegroundColor Red
}
