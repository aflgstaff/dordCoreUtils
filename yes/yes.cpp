#include <iostream>
#include <string>
#include <chrono>

// A simple Windows port of UNIX's yes command.
// Written by afstaff 8/1/2024

void printRepeatedly(const std::string& str) {
    while (true) {
        std::cout << str << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string output = "y"; // Default output

    if (argc > 1) {
        output = argv[1]; // Use the first command-line argument if provided
    }

    printRepeatedly(output);

    return 0;
}
