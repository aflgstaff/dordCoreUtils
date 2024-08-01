#include <iostream>
#include <windows.h>

// Simple pwd port to Windows.
// Written by afstaff 8/1/2024

int main() {
    char buffer[MAX_PATH];

    if (GetCurrentDirectoryA(MAX_PATH, buffer)) {
        std::cout << buffer << std::endl;
    }
    else {
        std::cerr << "Error getting current directory." << std::endl;
        return 1;
    }

    return 0;
}
