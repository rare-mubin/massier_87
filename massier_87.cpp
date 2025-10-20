#include <windows.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>
#include <fstream>
#include <shlobj.h>

#define MUTEX_NAME "Global\\Massier87WindowManager"
#define PROCESS_NAME "massier_87.exe"
#define CONFIG_FILE "massier_config.ini"
#define DEFAULT_TRANSPARENCY 200
#define MASSIER_VERSION "v1.0"

// Global variables
bool altPressed = false;
bool dragging = false;
bool resizing = false;
HWND dragWindow = NULL;
HWND resizeWindow = NULL;
POINT dragOffset = {0, 0};
POINT dragStartMouse = {0, 0};  // Initial mouse position when drag started
POINT resizeStartMouse = {0, 0};  // Initial mouse position when resize started
RECT originalRect = {0, 0, 0, 0};
POINT resizeAnchor = {0, 0};  // The corner that stays fixed during resize
bool wasMaximized = false;  // Track if window was just restored from maximized
int transparencyLevel = DEFAULT_TRANSPARENCY;  // Transparency level (0-255)

// Low-level mouse hook procedure
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        
        // Alt + Left button down
        if (wParam == WM_LBUTTONDOWN && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            POINT pt = mouseStruct->pt;
            HWND hwnd = WindowFromPoint(pt);
            
            if (hwnd) {
                // Get the root window first
                HWND hwndTop = GetAncestor(hwnd, GA_ROOT);
                if (hwndTop) hwnd = hwndTop;
                
                // Check if window is maximized
                WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
                GetWindowPlacement(hwnd, &wp);
                if (wp.showCmd == SW_MAXIMIZE) {
                    // Save initial mouse position
                    dragStartMouse = pt;
                    
                    // Restore to normal mode
                    ShowWindow(hwnd, SW_RESTORE);
                    
                    // Get the restored window rect
                    RECT rect;
                    GetWindowRect(hwnd, &rect);
                    
                    // Calculate drag offset as if cursor was at the position where it is
                    // This prevents any jumping
                    dragOffset.x = pt.x - rect.left;
                    dragOffset.y = pt.y - rect.top;
                    
                    // Mark that we just restored from maximized
                    wasMaximized = true;
                } else {
                    // Normal window - calculate offset normally
                    RECT rect;
                    GetWindowRect(hwnd, &rect);
                    dragOffset.x = pt.x - rect.left;
                    dragOffset.y = pt.y - rect.top;
                    wasMaximized = false;
                }
                
                // Check if it's a VNC viewer
                char className[256];
                GetClassNameA(hwnd, className, 256);
                if (strstr(className, "vnc") || strstr(className, "VNC")) {
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }
                
                // Start dragging
                dragWindow = hwnd;
                dragging = true;
                
                // Focus and activate the window
                // DON'T simulate Alt key events - keep it simple
                DWORD currentThreadId = GetCurrentThreadId();
                DWORD windowThreadId = GetWindowThreadProcessId(hwnd, NULL);
                
                // Temporarily attach to window's thread to force focus
                BOOL attached = FALSE;
                if (windowThreadId != currentThreadId) {
                    attached = AttachThreadInput(currentThreadId, windowThreadId, TRUE);
                }
                
                BringWindowToTop(hwnd);
                SetForegroundWindow(hwnd);
                
                // Detach if we attached
                if (attached) {
                    AttachThreadInput(currentThreadId, windowThreadId, FALSE);
                }
                
                // Set window transparency
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
                SetLayeredWindowAttributes(hwnd, 0, transparencyLevel, LWA_ALPHA);
                
                return 1; // Block the click
            }
        }
        
        // Alt + Right button down - Start resizing
        if (wParam == WM_RBUTTONDOWN && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            POINT pt = mouseStruct->pt;
            HWND hwnd = WindowFromPoint(pt);
            
            if (hwnd) {
                // Get the root window first
                HWND hwndTop = GetAncestor(hwnd, GA_ROOT);
                if (hwndTop) hwnd = hwndTop;
                
                // Check if window is maximized
                WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
                GetWindowPlacement(hwnd, &wp);
                if (wp.showCmd == SW_MAXIMIZE) {
                    return 1; // Block the click
                }
                
                // Check if it's a VNC viewer
                char className[256];
                GetClassNameA(hwnd, className, 256);
                if (strstr(className, "vnc") || strstr(className, "VNC")) {
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }
                
                // Start resizing
                resizeWindow = hwnd;
                resizing = true;
                
                // Save initial mouse position
                resizeStartMouse = pt;
                
                // Get window rect
                GetWindowRect(hwnd, &originalRect);
                
                // Determine which corner to use as anchor (opposite to where clicked)
                int centerX = (originalRect.left + originalRect.right) / 2;
                int centerY = (originalRect.top + originalRect.bottom) / 2;
                
                if (pt.x < centerX) {
                    // Clicked on left side - anchor right edge
                    resizeAnchor.x = originalRect.right;
                } else {
                    // Clicked on right side - anchor left edge
                    resizeAnchor.x = originalRect.left;
                }
                
                if (pt.y < centerY) {
                    // Clicked on top side - anchor bottom edge
                    resizeAnchor.y = originalRect.bottom;
                } else {
                    // Clicked on bottom side - anchor top edge
                    resizeAnchor.y = originalRect.top;
                }
                
                // Focus the window
                BringWindowToTop(hwnd);
                SetForegroundWindow(hwnd);
                
                // Set window transparency
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
                SetLayeredWindowAttributes(hwnd, 0, transparencyLevel, LWA_ALPHA);
                
                return 1; // Block the click
            }
        }
        
        // Mouse move while dragging
        if (wParam == WM_MOUSEMOVE && dragging && dragWindow) {
            POINT pt = mouseStruct->pt;
            
            // Don't move the window if cursor is at the top (about to maximize)
            if (pt.y <= 5) {
                // Skip moving, just wait for mouse release to maximize
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }
            
            if (wasMaximized) {
                // For restored maximized windows, only start moving after some mouse movement
                int deltaX = pt.x - dragStartMouse.x;
                int deltaY = pt.y - dragStartMouse.y;
                
                // Check if mouse has moved enough to start dragging
                if (abs(deltaX) > 5 || abs(deltaY) > 5) {
                    // Now move the window
                    SetWindowPos(dragWindow, NULL, 
                                pt.x - dragOffset.x, 
                                pt.y - dragOffset.y,
                                0, 0, 
                                SWP_NOSIZE | SWP_NOZORDER);
                }
            } else {
                // Normal dragging
                SetWindowPos(dragWindow, NULL, 
                            pt.x - dragOffset.x, 
                            pt.y - dragOffset.y,
                            0, 0, 
                            SWP_NOSIZE | SWP_NOZORDER);
            }
        }
        
        // Block right-click context menu during resizing (but handle button up first)
        if (resizing && (wParam == WM_RBUTTONDOWN || wParam == WM_CONTEXTMENU)) {
            return 1; // Block right-click down and context menu during resize
        }
        
        // Right button up - must be handled BEFORE blocking
        if (wParam == WM_RBUTTONUP && resizing) {
            if (resizeWindow) {
                // Reset transparency
                SetLayeredWindowAttributes(resizeWindow, 0, 255, LWA_ALPHA);
                SetWindowLong(resizeWindow, GWL_EXSTYLE, 
                             GetWindowLong(resizeWindow, GWL_EXSTYLE) & ~WS_EX_LAYERED);
            }
            resizing = false;
            resizeWindow = NULL;
            return 1; // Block the button up from reaching the application
        }
        
        // Mouse move while resizing
        if (wParam == WM_MOUSEMOVE && resizing && resizeWindow) {
            POINT pt = mouseStruct->pt;
            
            // Calculate mouse delta from start position
            int deltaX = pt.x - resizeStartMouse.x;
            int deltaY = pt.y - resizeStartMouse.y;
            
            // Calculate new window rectangle based on anchor point and delta
            int newLeft = originalRect.left;
            int newTop = originalRect.top;
            int newRight = originalRect.right;
            int newBottom = originalRect.bottom;
            
            // Apply delta based on which edges are being moved
            if (resizeAnchor.x == originalRect.right) {
                // Left edge is moving
                newLeft = originalRect.left + deltaX;
            } else {
                // Right edge is moving
                newRight = originalRect.right + deltaX;
            }
            
            if (resizeAnchor.y == originalRect.bottom) {
                // Top edge is moving
                newTop = originalRect.top + deltaY;
            } else {
                // Bottom edge is moving
                newBottom = originalRect.bottom + deltaY;
            }
            
            int newWidth = newRight - newLeft;
            int newHeight = newBottom - newTop;
            
            // Enforce minimum size
            if (newWidth < 100) {
                if (resizeAnchor.x == originalRect.left) {
                    newRight = newLeft + 100;
                } else {
                    newLeft = newRight - 100;
                }
                newWidth = 100;
            }
            if (newHeight < 100) {
                if (resizeAnchor.y == originalRect.top) {
                    newBottom = newTop + 100;
                } else {
                    newTop = newBottom - 100;
                }
                newHeight = 100;
            }
            
            SetWindowPos(resizeWindow, NULL, newLeft, newTop, newWidth, newHeight, SWP_NOZORDER);
        }
        
        // Left button up
        if (wParam == WM_LBUTTONUP && dragging) {
            if (dragWindow) {
                // Check if window is at the top of the screen
                POINT pt = mouseStruct->pt;
                if (pt.y <= 5) {
                    // Maximize the window if dragged to top
                    ShowWindow(dragWindow, SW_MAXIMIZE);
                }
                
                // Reset transparency
                SetLayeredWindowAttributes(dragWindow, 0, 255, LWA_ALPHA);
                SetWindowLong(dragWindow, GWL_EXSTYLE, 
                             GetWindowLong(dragWindow, GWL_EXSTYLE) & ~WS_EX_LAYERED);
            }
            dragging = false;
            dragWindow = NULL;
            return 1; // Block the left button up to prevent window close
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Get config file path
std::string getConfigPath() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        path = path.substr(0, lastSlash + 1);
    }
    return path + CONFIG_FILE;
}

