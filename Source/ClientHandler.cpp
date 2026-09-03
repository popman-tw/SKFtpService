#include "ClientHandler.h"
#include "FTPServiceManager.h"
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>
#include <windows.h>
#include <sstream>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")

ClientHandler::ClientHandler(SOCKET clientSocket, const AnsiString& clientIP, int clientPort, FTPServiceManager* manager)
    : allowUpload(true), allowDownload(true), serviceManager(manager) {
    session.socket = clientSocket;
    session.clientIP = clientIP;
    session.clientPort = clientPort;
    session.isAuthenticated = false;
    session.username = "";
    session.currentDirectory = "/";
    session.dataSocket = INVALID_SOCKET;
    session.binaryMode = false; // 預設 ASCII 模式
    session.restPosition = 0;
    
    pasvInfo.listenSocket = INVALID_SOCKET;
    pasvInfo.port = 0;
    pasvInfo.active = false;
    
    portInfo.active = false;
    portInfo.port = 0;
    portInfo.hostIP = "";
    
    ZeroMemory(&transferInfo, sizeof(FileTransferInfo));
    transferInfo.fileHandle = INVALID_HANDLE_VALUE;
}

ClientHandler::~ClientHandler() {
    CloseDataConnection();
    if (session.socket != INVALID_SOCKET) {
        closesocket(session.socket);
    }
}

void ClientHandler::HandleClient() {
    try {
        // 發送歡迎信息
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
                param = command.SubString(pos + 1, command.Length()).Trim();
            } else {
                cmd = command.UpperCase();
                param = "";
            }
            
            // 執行對應的命令
            if (cmd == "USER") HandleUSER(param);
            else if (cmd == "PASS") HandlePASS(param);
            else if (cmd == "QUIT") { HandleQUIT(param); break; }
            else if (cmd == "LIST") HandleLIST(param);
            else if (cmd == "NLST") HandleNLST(param);
            else if (cmd == "CWD") HandleCWD(param);
            else if (cmd == "PWD") HandlePWD(param);
            else if (cmd == "TYPE") HandleTYPE(param);
            else if (cmd == "RETR") HandleRETR(param);
            else if (cmd == "STOR") HandleSTOR(param);
            else if (cmd == "APPE") HandleAPPE(param);
            else if (cmd == "DELE") HandleDELE(param);
            else if (cmd == "MKD") HandleMKD(param);
            else if (cmd == "RMD") HandleRMD(param);
            else if (cmd == "PASV") HandlePASV(param);
            else if (cmd == "PORT") HandlePORT(param);
            else if (cmd == "REST") HandleREST(param);
            else if (cmd == "SIZE") HandleSIZE(param);
            else if (cmd == "MDTM") HandleMDTM(param);
            else if (cmd == "ABOR") HandleABOR(param);
            else if (cmd == "RNFR") HandleRNFR(param);
            else if (cmd == "RNTO") HandleRNTO(param);
            else if (cmd == "SYST") HandleSYST(param);
            else if (cmd == "NOOP") HandleNOOP(param);
            else {
                SendResponse(500, "Unknown command");
            }
        }
    } catch (...) {
        // 捕獲任何異常
    }
}

void ClientHandler::HandleUSER(const AnsiString& param) {
    if (param.IsEmpty()) {
        SendResponse(501, "No username given");
        return;
    }
    session.username = param;
    SendResponse(331, "User name ok, need password");
}

