#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <string>

// Barebones port of mv from UNIX into Windows.
// Written by afstaff 8/1/2024

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " source destination\n";
}

bool isDirectory(const char* path) {
    DWORD attribs = GetFileAttributesA(path);
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

std::string constructNewPath(const char* source, const char* destination) {
    std::string newPath(destination);
    if (newPath.back() != '\\' && newPath.back() != '/') {
        newPath += '\\';
    }

    char fileName[MAX_PATH];
    _splitpath_s(source, nullptr, 0, nullptr, 0, fileName, MAX_PATH, nullptr, 0);
    newPath += fileName;

    return newPath;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage(argv[0]);
        return 1;
    }

    const char* source = argv[1];
    const char* destination = argv[2];

    std::string finalDestination = destination;
    if (isDirectory(destination)) {
        finalDestination = constructNewPath(source, destination);
    }

    // Attempt to move the file
    if (!MoveFileA(source, finalDestination.c_str())) {
        DWORD error = GetLastError();
        LPVOID errorMessage;

        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            0,
            (LPSTR)&errorMessage,
            0,
            NULL
        );

        std::cerr << "Error: Failed to move " << source << " to " << finalDestination << "\n";
        std::cerr << "System Error Message: " << (char*)errorMessage << "\n";

        LocalFree(errorMessage);
        return 1;
    }

    return 0;
}