// Load transparency from config file
int loadTransparency() {
    std::string configPath = getConfigPath();
    std::ifstream file(configPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("transparency=") == 0) {
                int value = std::stoi(line.substr(13));
                file.close();
                if (value >= 0 && value <= 255) {
                    return value;
                }
            }
        }
        file.close();
    }
    return DEFAULT_TRANSPARENCY;
}

// Save transparency to config file
void saveTransparency(int value) {
    std::string configPath = getConfigPath();
    std::ofstream file(configPath);
    if (file.is_open()) {
        file << "transparency=" << value << std::endl;
        file.close();
    }
}

// Check if Massier is already running
bool isRunning() {
    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, MUTEX_NAME);
    if (hMutex) {
        CloseHandle(hMutex);
        return true;
    }
    return false;
}

// Get process ID of running Massier instance
DWORD getRunningPID() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, L"massier_87.exe") == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return 0;
}

// Stop running Massier instance
bool stopMassier() {
    DWORD pid = getRunningPID();
    if (pid == 0) {
        std::cout << "Massier is not running." << std::endl;
        return false;
    }
    
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        std::cout << "Failed to stop Massier (Access denied)." << std::endl;
        return false;
    }
    
    if (TerminateProcess(hProcess, 0)) {
        CloseHandle(hProcess);
        std::cout << "Massier stopped successfully." << std::endl;
        return true;
    } else {
        CloseHandle(hProcess);
        std::cout << "Failed to stop Massier." << std::endl;
        return false;
    }
}

