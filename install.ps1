# Massier Installation Script
# This script compiles and installs Massier

Write-Host "Installing Massier..." -ForegroundColor Green

# Get the current directory
$installPath = $PSScriptRoot

# Check if executable already exists
if (Test-Path "$installPath\massier_87.exe") {
    Write-Host "massier_87.exe already exists, skipping compilation." -ForegroundColor Yellow
    $skipCompile = $true
} else {
    $skipCompile = $false
    
    # Check if C++ compiler (g++) is available
    Write-Host "Checking for C++ compiler..." -ForegroundColor Cyan

    try {
        $gppVersion = & g++ --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            $versionLine = ($gppVersion | Select-Object -First 1)
            Write-Host "Found C++ compiler: $versionLine" -ForegroundColor Green
            $compilerCommand = "g++"
        } else {
            throw "g++ not found"
        }
    } catch {
        Write-Host "`nError: C++ compiler (g++) not found!" -ForegroundColor Red
        Write-Host "`nTo install MinGW-w64 (includes g++):" -ForegroundColor Yellow
        Write-Host "1. Download from: https://winlibs.com/" -ForegroundColor White
        Write-Host "2. Download from: https://www.mingw-w64.org/downloads/" -ForegroundColor White
        Write-Host "3. Or install via package manager:" -ForegroundColor White
        Write-Host "   - Chocolatey: choco install mingw" -ForegroundColor Cyan
        Write-Host "   - Scoop: scoop install mingw" -ForegroundColor Cyan
        Write-Host "   - MSYS2: pacman -S mingw-w64-x86_64-gcc" -ForegroundColor Cyan
        Write-Host "`nAfter installation, add the compiler to your PATH and run this script again." -ForegroundColor Yellow
        exit 1
    }
}

# Check if source file exists
if (-not (Test-Path "$installPath\massier_87.cpp")) {
    Write-Host "Error: massier_87.cpp not found in $installPath" -ForegroundColor Red
    exit 1
}

# Compile if needed
if (-not $skipCompile) {
    # Compile the C++ source
    Write-Host "`nCompiling massier_87.cpp..." -ForegroundColor Cyan

    $compileArgs = @(
        "-fdiagnostics-color=always",
        "-g",
        "$installPath\massier_87.cpp",
        "-o",
        "$installPath\massier_87.exe"
    )

    try {
        & $compilerCommand $compileArgs
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Compilation successful!" -ForegroundColor Green
        } else {
            Write-Host "Compilation failed with exit code $LASTEXITCODE" -ForegroundColor Red
            exit 1
        }
    } catch {
        Write-Host "Compilation failed: $_" -ForegroundColor Red
        exit 1
    }

    # Check if massier_87.exe was created
    if (-not (Test-Path "$installPath\massier_87.exe")) {
        Write-Host "Error: Compilation did not produce massier_87.exe" -ForegroundColor Red
        exit 1
    }
}

# Get current user PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# Check if already in PATH
if ($currentPath -like "*$installPath*") {
    Write-Host "Massier directory is already in PATH." -ForegroundColor Yellow
} else {
    # Add to PATH
    $newPath = "$currentPath;$installPath"
    [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Host "Added $installPath to PATH." -ForegroundColor Green
}

# Create an alias batch file for easier command
$batchContent = "@echo off`r`n`"$installPath\massier_87.exe`" %*"
$batchPath = "$installPath\massier.bat"
Set-Content -Path $batchPath -Value $batchContent

Write-Host "`nInstallation complete!" -ForegroundColor Green
Write-Host "`nPlease restart your terminal or run:" -ForegroundColor Cyan
Write-Host "  `$env:Path = [System.Environment]::GetEnvironmentVariable('Path','User')" -ForegroundColor Yellow
Write-Host "`nThen you can use:" -ForegroundColor Cyan
Write-Host "  massier run" -ForegroundColor White
Write-Host "  massier stop" -ForegroundColor White
Write-Host "  massier status" -ForegroundColor White
Write-Host "  massier help" -ForegroundColor White
