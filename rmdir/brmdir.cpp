#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

// brmdir, rmdir but better.
// forgot that windows had rmdir already, but its weird and bad and lame and complains when you try to delete a directory that isnt empty
// so I just added the unix rmdir option to ignore that and delete anyway
// Written by afstaff 8/1/2024




// Helper function to delete a file
bool deleteFile(const std::wstring& filePath) {
    return DeleteFile(filePath.c_str()) != 0;
}

// Helper function to delete a directory and its contents recursively
bool deleteDirectoryRecursively(const std::wstring& dirPath) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((dirPath + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error: Unable to access the directory. Error code: " << GetLastError() << L"\n";
        return false;
    }

    do {
        const std::wstring fileOrDir = findFileData.cFileName;

        if (fileOrDir != L"." && fileOrDir != L"..") {
            const std::wstring fullPath = dirPath + L"\\" + fileOrDir;

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // It's a directory, recursively delete its contents
                if (!deleteDirectoryRecursively(fullPath)) {
                    FindClose(hFind);
                    return false;
                }
            }
            else {
                // It's a file, delete it
                if (!deleteFile(fullPath)) {
                    std::wcerr << L"Error: Unable to delete file " << fullPath << L". Error code: " << GetLastError() << L"\n";
                    FindClose(hFind);
                    return false;
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return RemoveDirectory(dirPath.c_str()) != 0;
}

bool removeDirectory(const std::wstring& dirPath, bool ignoreFailOnNonEmpty) {
    if (ignoreFailOnNonEmpty) {
        if (deleteDirectoryRecursively(dirPath)) {
            return true;
        }
        else {
            std::wcerr << L"Error: Unable to remove the directory. It might be non-empty.\n";
            return false;
        }
    }
    else {
        // Try to remove the directory directly
        if (RemoveDirectory(dirPath.c_str())) {
            return true;
        }
        else {
            if (GetLastError() == ERROR_DIR_NOT_EMPTY) {
                std::wcerr << L"Error: Directory not empty. Use --ignore-fail-on-non-empty to force deletion.\n";
            }
            else {
                std::wcerr << L"Error: Unable to remove the directory. Error code: " << GetLastError() << L"\n";
            }
            return false;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <directory_path> [--ignore-fail-on-non-empty]\n";
        return 1;
    }

    std::wstring dirPath(argv[1], argv[1] + strlen(argv[1]));
    bool ignoreFailOnNonEmpty = (argc == 3 && std::string(argv[2]) == "--ignore-fail-on-non-empty");

    if (removeDirectory(dirPath, ignoreFailOnNonEmpty)) {
        std::wcout << L"Directory removed successfully.\n";
        return 0;
    }
    else {
        return 1;
    }
}
