#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <aclapi.h>
#include <sddl.h>
#include <iomanip>

// Barebones not-so-great version of ls, made for Windows.
// Supports the options I actually use, might add the other ones in the future.
// Written by afstaff 8/1/2024

// Function to set console text color
void SetConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Function to get Unix-like permissions string for a file
std::string GetUnixPermissions(DWORD attributes, const std::wstring& fileName) {
    std::string permissions = "---------";

    // Directories
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        permissions[0] = 'd';

    // Read-only files
    if (!(attributes & FILE_ATTRIBUTE_READONLY)) {
        permissions[1] = 'r'; // Owner read
        permissions[2] = 'w'; // Owner write

        permissions[4] = 'r'; // Group read
        permissions[5] = 'w'; // Group write

        permissions[7] = 'r'; // Others read
        permissions[8] = 'w'; // Others write
    }
    else {
        permissions[1] = 'r'; // Owner read

        permissions[4] = 'r'; // Group read

        permissions[7] = 'r'; // Others read
    }

    // Executable files (assuming executable if it has .exe, .com, .bat, .cmd)
    std::wstring extension = L"";
    size_t dotIndex = fileName.find_last_of(L".");
    if (dotIndex != std::wstring::npos) {
        extension = fileName.substr(dotIndex + 1);
    }

    if (extension == L"exe" || extension == L"com" || extension == L"bat" || extension == L"cmd") {
        permissions[3] = 'x'; // Owner execute
        permissions[6] = 'x'; // Group execute
        permissions[9] = 'x'; // Others execute
    }

    return permissions;
}

// Function to get owner and group of a file
std::pair<std::wstring, std::wstring> GetOwnerAndGroup(const std::wstring& filePath) {
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    std::wstring owner, group;

    if (GetNamedSecurityInfoW(filePath.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
        &pOwnerSid, &pGroupSid, NULL, NULL, &pSD) == ERROR_SUCCESS) {
        wchar_t ownerName[256], groupName[256];
        DWORD ownerNameSize = sizeof(ownerName) / sizeof(wchar_t);
        DWORD groupNameSize = sizeof(groupName) / sizeof(wchar_t);
        wchar_t domainName[256];
        DWORD domainNameSize = sizeof(domainName) / sizeof(wchar_t);
        SID_NAME_USE eUse;

        if (LookupAccountSidW(NULL, pOwnerSid, ownerName, &ownerNameSize, domainName, &domainNameSize, &eUse)) {
            owner = ownerName;
        }

        domainNameSize = sizeof(domainName) / sizeof(wchar_t);
        if (LookupAccountSidW(NULL, pGroupSid, groupName, &groupNameSize, domainName, &domainNameSize, &eUse)) {
            group = groupName;
        }

        LocalFree(pSD);
    }

    return { owner, group };
}

// Function to get human-readable file size
std::wstring GetHumanReadableSize(LONGLONG size) {
    const wchar_t* sizes[] = { L"B", L"K", L"M", L"G", L"T" };
    int order = 0;
    double doubleSize = static_cast<double>(size);
    while (doubleSize >= 1024.0 && order < _countof(sizes) - 1) {
        order++;
        doubleSize = doubleSize / 1024.0;
    }
    wchar_t buffer[20];
    swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%.1f%s", doubleSize, sizes[order]);
    return std::wstring(buffer);
}

// Function to get file size in bytes (or human-readable format)
std::wstring GetFileSizeString(const std::wstring& filePath, bool humanReadable) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        LARGE_INTEGER size;
        size.HighPart = fileInfo.nFileSizeHigh;
        size.LowPart = fileInfo.nFileSizeLow;
        if (humanReadable) {
            return GetHumanReadableSize(size.QuadPart);
        }
        else {
            return std::to_wstring(size.QuadPart);
        }
    }
    return L"0";
}

