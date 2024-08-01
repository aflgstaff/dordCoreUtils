#include <iostream>
#include <windows.h>
// Simple program that clears the working terminal
// Written by afstaff 8/1/2024
int main()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    COORD coord = { 0, 0 };
    DWORD dwWritten;
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coord, &dwWritten);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coord, &dwWritten);
    SetConsoleCursorPosition(hConsole, coord);
}
