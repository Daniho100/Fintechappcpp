// main.cpp
#include <windows.h>
#include <fstream>
#include <string>
#include "resource.h"
#include "dialogs.h"
#include "bank.h"

// Global bank instance
Bank g_bank;
std::string g_logged_user;

// Debug log function
void DebugLog(const std::string& msg) {
    std::ofstream ofs("debug.txt", std::ios::app);
    if (ofs.is_open()) {
        ofs << msg << std::endl;
    }
}

// Entry point
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Clear previous debug log
    std::ofstream ofs("debug.txt", std::ios::trunc);
    ofs << "=== Application started at " << __TIME__ << " ===\n";

    try {
        DebugLog("WinMain: Loading bank data...");
        g_bank.LoadFromFile();

        DebugLog("WinMain: Opening Login Dialog...");
        INT_PTR result = DialogBoxParamA(
            hInstance,
            MAKEINTRESOURCE(IDD_LOGIN_DIALOG),
            NULL,
            LoginDlgProc,
            0
        );

        if (result == IDOK) {
            DebugLog("WinMain: Login successful for '" + g_logged_user + "'. Opening Dashboard...");
            DialogBoxParamA(
                hInstance,
                MAKEINTRESOURCE(IDD_MAIN_WINDOW),
                NULL,
                MainWindowDlgProc,
                0
            );
        } else if (result == IDCANCEL) {
            DebugLog("WinMain: Login canceled.");
        } else {
            DebugLog("WinMain: Failed to create login dialog!");
            MessageBoxA(NULL, "Failed to open login dialog!", "Error", MB_OK | MB_ICONERROR);
        }

    } catch (const std::exception& e) {
        DebugLog("CRITICAL ERROR in WinMain: " + std::string(e.what()));
        MessageBoxA(NULL, ("Startup failed: " + std::string(e.what()) + "\n\nDelete bank_data.json and try again.").c_str(), 
                    "Critical Error", MB_OK | MB_ICONERROR);
    } catch (...) {
        DebugLog("CRITICAL UNKNOWN ERROR in WinMain!");
        MessageBoxA(NULL, "Unknown startup error!\n\nDelete bank_data.json and try again.", 
                    "Critical Error", MB_OK | MB_ICONERROR);
    }

    DebugLog("WinMain: Saving bank data...");
    g_bank.SaveToFileSync();
    DebugLog("WinMain: Application exiting.");
    return 0;
}






// If you want, next I can help you:

// Add transaction history UI (proper fintech style)

// Add audit logs (very important for fintech)

// Or make this production-safe (no data corruption, crash-safe writes)