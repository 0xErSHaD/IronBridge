#include "../include/common.h"

int wmain() {
    InitConsole();
    int choice = 0;

    while (1) {
        PrintMenu();
        wprintf(L"\nEnter option(1/2/3) : ");
        
        wscanf(L"%d", &choice);
        
        switch (choice) {
            case 1:
                HandleFileProcess();
                break;
            case 2:
                PrintHelp();
                break;
            case 3:
                PrintAbout();
                break;
            default:
                LogInfo(L"Invalid option, try again.");
        }
    }
    return 0;
}