// Function to get last modified date
std::wstring GetLastModifiedDate(const std::wstring& filePath) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        FILETIME ft = fileInfo.ftLastWriteTime;
        SYSTEMTIME stUTC, stLocal;
        FileTimeToSystemTime(&ft, &stUTC);
        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

        wchar_t dateStr[20];
        swprintf(dateStr, sizeof(dateStr) / sizeof(wchar_t), L"%04d-%02d-%02d %02d:%02d",
            stLocal.wYear, stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute);
        return dateStr;
    }
    return L"";
}

// Function to get the number of hardlinks for a given file
DWORD GetHardLinkCount(const std::wstring& filePath) {
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    BY_HANDLE_FILE_INFORMATION fileInfo;
    if (GetFileInformationByHandle(hFile, &fileInfo)) {
        CloseHandle(hFile);
        return fileInfo.nNumberOfLinks;
    }

    CloseHandle(hFile);
    return 0;
}

bool DirectoryHasParent(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((path + L"\\..").c_str(), &findFileData);
    bool hasParent = (hFind != INVALID_HANDLE_VALUE && (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    FindClose(hFind);
    return hasParent;
}

void ListDirectory(
    const std::wstring& directoryPath,
    const std::wstring& displayPath,
    bool includeHidden,
    bool hideDotEntries,
    bool longListing,
    bool recursive,
    bool excludeOwner,
    bool excludeGroup,
    bool humanReadable,
    std::unordered_map<std::wstring, WORD>& fileColors,
    int consoleWidth)
{
    // Append "\\*" to the path to search for all files and folders
    std::wstring searchPath = directoryPath + L"\\*";

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to find files in the directory." << std::endl;
        return;
    }

    std::vector<std::wstring> filesAndFolders;
    std::vector<std::wstring> subDirectories;
    LONGLONG totalSize = 0;

    do {
        const std::wstring fileOrFolder = findFileData.cFileName;
        bool isHidden = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
        bool isDotFile = fileOrFolder[0] == L'.';

        // Determine inclusion based on flags
        if (includeHidden || (hideDotEntries && fileOrFolder != L"." && fileOrFolder != L"..") || (!isHidden && !isDotFile)) {
            filesAndFolders.push_back(fileOrFolder);

            // If directory, add to subdirectory list
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                subDirectories.push_back(fileOrFolder);
            }

            // Calculate total size
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                LARGE_INTEGER size;
                size.HighPart = findFileData.nFileSizeHigh;
                size.LowPart = findFileData.nFileSizeLow;
                totalSize += size.QuadPart;
            }
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);

    // Print total size in KB blocks or human-readable format
    if (longListing) {
        if (humanReadable) {
            std::wcout << L"total " << GetHumanReadableSize(totalSize) << std::endl;
        }
        else {
            std::wcout << L"total " << (totalSize + 1023) / 1024 << std::endl; // Calculate total size in 1KB blocks
        }
    }

    // Calculate maximum width of filenames
    size_t maxFilenameLength = 0;
    for (const auto& entry : filesAndFolders) {
        if (entry.length() > maxFilenameLength) {
            maxFilenameLength = entry.length();
        }
    }
    maxFilenameLength += 2; // Padding

    // Calculate number of columns
    int columns = consoleWidth / maxFilenameLength;
    if (columns < 1) columns = 1;

    // Print directory name with ":" format
    if (recursive) {
        std::wcout << displayPath << ":" << std::endl;
    }
    // Print files and folders based on the selected listing format
    if (longListing) {
        // Long listing format
        for (const auto& entry : filesAndFolders) {
            std::wstring fullPath = directoryPath + L"\\" + entry;
            DWORD fileAttributes = GetFileAttributesW(fullPath.c_str());

            std::string permissions = GetUnixPermissions(fileAttributes, entry);
            auto ownerAndGroup = GetOwnerAndGroup(fullPath);
            std::wstring fileSize = GetFileSizeString(fullPath, humanReadable);
            std::wstring lastModified = GetLastModifiedDate(fullPath);
            DWORD hardLinkCount = GetHardLinkCount(fullPath);

            if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                // Set color to blue for directories
                SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // White for non-filename part
                std::wcout << std::wstring(permissions.begin(), permissions.end()) << L" "
                    << hardLinkCount << L" ";
                if (!excludeOwner) {
                    std::wcout << ownerAndGroup.first << L" ";
                }
                if (!excludeGroup) {
                    std::wcout << ownerAndGroup.second << L" ";
                }
                std::wcout << fileSize << L" "
                    << lastModified << L" ";
                SetConsoleColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY); // Blue for directories
                std::wcout << entry << std::endl;
            }
            else {
                // Determine color based on file extension
                WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // Default to white
                size_t dotIndex = entry.find_last_of(L".");
                if (dotIndex != std::wstring::npos) {
                    std::wstring extension = entry.substr(dotIndex + 1);
                    auto it = fileColors.find(extension);
                    if (it != fileColors.end()) {
                        color = it->second;
                    }
                }

                SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // White for non-filename part
                std::wcout << std::wstring(permissions.begin(), permissions.end()) << L" "
                    << hardLinkCount << L" ";
                if (!excludeOwner) {
                    std::wcout << ownerAndGroup.first << L" ";
                }
                if (!excludeGroup) {
                    std::wcout << ownerAndGroup.second << L" ";
                }
                std::wcout << fileSize << L" "
                    << lastModified << L" ";
                SetConsoleColor(color); // Color for filenames based on file type
                std::wcout << entry << std::endl;
            }
        }
    }
    else {
        // Standard listing format
        for (size_t i = 0; i < filesAndFolders.size(); ++i) {
            if (i > 0 && i % columns == 0) {
                std::wcout << std::endl;
            }

            const auto& entry = filesAndFolders[i];
            std::wstring fullPath = directoryPath + L"\\" + entry;
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

            std::wcout << entry;
            std::wcout << std::wstring(maxFilenameLength - entry.length(), L' '); // Padding
        }

        std::wcout << std::endl;
    }

    // Reset color to white
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    // Recursively list subdirectories if -R is specified
    if (recursive) {
        for (const auto& subDir : subDirectories) {
            std::wstring newDirPath = directoryPath + L"\\" + subDir;
            std::wstring newDisplayPath = displayPath + L"/" + subDir;
            std::wcout << std::endl;
            ListDirectory(newDirPath, newDisplayPath, includeHidden, hideDotEntries, longListing, recursive, excludeOwner, excludeGroup, humanReadable, fileColors, consoleWidth);
        }
    }
}

