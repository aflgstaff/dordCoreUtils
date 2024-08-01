#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

void SetConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

bool HasExtension(const std::wstring& fileName, const std::vector<std::wstring>& extensions) {
    for (const auto& ext : extensions) {
        if (fileName.length() >= ext.length() && fileName.compare(fileName.length() - ext.length(), ext.length(), ext) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    // Check for -a option
    bool includeHidden = false;
    if (argc > 1) {
        std::string option(argv[1]);
        if (option == "-a") {
            includeHidden = true;
        }
    }

    // Get the current working directory
    wchar_t currentPath[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentPath);

    // Append "\\*" to the path to search for all files and folders
    std::wstring searchPath = std::wstring(currentPath) + L"\\*";

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to find files in the directory." << std::endl;
        return 1;
    }

    std::vector<std::wstring> filesAndFolders;

    do {
        const std::wstring fileOrFolder = findFileData.cFileName;
        bool isHidden = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
        bool isDotFile = fileOrFolder[0] == L'.';

        // Skip "." and ".." entries
        if (fileOrFolder != L"." && fileOrFolder != L"..") {
            // Include hidden files and dot files if -a option is supplied
            if (includeHidden || (!isHidden && !isDotFile)) {
                filesAndFolders.push_back(fileOrFolder);
            }
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);

    // Define file type extensions
    std::unordered_map<std::wstring, WORD> fileColors = {
        {L"exe", FOREGROUND_GREEN | FOREGROUND_INTENSITY},
        {L"com", FOREGROUND_GREEN | FOREGROUND_INTENSITY},
        {L"bat", FOREGROUND_GREEN | FOREGROUND_INTENSITY},
        {L"cmd", FOREGROUND_GREEN | FOREGROUND_INTENSITY},
        {L"zip", FOREGROUND_RED | FOREGROUND_INTENSITY},
        {L"rar", FOREGROUND_RED | FOREGROUND_INTENSITY},
        {L"7z",  FOREGROUND_RED | FOREGROUND_INTENSITY},
        {L"tar", FOREGROUND_RED | FOREGROUND_INTENSITY},
        {L"gz",  FOREGROUND_RED | FOREGROUND_INTENSITY},
        {L"jpg", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
        {L"jpeg",FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
        {L"png", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
        {L"bmp", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
        {L"gif", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
        {L"lnk", FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY},
        {L"url", FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY}
    };

    // Print all files and folders in one line
    for (const auto& entry : filesAndFolders) {
        std::wstring fullPath = std::wstring(currentPath) + L"\\" + entry;
        DWORD fileAttributes = GetFileAttributesW(fullPath.c_str());

        if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // Set color to blue for directories
            SetConsoleColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        }
        else {
            // Determine color based on file extension
            size_t dotIndex = entry.find_last_of(L".");
            if (dotIndex != std::wstring::npos) {
                std::wstring extension = entry.substr(dotIndex + 1);
                auto it = fileColors.find(extension);
                if (it != fileColors.end()) {
                    SetConsoleColor(it->second);
                }
                else {
                    // Set color to white for other files
                    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
            }
            else {
                // Set color to white for files without extension
                SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
        }

        std::wcout << entry << L" ";
    }

    // Reset color to white
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::wcout << std::endl;

    return 0;
}