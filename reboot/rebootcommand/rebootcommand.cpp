#include <windows.h>
#include <iostream>

// The program that started this project, a barebones reboot port from UNIX.
// Written by afstaff 8/1/2024

bool RestartComputer() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "Failed to open process token." << std::endl;
        return false;
    }
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0)) {
        std::cerr << "Failed to adjust token privileges." << std::endl;
        CloseHandle(hToken);
        return false;
    }
    if (GetLastError() != ERROR_SUCCESS) {
        std::cerr << "Error occurred while adjusting token privileges." << std::endl;
        CloseHandle(hToken);
        return false;
    }
    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER)) {
        std::cerr << "Failed to restart the computer." << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

int main() {
    if (RestartComputer()) {
        return 0;
    }
    else {
        std::cerr << "Failed to issue restart command." << std::endl;
    }
    return 0;
}
