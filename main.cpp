#include <windows.h>
#include <fstream>
#include <string>
#include "dialogs.h"
#include "bank.h"

// Global logged user
std::string g_logged_user;

// Global bank instance
// Bank g_bank("C:\\Program Files (x86)\\FintechApp\\bank_data.json");


// Get path of the folder where EXE is installed
std::string GetAppFolder() {
    char path[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    // Remove the executable name
    size_t pos = fullPath.find_last_of("\\/");
    return (pos != std::string::npos) ? fullPath.substr(0, pos) : ".";
}

// Global bank instance
Bank g_bank(GetAppFolder());



// Debug log function
void DebugLog(const std::string& msg) {
    std::ofstream ofs("debug.txt", std::ios::app);
    if (ofs.is_open()) ofs << msg << std::endl;
}

// Entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        std::ofstream ofs("debug.txt", std::ios::trunc);
        ofs << "=== Application started at " << __TIME__ << " ===\n";

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
        }

    } catch (const std::exception& e) {
        DebugLog("CRITICAL ERROR in WinMain: " + std::string(e.what()));
        MessageBoxA(NULL, ("Startup failed: " + std::string(e.what())).c_str(), "Critical Error", MB_OK | MB_ICONERROR);
    } catch (...) {
        DebugLog("CRITICAL UNKNOWN ERROR in WinMain!");
        MessageBoxA(NULL, "Unknown startup error!", "Critical Error", MB_OK | MB_ICONERROR);
    }

    DebugLog("WinMain: Saving bank data...");
    g_bank.SaveToFileSync();
    DebugLog("WinMain: Exiting.");
    return 0;
}