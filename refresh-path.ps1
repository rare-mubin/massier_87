# Refresh PATH in current PowerShell session
$userPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
$machinePath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
$env:Path = $userPath + ";" + $machinePath

Write-Host "PATH refreshed in current session!" -ForegroundColor Green
Write-Host "You can now use: massier run, massier stop, massier status" -ForegroundColor Cyan
