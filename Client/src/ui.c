#include "../include/common.h"

void InitConsole() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

void PrintBanner() {
    wprintf(L"\033[1;36m========================================\033[0m\n");
    wprintf(L"\033[1;36m      IRON BRIDGE - Security Tool       \033[0m\n");
    wprintf(L"\033[1;36m========================================\033[0m\n");
}

void PrintMenu() {
    PrintBanner();
    wprintf(L"\033[1;32m[+]\033[0m Enter path (directory/file) (1)\n");
    wprintf(L"\033[1;32m[+]\033[0m Help (2)\n");
    wprintf(L"\033[1;32m[+]\033[0m About (3)\n");
}

void PrintHelp() {
    wprintf(L"\n--- Help ---\nIronBridge is an advanced file security tool utilizing Windows API Memory Mapping.\n\n");
}

void PrintAbout() {
    wprintf(L"\n--- About ---\nIronBridge CLI v1.0\n\n");
}

void LogInfo(const wchar_t* message) {
    wprintf(L"\033[1;33m[*]\033[0m %ls\n", message);
}