void ClientHandler::HandlePASS(const AnsiString& param) {
    if (session.username.IsEmpty()) {
        SendResponse(503, "Login with USER first");
        return;
    }
    
    if (AuthenticateUser(session.username, param)) {
        session.isAuthenticated = true;
        SendResponse(230, "User logged in");
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
    
    AnsiString path = param.IsEmpty() ? session.currentDirectory : param;
    path = NormalizePath(path);
    
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    SendResponse(150, "Opening ASCII mode data connection for file list");
    if (SendFileList(path, false)) {
        SendResponse(226, "Transfer complete");
    } else {
        SendResponse(550, "Failed to open directory");
    }
}

void ClientHandler::HandleNLST(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    AnsiString path = param.IsEmpty() ? session.currentDirectory : param;
    path = NormalizePath(path);
    
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    SendResponse(150, "Opening ASCII mode data connection for file list");
    if (SendFileList(path, true)) {
        SendResponse(226, "Transfer complete");
    } else {
        SendResponse(550, "Failed to open directory");
    }
}

void ClientHandler::HandleCWD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No path given");
        return;
    }
    
    AnsiString newDir = NormalizePath(param);
    if (IsPathTraversal(newDir)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fullPath = GetAbsolutePath(newDir);
    
    // 檢查目錄是否存在
    DWORD attrib = GetFileAttributes(fullPath.c_str());
    if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        SendResponse(550, "Cannot change directory");
        return;
    }
    
    session.currentDirectory = newDir;
    SendResponse(250, "CWD command successful");
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
    if (param.IsEmpty()) {
        SendResponse(501, "No type given");
        return;
    }
    
    AnsiString type = param.SubString(1, 1).UpperCase();
    if (type == "A") {
        session.binaryMode = false;
        SendResponse(200, "Type set to ASCII");
    } else if (type == "I") {
        session.binaryMode = true;
        SendResponse(200, "Type set to BINARY");
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
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    SendResponse(150, "Opening BINARY mode data connection");
    if (SendFile(path)) {
        SendResponse(226, "Transfer complete");
    } else {
        SendResponse(550, "Failed to open file for reading");
    }
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
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    SendResponse(150, "Opening BINARY mode data connection");
    if (ReceiveFile(path, false)) {
        SendResponse(226, "Transfer complete");
    } else {
        SendResponse(550, "Failed to write file");
    }
}

