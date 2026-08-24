#include "FTPServer.h"
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

FTPServer::FTPServer() : isRunning(false), serverThread(NULL) {
    serviceManager = new FTPServiceManager();
    systemTray = new SystemTray();
}

FTPServer::~FTPServer() {
    if (isRunning) {
        StopService();
    }
    if (serviceManager) delete serviceManager;
    if (systemTray) delete systemTray;
}

bool FTPServer::Initialize() {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    
    // 初始化服務管理器
    if (!serviceManager->Initialize()) {
        WSACleanup();
        return false;
    }
    
    // 初始化系統托盤
    if (!systemTray->Initialize()) {
        WSACleanup();
        return false;
    }
    
    return true;
}

bool FTPServer::StartService() {
    if (isRunning) {
        return false; // 服務已在運行
    }
    
    if (!serviceManager->Start()) {
        return false;
    }
    
    isRunning = true;
    if (systemTray) {
        systemTray->SetServiceStatus(true);
    }
    
    return true;
}

bool FTPServer::StopService() {
    if (!isRunning) {
        return false; // 服務未運行
    }
    
    if (!serviceManager->Stop()) {
        return false;
    }
    
    isRunning = false;
    if (systemTray) {
        systemTray->SetServiceStatus(false);
    }
    
    return true;
}

void FTPServer::SetPort(int port) {
    if (serviceManager) {
        serviceManager->SetPort(port);
    }
}

void FTPServer::SetSharedFolder(const AnsiString& folder) {
    if (serviceManager) {
        serviceManager->SetSharedFolder(folder);
    }
}