// Show status of Massier
void showStatus() {
    if (isRunning()) {
        DWORD pid = getRunningPID();
        std::cout << "Massier is running (PID: " << pid << ")" << std::endl;
    } else {
        std::cout << "Massier is not running." << std::endl;
    }
}

// Show help message
void showHelp() {
    std::cout << "\nMassier 87 - Window Manager " << MASSIER_VERSION << "\n" << std::endl;
    std::cout << "Usage:---------------------------------------------------------------" << std::endl;
    std::cout << "  massier run              - Start the window manager" << std::endl;
    std::cout << "  massier stop             - Stop the window manager" << std::endl;
    std::cout << "  massier status           - Check if the window manager is running" << std::endl;
    std::cout << "  massier startup enable   - Configure Massier to run on startup" << std::endl;
    std::cout << "  massier startup disable  - Remove Massier from startup" << std::endl;
    std::cout << "  massier startup status   - Check startup configuration" << std::endl;
    std::cout << "  massier transparency <n> - Set transparency (0-255, default: 150)" << std::endl;
    std::cout << "  massier transparency     - Show current transparency" << std::endl;
    std::cout << "  massier version          - Show version information" << std::endl;
    std::cout << "  massier help             - Show this help message" << std::endl;
    std::cout << "\nFeatures:------------------------------------------------------------" << std::endl;
    std::cout << "  Alt+Left Click  - Move windows" << std::endl;
    std::cout << "  Alt+Right Click - Resize windows" << std::endl;
    std::cout << "  Drag to top     - Maximize window" << std::endl;
    std::cout << "\nNote: Transparency 0=invisible, 255=opaque\n" << std::endl;
}