void ClientHandler::HandleAPPE(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (!allowUpload) {
        SendResponse(550, "Upload not allowed");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    SendResponse(150, "Opening BINARY mode data connection");
    if (ReceiveFile(path, true)) {
        SendResponse(226, "Transfer complete");
    } else {
        SendResponse(550, "Failed to append file");
    }
}

void ClientHandler::HandleDELE(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (!allowUpload) {
        SendResponse(550, "Delete not allowed");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fullPath = GetAbsolutePath(path);
    
    if (DeleteFile(fullPath.c_str())) {
        SendResponse(250, "Requested file action okay");
    } else {
        SendResponse(550, "Cannot delete file");
    }
}

void ClientHandler::HandleMKD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (!allowUpload) {
        SendResponse(550, "Create directory not allowed");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No pathname given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fullPath = GetAbsolutePath(path);
    
    if (CreateDirectory(fullPath.c_str(), NULL)) {
        SendResponse(257, "Pathname created");
    } else {
        SendResponse(550, "Cannot create directory");
    }
}

void ClientHandler::HandleRMD(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (!allowUpload) {
        SendResponse(550, "Remove directory not allowed");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No pathname given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fullPath = GetAbsolutePath(path);
    
    if (RemoveDirectory(fullPath.c_str())) {
        SendResponse(250, "Directory deleted");
    } else {
        SendResponse(550, "Cannot remove directory");
    }
}

void ClientHandler::HandlePASV(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    SOCKET dataSocket = EstablishPASVConnection();
    if (dataSocket != INVALID_SOCKET) {
        session.dataSocket = dataSocket;
        pasvInfo.active = true;
        SendResponse(227, AnsiString().sprintf("Entering Passive Mode (%d,%d)", pasvInfo.port / 256, pasvInfo.port % 256));
    } else {
        SendResponse(550, "Cannot open passive mode");
    }
}

void ClientHandler::HandlePORT(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    // 解析 PORT 命令格式: PORT h1,h2,h3,h4,p1,p2
    // h1.h2.h3.h4:p1*256+p2
    
    portInfo.hostIP = param.SubString(1, param.Pos(",") - 1);
    AnsiString portStr = param.SubString(param.LastDelimiter(",") + 1, param.Length());
    portInfo.port = StrToInt(portStr);
    portInfo.active = true;
    
    SendResponse(200, "PORT command successful");
}

void ClientHandler::HandleREST(const AnsiString& param) {
    if (param.IsEmpty()) {
        SendResponse(501, "Syntax error");
        return;
    }
    
    try {
        session.restPosition = StrToInt64(param);
        SendResponse(350, "Restart position accepted");
    } catch (...) {
        SendResponse(501, "Invalid position");
    }
}

void ClientHandler::HandleSIZE(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fullPath = GetAbsolutePath(path);
    
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(fullPath.c_str(), GetFileExInfoStandard, &fad)) {
        __int64 fileSize = ((__int64)fad.nFileSizeHigh << 32) + fad.nFileSizeLow;
        SendResponse(213, AnsiString().sprintf("%I64d", fileSize));
    } else {
        SendResponse(550, "File not found");
    }
}

void ClientHandler::HandleMDTM(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fullPath = GetAbsolutePath(path);
    
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(fullPath.c_str(), GetFileExInfoStandard, &fad)) {
        SendResponse(213, ConvertTimeToFTPFormat(fad.ftLastWriteTime));
    } else {
        SendResponse(550, "File not found");
    }
}

void ClientHandler::HandleABOR(const AnsiString& param) {
    CloseDataConnection();
    SendResponse(226, "Abort successful");
}

void ClientHandler::HandleRNFR(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    AnsiString path = NormalizePath(param);
    if (IsPathTraversal(path)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    renameFromPath = path;
    SendResponse(350, "File exists, ready for destination name");
}

void ClientHandler::HandleRNTO(const AnsiString& param) {
    if (!session.isAuthenticated) {
        SendResponse(530, "Not logged in");
        return;
    }
    
    if (param.IsEmpty()) {
        SendResponse(501, "No filename given");
        return;
    }
    
    if (renameFromPath.IsEmpty()) {
        SendResponse(503, "Bad sequence of commands");
        return;
    }
    
    AnsiString toPath = NormalizePath(param);
    if (IsPathTraversal(toPath)) {
        SendResponse(550, "Invalid path");
        return;
    }
    
    AnsiString fromFullPath = GetAbsolutePath(renameFromPath);
    AnsiString toFullPath = GetAbsolutePath(toPath);
    
    if (MoveFile(fromFullPath.c_str(), toFullPath.c_str())) {
        SendResponse(250, "Rename successful");
        renameFromPath = "";
    } else {
        SendResponse(550, "Rename failed");
    }
}

void ClientHandler::HandleSYST(const AnsiString& param) {
    SendResponse(215, "UNIX");
}

void ClientHandler::HandleNOOP(const AnsiString& param) {
    SendResponse(200, "NOOP ok");
}

void ClientHandler::SendResponse(int code, const AnsiString& message) {
    try {
        AnsiString response = AnsiString().sprintf("%d %s\r\n", code, message.c_str());
        send(session.socket, response.c_str(), response.Length(), 0);
    } catch (...) {
    }
}

bool ClientHandler::ValidatePath(const AnsiString& path) {
    AnsiString fullPath = GetAbsolutePath(path);
    // 檢查路徑是否在共享目錄內
    return fullPath.Pos(sharedFolder) == 1;
}

AnsiString ClientHandler::NormalizePath(const AnsiString& path) {
    if (path.SubString(1, 1) == "/") {
        return path;
    } else {
        return session.currentDirectory + (session.currentDirectory.LastChar() != '/' ? "/" : "") + path;
    }
}

bool ClientHandler::AuthenticateUser(const AnsiString& username, const AnsiString& password) {
    if (!serviceManager) {
        return false;
    }
    
    if (serviceManager->GetADConfig().enabled) {
        if (!serviceManager->VerifyADCredentials(username, password)) {
            return false;
        }
        
        if (!serviceManager->IsUserInAllowedList(username)) {
            return false;
        }
        
        return true;
    } else {
        return true;
    }
}

bool ClientHandler::OpenDataConnection() {
    if (pasvInfo.active) {
        // 被動模式
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET dataSocket = accept(pasvInfo.listenSocket, (sockaddr*)&clientAddr, &addrLen);
        if (dataSocket != INVALID_SOCKET) {
            session.dataSocket = dataSocket;
            return true;
        }
        return false;
    } else if (portInfo.active) {
        // 主動模式
        return EstablishPORTConnection() != INVALID_SOCKET;
    }
    return false;
}

bool ClientHandler::CloseDataConnection() {
    if (session.dataSocket != INVALID_SOCKET) {
        closesocket(session.dataSocket);
        session.dataSocket = INVALID_SOCKET;
    }
    
    if (pasvInfo.listenSocket != INVALID_SOCKET) {
        closesocket(pasvInfo.listenSocket);
        pasvInfo.listenSocket = INVALID_SOCKET;
        pasvInfo.active = false;
    }
    
    portInfo.active = false;
    
    return true;
}

bool ClientHandler::SendFileList(const AnsiString& path, bool nameOnly) {
    AnsiString fullPath = GetAbsolutePath(path);
    
    if (!OpenDataConnection()) {
        return false;
    }
    
    WIN32_FIND_DATA findData;
    HANDLE findHandle = FindFirstFile((fullPath + "*").c_str(), &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        CloseDataConnection();
        return false;
    }
    
    try {
        do {
            if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                AnsiString line;
                if (nameOnly) {
                    line = AnsiString(findData.cFileName) + "\r\n";
                } else {
                    // UNIX 風格的列表格式
                    char perms[11];
                    sprintf_s(perms, sizeof(perms), "%s%s%s%s%s%s%s%s%s%s",
                        findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "d" : "-",
                        findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? "r--" : "rw-",
                        "-rw-",
                        "-", "--", "--", "-- ", "-- ", "-- ", "--");
                    
                    // 簡化的格式: 權限 連結數 所有者 群組 大小 日期 檔名
                    __int64 fileSize = ((__int64)findData.nFileSizeHigh << 32) + findData.nFileSizeLow;
                    
                    SYSTEMTIME st;
                    FileTimeToSystemTime(&findData.ftLastWriteTime, &st);
                    
                    line = AnsiString().sprintf("-rw-r--r-- 1 owner group %10I64d %02d-%02d-%04d %02d:%02d %s\r\n",
                        fileSize,
                        st.wMonth, st.wDay, st.wYear,
                        st.wHour, st.wMinute,
                        AnsiString(findData.cFileName).c_str());
                }
                
                send(session.dataSocket, line.c_str(), line.Length(), 0);
            }
        } while (FindNextFile(findHandle, &findData));
    } catch (...) {
    }
    
    FindClose(findHandle);
    CloseDataConnection();
    return true;
}

bool ClientHandler::SendFile(const AnsiString& path) {
    AnsiString fullPath = GetAbsolutePath(path);
    
    // 檢查文件大小
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesEx(fullPath.c_str(), GetFileExInfoStandard, &fad)) {
        return false;
    }
    
    __int64 fileSize = ((__int64)fad.nFileSizeHigh << 32) + fad.nFileSizeLow;
    if (fileSize > MAX_FILE_SIZE) {
        SendResponse(552, "Requested file action aborted; exceeded storage allocation");
        return false;
    }
    
    if (!OpenDataConnection()) {
        return false;
    }
    
    HANDLE hFile = CreateFile(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        CloseDataConnection();
        return false;
    }
    
    // 處理 REST 命令的斷點續傳
    if (session.restPosition > 0) {
        LARGE_INTEGER li;
        li.QuadPart = session.restPosition;
        SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
        session.restPosition = 0;
    }
    
    try {
        char buffer[DATA_BUFFER_SIZE];
        DWORD bytesRead;
        
        while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
            if (session.binaryMode) {
                send(session.dataSocket, buffer, bytesRead, 0);
            } else {
                // ASCII 模式：轉換 CRLF
                for (DWORD i = 0; i < bytesRead; i++) {
                    if (buffer[i] == '\n' && (i == 0 || buffer[i-1] != '\r')) {
                        send(session.dataSocket, "\r\n", 2, 0);
                    } else {
                        send(session.dataSocket, &buffer[i], 1, 0);
                    }
                }
            }
        }
    } catch (...) {
    }
    
    CloseHandle(hFile);
    CloseDataConnection();
    return true;
}

