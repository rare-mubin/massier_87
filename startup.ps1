# Massier Startup Configuration Script
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("enable", "disable", "status")]
    [string]$Action
)

$startupFolder = [System.IO.Path]::Combine([Environment]::GetFolderPath("Startup"))
$shortcutPath = [System.IO.Path]::Combine($startupFolder, "Massier 87 - Window Manager.lnk")
$vbsPath = Join-Path $PSScriptRoot "massier_silent.vbs"
$exePath = Join-Path $PSScriptRoot "massier_87.exe"

function Enable-Startup {
    # Always update the VBScript with the correct path
    $scriptDir = $PSScriptRoot -replace '\\', '\\'
    $vbsContent = @"
Set WshShell = CreateObject("WScript.Shell")
WshShell.Run """$scriptDir\\massier_87.exe"" --daemon", 0, False
Set WshShell = Nothing
"@
    Set-Content -Path $vbsPath -Value $vbsContent
    
    # Check if task already exists
    $taskName = "Massier 87 - Window Manager"
    $existingTask = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    
    if ($existingTask) {
        Write-Host "Massier is already configured to run on startup." -ForegroundColor Yellow
        return
    }
    
    # Create a scheduled task to run at logon with highest privileges
    $action = New-ScheduledTaskAction -Execute "wscript.exe" -Argument """$vbsPath"""
    $trigger = New-ScheduledTaskTrigger -AtLogOn
    $principal = New-ScheduledTaskPrincipal -UserId "$env:USERDOMAIN\$env:USERNAME" -LogonType Interactive -RunLevel Highest
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable
    
    Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Description "Massier Window Manager - Auto Start" | Out-Null
    
    Write-Host "Massier will now run automatically on startup as administrator (no UAC prompt)." -ForegroundColor Green
}

function Disable-Startup {
    # Remove scheduled task
    $taskName = "Massier 87 - Window Manager"
    $existingTask = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    
    if ($existingTask) {
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false
        Write-Host "Massier scheduled task removed." -ForegroundColor Green
    }
    
    # Also remove old shortcut if it exists
    if (Test-Path $shortcutPath) {
        Remove-Item $shortcutPath -Force
        Write-Host "Massier shortcut removed." -ForegroundColor Green
    }
    
    # Also remove the VBScript if it exists
    if (Test-Path $vbsPath) {
        Remove-Item $vbsPath -Force
    }
    
    if (-not $existingTask -and -not (Test-Path $shortcutPath)) {
        Write-Host "Massier is not configured to run on startup." -ForegroundColor Yellow
    } else {
        Write-Host "Massier startup has been disabled." -ForegroundColor Green
    }
}

function Show-Status {
    $taskName = "Massier 87 - Window Manager"
    $existingTask = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    
    if ($existingTask) {
        Write-Host "Massier is configured to run on startup (via Task Scheduler)." -ForegroundColor Green
        Write-Host "Task Name: $taskName" -ForegroundColor Cyan
        Write-Host "Run Level: Highest (Administrator)" -ForegroundColor Cyan
    } elseif (Test-Path $shortcutPath) {
        Write-Host "Massier is configured to run on startup (via Startup folder)." -ForegroundColor Green
        Write-Host "Shortcut location: $shortcutPath" -ForegroundColor Cyan
    } else {
        Write-Host "Massier is NOT configured to run on startup." -ForegroundColor Yellow
    }
}

switch ($Action) {
    "enable" { Enable-Startup }
    "disable" { Disable-Startup }
    "status" { Show-Status }
}
