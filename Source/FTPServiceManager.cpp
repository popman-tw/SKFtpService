#include "FTPServiceManager.h"
#include <process.h>
#include <stdio.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "ole32.lib")

FTPServiceManager::FTPServiceManager() : listenSocket(INVALID_SOCKET), isRunning(false) {
    ZeroMemory(&config, sizeof(FTPConfig));
    config.port = 21;
    config.maxConnections = 10;
    config.enableAnonymous = true;
    config.allowDownload = true;
    config.allowUpload = true;
    config.sharedFolder = "C:\\FTPShare";
    
    // 初始化 AD 配置
    config.adConfig.enabled = false;
    config.adConfig.port = 389;
    config.adConfig.timeout = 30;
    config.adConfig.server = "";
    config.adConfig.baseDN = "";
    config.adConfig.allowedGroups = "";
    config.adConfig.allowedUsers = "";
    
    InitializeCriticalSection(&csClientList);
}

FTPServiceManager::~FTPServiceManager() {
    if (isRunning) {
        Stop();
    }
    DeleteCriticalSection(&csClientList);
}

bool FTPServiceManager::Initialize() {
    // 載入配置檔案
    LoadConfig();
    
    // 載入 AD 配置
    LoadADConfig("config.ini");
    
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
    return true;
}

bool FTPServiceManager::SaveConfig() {
    // TODO: 實現將配置保存到 config.ini
    return true;
}

bool FTPServiceManager::LoadADConfig(const AnsiString& configFile) {
    // 簡單的 INI 檔案解析
    std::ifstream file(configFile.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    bool inADSection = false;
    
    while (std::getline(file, line)) {
        // 移除前後空白
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // 跳過空行和註解
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        
        // 檢查章節
        if (line[0] == '[' && line[line.length()-1] == ']') {
            inADSection = (line == "[AD]");
            continue;
        }
        
        if (inADSection) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // 移除空白
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                
                if (key == "Enabled") {
                    config.adConfig.enabled = (value == "1" || value == "true");
                } else if (key == "Server") {
                    config.adConfig.server = value.c_str();
                } else if (key == "Port") {
                    config.adConfig.port = atoi(value.c_str());
                } else if (key == "BaseDN") {
                    config.adConfig.baseDN = value.c_str();
                } else if (key == "AllowedGroups") {
                    config.adConfig.allowedGroups = value.c_str();
                } else if (key == "AllowedUsers") {
                    config.adConfig.allowedUsers = value.c_str();
                } else if (key == "Timeout") {
                    config.adConfig.timeout = atoi(value.c_str());
                }
            }
        }
    }
    
    file.close();
    return true;
}

bool FTPServiceManager::VerifyADCredentials(const AnsiString& username, const AnsiString& password) {
    if (!config.adConfig.enabled) {
        return false;
    }
    
    // 提取伺服器名稱和埠
    AnsiString server = config.adConfig.server;
    int port = config.adConfig.port;
    
    // 移除 "ldap://" 前綴
    if (server.Pos("ldap://") > 0) {
        server = server.Delete(1, 7);
    }
    
    // 提取伺服器名稱和埠
    int colonPos = server.Pos(":");
    if (colonPos > 0) {
        port = StrToInt(server.SubString(colonPos + 1, server.Length()));
        server = server.SubString(1, colonPos - 1);
    }
    
    // 將 username 轉換為適當的 DN 格式
    AnsiString userDN;
    int backslashPos = username.Pos("\\");
    int atPos = username.Pos("@");
    
    if (backslashPos > 0) {
        // Domain\\username 格式
        AnsiString domain = username.SubString(1, backslashPos - 1);
        AnsiString user = username.SubString(backslashPos + 1, username.Length());
        userDN = "cn=" + user + "," + config.adConfig.baseDN;
    } else if (atPos > 0) {
        // username@domain.com 格式
        AnsiString user = username.SubString(1, atPos - 1);
        userDN = "cn=" + user + "," + config.adConfig.baseDN;
    } else {
        // 只有 username
        userDN = "cn=" + username + "," + config.adConfig.baseDN;
    }
    
    // 使用 LDAP 連接和驗證
    LDAP* pLdapHandle = NULL;
    ULONG version = LDAP_VERSION3;
    
    // 連接到 LDAP 伺服器
    pLdapHandle = ldap_init((char*)server.c_str(), port);
    if (!pLdapHandle) {
        return false;
    }
    
    // 設定 LDAP 版本
    ldap_set_option(pLdapHandle, LDAP_OPT_PROTOCOL_VERSION, &version);
    
    // 設定連接逾時時間
    ULONG timeout = config.adConfig.timeout * 1000; // 轉換為毫秒
    ldap_set_option(pLdapHandle, LDAP_OPT_TIMELIMIT, &timeout);
    
    // 嘗試綁定（驗證）
    ULONG result = ldap_simple_bind_s(pLdapHandle, (char*)userDN.c_str(), (char*)password.c_str());
    
    if (result == LDAP_SUCCESS) {
        ldap_unbind_s(pLdapHandle);
        return true;
    } else {
        ldap_unbind_s(pLdapHandle);
        return false;
    }
}

bool FTPServiceManager::IsUserInAllowedList(const AnsiString& username) {
    if (!config.adConfig.enabled) {
        return true; // 如果未啟用 AD 認證，允許所有用戶
    }
    
    // 檢查允許的用戶列表
    if (!config.adConfig.allowedUsers.IsEmpty()) {
        AnsiString users = config.adConfig.allowedUsers;
        int pos = 1;
        while (pos <= users.Length()) {
            int semicolonPos = users.Pos(";", pos);
            AnsiString user;
            if (semicolonPos > 0) {
                user = users.SubString(pos, semicolonPos - pos);
                pos = semicolonPos + 1;
            } else {
                user = users.SubString(pos, users.Length());
                pos = users.Length() + 1;
            }
            user = user.Trim();
            if (user == username) {
                return true;
            }
        }
        return false; // 用戶不在允許列表中
    }
    
    // TODO: 實現群組檢查邏輯
    // 需要使用 LDAP 查詢用戶所屬的群組
    
    return true; // 如果沒有限制，允許用戶
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