bool ClientHandler::ReceiveFile(const AnsiString& path, bool append) {
    AnsiString fullPath = GetAbsolutePath(path);
    
    if (!OpenDataConnection()) {
        return false;
    }
    
    DWORD creationDisposition = append ? OPEN_ALWAYS : CREATE_ALWAYS;
    HANDLE hFile = CreateFile(fullPath.c_str(), GENERIC_WRITE, 0, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        CloseDataConnection();
        return false;
    }
    
    // 如果是附加模式，移動到文件末尾
    if (append) {
        SetFilePointer(hFile, 0, NULL, FILE_END);
    } else if (session.restPosition > 0) {
        LARGE_INTEGER li;
        li.QuadPart = session.restPosition;
        SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
    }
    
    try {
        char buffer[DATA_BUFFER_SIZE];
        int bytesRecv;
        __int64 totalSize = 0;
        
        while ((bytesRecv = recv(session.dataSocket, buffer, sizeof(buffer), 0)) > 0) {
            totalSize += bytesRecv;
            
            // 檢查文件大小限制
            if (totalSize > MAX_FILE_SIZE) {
                SendResponse(552, "Requested file action aborted; exceeded storage allocation");
                CloseDataConnection();
                CloseHandle(hFile);
                return false;
            }
            
            DWORD bytesWritten;
            if (!WriteFile(hFile, buffer, bytesRecv, &bytesWritten, NULL)) {
                CloseDataConnection();
                CloseHandle(hFile);
                return false;
            }
        }
    } catch (...) {
    }
    
    CloseHandle(hFile);
    CloseDataConnection();
    session.restPosition = 0;
    return true;
}