int main(int argc, char* argv[]) {
    // Check for -a, -A, -l, -R, -g, -o, and -h options
    bool includeHidden = false;
    bool hideDotEntries = false;
    bool longListing = false;
    bool recursive = false;
    bool excludeOwner = false;
    bool excludeGroup = false;
    bool humanReadable = false;

    for (int i = 1; i < argc; ++i) {
        std::string option(argv[i]);
        if (option[0] == '-') {
            for (size_t j = 1; j < option.size(); ++j) {
                switch (option[j]) {
                case 'a':
                    includeHidden = true;
                    break;
                case 'A':
                    hideDotEntries = true;
                    break;
                case 'l':
                    longListing = true;
                    break;
                case 'R':
                    recursive = true;
                    break;
                case 'g':
                    excludeOwner = true;
                    break;
                case 'o':
                    excludeGroup = true;
                    break;
                case 'h':
                    humanReadable = true;
                    break;
                default:
                    std::cerr << "Unknown option: " << option[j] << std::endl;
                    return 1;
                }
            }
        }
    }

    // Get the current working directory
    wchar_t currentPath[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentPath);

    // Define file type extensions and their colors
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

    // Get console width
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int consoleWidth = csbi.dwSize.X;

    // Start listing from the current directory
    ListDirectory(currentPath, L".", includeHidden, hideDotEntries, longListing, recursive, excludeOwner, excludeGroup, humanReadable, fileColors, consoleWidth);

    return 0;
}