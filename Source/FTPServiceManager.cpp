#include "FTPServiceManager.h"
#include <process.h>
#include <stdio.h>

FTPServiceManager::FTPServiceManager() : listenSocket(INVALID_SOCKET), isRunning(false) {
    ZeroMemory(&config, sizeof(FTPConfig));
    config.port = 21;
    config.maxConnections = 10;
    config.enableAnonymous = true;
    config.allowDownload = true;
    config.allowUpload = true;
    config.sharedFolder = "C:\\FTPShare";
    
    InitializeCriticalSection(&csClientList);
}

FTPServiceManager::~FTPServiceManager() {
    if (isRunning) {
        Stop();
    }
    DeleteCriticalSection(&csClientList);
}

bool FTPServiceManager::Initialize() {
    // 加載配置檔案
    LoadConfig();
    
    // 建立共享目錄（如果不存在）
    if (!DirectoryExists(config.sharedFolder)) {
        CreateDir(config.sharedFolder);
    }
    
    return true;
}

bool FTPServiceManager::Start() {
    if (isRunning) {
        return false; // 已在運行
    }
    
    // 建立監聽 socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        return false;
    }
    
    // 綁定地址和埠號
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(config.port);
    
    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
        return false;
    }
    
    // 開始監聽
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
        return false;
    }
    
    isRunning = true;
    
    // 啟動接受連接的執行緒
    unsigned int threadID;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, AcceptClientThread, this, 0, &threadID);
    if (hThread) {
        CloseHandle(hThread);
    }
    
    // 保存配置
    SaveConfig();
    
    return true;
}

bool FTPServiceManager::Stop() {
    if (!isRunning) {
        return false; // 未運行
    }
    
    isRunning = false;
    
    // 關閉監聽 socket
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }
    
    // 關閉所有用戶端連接
    EnterCriticalSection(&csClientList);
    for (size_t i = 0; i < clientSockets.size(); i++) {
        closesocket(clientSockets[i]);
    }
    clientSockets.clear();
    LeaveCriticalSection(&csClientList);
    
    return true;
}

bool FTPServiceManager::LoadConfig() {
    // TODO: 實現從 config.ini 讀取配置
    // 這裡可以使用 Windows API 或第三方庫讀取 INI 檔案
    return true;
}

bool FTPServiceManager::SaveConfig() {
    // TODO: 實現將配置保存到 config.ini
    // 這裡可以使用 Windows API 或第三方庫寫入 INI 檔案
    return true;
}

unsigned int __stdcall FTPServiceManager::AcceptClientThread(void* param) {
    FTPServiceManager* pThis = (FTPServiceManager*)param;
    
    while (pThis->isRunning) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(pThis->listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        
        // 檢查連接數量限制
        EnterCriticalSection(&pThis->csClientList);
        if (pThis->clientSockets.size() >= (size_t)pThis->config.maxConnections) {
            LeaveCriticalSection(&pThis->csClientList);
            closesocket(clientSocket);
            continue;
        }
        pThis->clientSockets.push_back(clientSocket);
        LeaveCriticalSection(&pThis->csClientList);
        
        // 為每個用戶端建立處理執行緒
        unsigned int threadID;
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClientThread, (void*)clientSocket, 0, &threadID);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    
    return 0;
}

unsigned int __stdcall FTPServiceManager::HandleClientThread(void* param) {
    SOCKET clientSocket = (SOCKET)param;
    
    // TODO: 實現 FTP 協議處理
    char buffer[1024];
    int recvResult;
    
    // 發送歡迎信息
    const char* welcomeMsg = "220 FTP Server Ready\r\n";
    send(clientSocket, welcomeMsg, strlen(welcomeMsg), 0);
    
    // 接收並處理命令
    while ((recvResult = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[recvResult] = '\0';
        // TODO: 解析 FTP 命令並執行相應操作
    }
    
    closesocket(clientSocket);
    return 0;
}