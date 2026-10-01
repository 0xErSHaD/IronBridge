#ifndef COMMON_H
#define COMMON_H

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <wchar.h>
#include <string.h>
#include <bcrypt.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

#define CHUNK_SIZE (10 * 1024 * 1024)

void InitConsole();
void PrintBanner();
void PrintMenu();
void PrintHelp();
void PrintAbout();
void LogInfo(const wchar_t* message);


void HandleFileProcess();


#ifndef BCRYPT_CHAIN_MODE_CTR
#define BCRYPT_CHAIN_MODE_CTR L"ChainingModeCTR"
#endif


#endif
