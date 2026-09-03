#include "ClientHandler.h"
#include "FTPServiceManager.h"
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

ClientHandler::ClientHandler(SOCKET clientSocket, const AnsiString& clientIP, int clientPort, FTPServiceManager* manager)
    : allowUpload(true), allowDownload(true), serviceManager(manager) {
    session.socket = clientSocket;
    session.clientIP = clientIP;
    session.clientPort = clientPort;
    session.isAuthenticated = false;
    session.username = "";
    session.currentDirectory = "\\";
}

ClientHandler::~ClientHandler() {
    if (session.socket != INVALID_SOCKET) {
        closesocket(session.socket);
    }
}

void ClientHandler::HandleClient() {
    // 發送歡迎消息
    SendResponse(220, "Welcome to SKFtpService");
    
    char buffer[1024];
    int recvResult;
    
    // 接收並處理 FTP 命令
    while ((recvResult = recv(session.socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[recvResult] = '\0';
        
        // 移除回車換行符
        AnsiString command = buffer;
        int pos = command.Pos("\r\n");
        if (pos > 0) {
            command = command.SubString(1, pos - 1);
        }
        
        // 解析命令
        pos = command.Pos(" ");
        AnsiString cmd, param;
        if (pos > 0) {
            cmd = command.SubString(1, pos - 1).UpperCase();
            param = command.SubString(pos + 1, command.Length());
        } else {
            cmd = command.UpperCase();
            param = "";
        }
        
        // 執行對應的命令
        if (cmd == "USER") HandleUSER(param);
        else if (cmd == "PASS") HandlePASS(param);
        else if (cmd == "QUIT") { HandleQUIT(param); break; }
        else if (cmd == "LIST") HandleLIST(param);
        else if (cmd == "CWD") HandleCWD(param);
        else if (cmd == "PWD") HandlePWD(param);
        else if (cmd == "TYPE") HandleTYPE(param);
        else if (cmd == "RETR") HandleRETR(param);
        else if (cmd == "STOR") HandleSTOR(param);
        else if (cmd == "DELE") HandleDELE(param);
        else if (cmd == "MKD") HandleMKD(param);
        else if (cmd == "RMD") HandleRMD(param);
        else if (cmd == "PASV") HandlePASV(param);
        else if (cmd == "PORT") HandlePORT(param);
        else {
            SendResponse(500, "Unknown command");
        }
    }
}

void ClientHandler::HandleUSER(const AnsiString& param) {
    session.username = param;
    SendResponse(331, "User name ok, need password");
}

void ClientHandler::HandlePASS(const AnsiString& param) {
    // 使用 AD 認證或本地認證
    if (AuthenticateUser(session.username, param)) {
        session.isAuthenticated = true;
        SendResponse(230, "User logged in, proceed");
    } else {
        session.isAuthenticated = false;
        SendResponse(530, "Authentication failed");
    }
}

void ClientHandler::HandleQUIT(const AnsiString& param) {
    SendResponse(221, "Goodbye");
}

void ClientHandler::HandleLIST(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    SendResponse(150, "Opening ASCII mode data connection for file list");
    // TODO: 實現目錄列表
    SendResponse(226, "Transfer complete");
}

void ClientHandler::HandleCWD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    AnsiString newDir = NormalizePath(param);
    if (ValidatePath(newDir)) {
        session.currentDirectory = newDir;
        SendResponse(250, "CWD command successful");
    } else {
        SendResponse(550, "Requested action not taken");
    }
}

void ClientHandler::HandlePWD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    AnsiString response = AnsiString().sprintf("257 \"%s\" is current directory", session.currentDirectory.c_str());
    SendResponse(257, response);
}

void ClientHandler::HandleTYPE(const AnsiString& param) {
    if (param.UpperCase() == "A" || param.UpperCase() == "I") {
        SendResponse(200, "Type set to");
    } else {
        SendResponse(500, "Unknown type");
    }
}

void ClientHandler::HandleRETR(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (!allowDownload) {
        SendResponse(550, "Download not allowed");
        return;
    }
    
    // TODO: 實現文件下載
    SendResponse(150, "Opening BINARY mode data connection");
    SendResponse(226, "Transfer complete");
}

void ClientHandler::HandleSTOR(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (!allowUpload) {
        SendResponse(550, "Upload not allowed");
        return;
    }
    
    // TODO: 實現文件上傳
    SendResponse(150, "Opening BINARY mode data connection");
    SendResponse(226, "Transfer complete");
}

void ClientHandler::HandleDELE(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    // TODO: 實現文件刪除
    SendResponse(250, "Requested file action okay");
}

void ClientHandler::HandleMKD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    // TODO: 實現目錄建立
    SendResponse(257, "Pathname created");
}

void ClientHandler::HandleRMD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    // TODO: 實現目錄刪除
    SendResponse(250, "Requested file action okay");
}

void ClientHandler::HandlePASV(const AnsiString& param) {
    // TODO: 實現被動模式
    SendResponse(227, "Entering Passive Mode");
}

void ClientHandler::HandlePORT(const AnsiString& param) {
    // TODO: 實現主動模式
    SendResponse(200, "PORT command successful");
}

void ClientHandler::SendResponse(int code, const AnsiString& message) {
    AnsiString response = AnsiString().sprintf("%d %s\r\n", code, message.c_str());
    send(session.socket, response.c_str(), response.Length(), 0);
}

bool ClientHandler::ValidatePath(const AnsiString& path) {
    // 驗證路徑是否在共享目錄內
    AnsiString fullPath = sharedFolder + path;
    // TODO: 實現路徑驗證邏輯
    return true;
}

AnsiString ClientHandler::NormalizePath(const AnsiString& path) {
    // 規範化路徑
    // TODO: 實現路徑規範化邏輯
    return path;
}

bool ClientHandler::AuthenticateUser(const AnsiString& username, const AnsiString& password) {
    if (!serviceManager) {
        return false;
    }
    
    // 檢查 AD 認證是否啟用
    if (serviceManager->GetADConfig().enabled) {
        // 使用 AD 驗證
        if (!serviceManager->VerifyADCredentials(username, password)) {
            return false;
        }
        
        // 檢查用戶是否在允許列表中
        if (!serviceManager->IsUserInAllowedList(username)) {
            return false;
        }
        
        return true;
    } else {
        // 簡單驗證 - 在實際應用中應實現真實的用戶驗證機制
        return true;
    }
}