# Massier 87 - Window Manager

**Massier 87** is a lightweight window manager for Windows that allows you to move and resize any window using keyboard shortcuts, similar to Linux window managers.

## Features

- **Alt + Left Click + Drag** - Move any window
- **Alt + Right Click + Drag** - Resize any window from any corner/edge
- **Drag to top edge** - Maximize window automatically
- **Auto-restore maximized windows** - Automatically restores maximized windows when you start dragging
- **Transparent windows during operations** - Windows become semi-transparent while moving/resizing
- **Configurable transparency** - Adjust transparency level via command line
- **Background service** - Runs silently in the background
- **Startup configuration** - Optionally run on Windows startup with administrator privileges
- **No UAC prompts** - Uses Task Scheduler for elevated startup without UAC interruption

## Installation

### Prerequisites

You need a C++ compiler (g++) installed. If you don't have one, see [Installing a C++ Compiler](#installing-a-c-compiler) below.

### Quick Install

1. Clone or download this repository
2. Open PowerShell in the project directory
3. Run the install script:

```powershell
.\install.bat
```

The script will:
- Check for g++ compiler (or skip if `massier_87.exe` already exists)
- Compile the source code (if needed)
- Add Massier to your system PATH
- Create a command wrapper for easy access

4. Restart your terminal or refresh the PATH:


### Installing a C++ Compiler

If you don't have g++ installed, you have several options:

#### Option 1: WinLibs (Recommended)
1. Download from: https://winlibs.com/
2. Extract to a location (e.g., `C:\mingw64`)
3. Add `C:\mingw64\bin` to your system PATH

#### Option 2: MinGW-W64
1. Download from: https://www.mingw-w64.org/downloads/
2. Follow the installation wizard
3. Add the `bin` directory to your system PATH

#### Option 3: Package Manager

**Chocolatey:**
```powershell
choco install mingw
```

**Scoop:**
```powershell
scoop install mingw
```

**MSYS2:**
```bash
pacman -S mingw-w64-x86_64-gcc
```

## Usage

### Starting Massier

```powershell
massier run
```

This starts Massier as a background service.

### Stopping Massier

```powershell
massier stop
```

### Check Status

```powershell
massier status
```

### Configure Transparency

View current transparency (0=invisible, 255=opaque):
```powershell
massier transparency
```

Set transparency (default: 200, recommended: 100-200):
```powershell
massier transparency 150
```

After changing transparency, restart Massier for changes to take effect:
```powershell
massier stop
massier run
```

### Startup Configuration

Enable auto-start on Windows login:
```powershell
massier startup enable
```

Disable auto-start:
```powershell
massier startup disable
```

Check startup status:
```powershell
massier startup status
```

**Note:** The startup enable command requires administrator privileges and will prompt for UAC elevation automatically.

### View Help

```powershell
massier help
```

### Check Version

```powershell
massier version
```

## How It Works

Massier uses Windows low-level mouse hooks to intercept Alt+Click combinations globally across all windows. When you:

1. **Alt + Left Click or Q** - Massier calculates the offset from the mouse to the window corner and moves the window to follow your mouse
2. **Alt + Right Click** - Massier determines the closest corner/edge as an anchor point and resizes the window from that anchor
3. **Drag to top** - When you drag a window to within 5 pixels of the top screen edge and release, it automatically maximizes

The application runs as a background process with no visible window, consuming minimal system resources.

## Uninstallation

Run the uninstall script:

```powershell
.\uninstall.ps1
```
After uninstalling, you can manually delete the project folder if desired.

## Technical Details

- **Language:** C++ (Windows API)
- **Compiler:** MinGW-w64 (g++)
- **Architecture:** Low-level mouse hooks (WH_MOUSE_LL)
- **Startup Method:** Windows Task Scheduler (for elevated privileges without UAC)
- **Single Instance:** Uses mutex to prevent multiple instances

## Troubleshooting

### Massier not working with some windows
Some applications run with higher privileges. Run Massier with administrator privileges:
```powershell
massier startup enable
```
This configures Massier to run as administrator on startup.

### Compilation errors
Make sure g++ is in your PATH:
```powershell
g++ --version
```

## Version

Current version: **v1.1**

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**TL;DR:** You can use, modify, distribute, and even sell this software. Just keep the copyright notice.
# massier_87