AnsiString ClientHandler::GetFileInfo(const AnsiString& path) {
    return path; // TODO: 實現
}

bool ClientHandler::IsPathTraversal(const AnsiString& path) {
    return path.Pos("..") > 0;
}

AnsiString ClientHandler::GetAbsolutePath(const AnsiString& relativePath) {
    AnsiString path = NormalizePath(relativePath);
    
    // 將 / 轉換為 \
    while (path.Pos("/") > 0) {
        int pos = path.Pos("/");
        path = path.SubString(1, pos - 1) + "\\" + path.SubString(pos + 1, path.Length());
    }
    
    return sharedFolder + path;
}

SOCKET ClientHandler::EstablishPASVConnection() {
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0); // 讓系統分配埠
    
    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    
    if (listen(listenSocket, 1) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    
    // 獲取分配的埠
    int addrLen = sizeof(addr);
    if (getsockname(listenSocket, (sockaddr*)&addr, &addrLen) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    
    pasvInfo.listenSocket = listenSocket;
    pasvInfo.port = ntohs(addr.sin_port);
    pasvInfo.active = true;
    
    return listenSocket;
}

SOCKET ClientHandler::EstablishPORTConnection() {
    SOCKET dataSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (dataSocket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(portInfo.hostIP.c_str());
    addr.sin_port = htons(portInfo.port);
    
    if (connect(dataSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(dataSocket);
        return INVALID_SOCKET;
    }
    
    session.dataSocket = dataSocket;
    return dataSocket;
}

AnsiString ClientHandler::ConvertFileSizeToString(__int64 size) {
    if (size < 1024) {
        return AnsiString().sprintf("%I64d B", size);
    } else if (size < 1024 * 1024) {
        return AnsiString().sprintf("%.2f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        return AnsiString().sprintf("%.2f MB", size / (1024.0 * 1024.0));
    } else {
        return AnsiString().sprintf("%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
}

AnsiString ClientHandler::ConvertTimeToFTPFormat(FILETIME ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    return AnsiString().sprintf("%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}