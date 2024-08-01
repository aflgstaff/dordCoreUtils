#include <iostream>
#include <windows.h>
#include <Lmcons.h> // For UNLEN and USER_INFO_0

// Simple whoami port to Windows.
// Written by afstaff 8/1/2024

int main() {
    // Buffer to hold the user name
    char username[UNLEN + 1];
    DWORD size = sizeof(username);

    // Get the user name
    if (GetUserNameA(username, &size)) {
        std::cout << username << std::endl;
    }
    else {
        std::cerr << "Failed to get user name. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    return 0;
}
