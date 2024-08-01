#include <iostream>
#include <fstream>
#include <string>

// Very simple cat command, doesnt have any options like the actual cat but I never use them anyway.
// Written by afstaff 8/1/2024

void printFileContents(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file1> [file2 ...]" << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        printFileContents(argv[i]);
    }

    return 0;
}