// Handle startup configuration
int handleStartup(const char* action) {
    char scriptPath[MAX_PATH];
    char cmdLine[MAX_PATH * 2];
    
    // Get the directory where the executable is located
    GetModuleFileNameA(NULL, scriptPath, MAX_PATH);
    char* lastSlash = strrchr(scriptPath, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
    }
    strcat(scriptPath, "startup.ps1");
    
    // Build PowerShell command
    sprintf(cmdLine, "-ExecutionPolicy Bypass -File \"%s\" %s", scriptPath, action);
    
    // For "enable" action, request administrator elevation
    if (strcmp(action, "enable") == 0) {
        SHELLEXECUTEINFOA sei = {sizeof(sei)};
        sei.lpVerb = "runas";  // Request elevation
        sei.lpFile = "powershell.exe";
        sei.lpParameters = cmdLine;
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
        
        if (ShellExecuteExA(&sei)) {
            if (sei.hProcess) {
                WaitForSingleObject(sei.hProcess, INFINITE);
                DWORD exitCode;
                GetExitCodeProcess(sei.hProcess, &exitCode);
                CloseHandle(sei.hProcess);
                return exitCode;
            }
            return 0;
        } else {
            std::cout << "Failed to execute startup script with elevation." << std::endl;
            return 1;
        }
    } else {
        // For other actions (disable, status), run normally
        sprintf(cmdLine, "powershell.exe -ExecutionPolicy Bypass -File \"%s\" %s", scriptPath, action);
        
        STARTUPINFOA si = {sizeof(si)};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        PROCESS_INFORMATION pi;
        
        if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exitCode;
        } else {
            std::cout << "Failed to execute startup script." << std::endl;
            return 1;
        }
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc > 1) {
        std::string command = argv[1];
        
        if (command == "run") {
            if (isRunning()) {
                std::cout << "Massier is already running." << std::endl;
                return 1;
            }
            
            // Start as a detached background process
            STARTUPINFOA si = {sizeof(si)};
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            
            PROCESS_INFORMATION pi;
            
            char cmdLine[MAX_PATH];
            GetModuleFileNameA(NULL, cmdLine, MAX_PATH);
            strcat(cmdLine, " --daemon");
            
            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 
                              CREATE_NO_WINDOW | DETACHED_PROCESS, 
                              NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                std::cout << "Massier started in background." << std::endl;
                return 0;
            } else {
                std::cout << "Failed to start Massier." << std::endl;
                return 1;
            }
        } else if (command == "stop") {
            return stopMassier() ? 0 : 1;
        } else if (command == "status") {
            showStatus();
            return 0;
        } else if (command == "startup") {
            // Handle startup subcommands
            if (argc > 2) {
                std::string subcommand = argv[2];
                if (subcommand == "enable") {
                    return handleStartup("enable");
                } else if (subcommand == "disable") {
                    return handleStartup("disable");
                } else if (subcommand == "status") {
                    return handleStartup("status");
                } else {
                    std::cout << "Unknown startup command: " << subcommand << std::endl;
                    std::cout << "Use: massier startup [enable|disable|status]" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Usage: massier startup [enable|disable|status]" << std::endl;
                return 1;
            }
        } else if (command == "transparency") {
            // Handle transparency command
            if (argc > 2) {
                // Set transparency
                int value = std::atoi(argv[2]);
                if (value < 0 || value > 255) {
                    std::cout << "Invalid transparency value. Must be between 0-255." << std::endl;
                    std::cout << "0 = invisible, 255 = opaque, recommended: 100-200" << std::endl;
                    return 1;
                }
                saveTransparency(value);
                std::cout << "Transparency set to " << value << std::endl;
                std::cout << "Restart Massier for changes to take effect: massier stop && massier run" << std::endl;
                return 0;
            } else {
                // Show current transparency
                int current = loadTransparency();
                std::cout << "Current transparency: " << current << " (0=invisible, 255=opaque)" << std::endl;
                return 0;
            }
        } else if (command == "help" || command == "--help" || command == "-h") {
            showHelp();
            return 0;
        } else if (command == "version" || command == "--version" || command == "-v") {
            std::cout << "Massier 87 - Window Manager " << MASSIER_VERSION << std::endl;
            return 0;
        } else if (command == "--daemon") {
            // This is the actual background process - continue to main loop
        } else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Use 'massier help' for usage information." << std::endl;
            return 1;
        }
    } else {
        // No arguments - show help
        showHelp();
        return 0;
    }
    
    // Create mutex to prevent multiple instances
    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Massier is already running." << std::endl;
        return 1;
    }
    
    // Load transparency setting
    transparencyLevel = loadTransparency();
    
    // Set console window title so it shows properly in taskbar
    SetConsoleTitleA("Massier 87 - Window Manager");
    
    // Install mouse hook
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    
    if (!mouseHook) {
        MessageBoxA(NULL, "Failed to install hook!", "Error", MB_OK);
        return 1;
    }
    
    // No console output - running silently in background
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    UnhookWindowsHookEx(mouseHook);
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}
