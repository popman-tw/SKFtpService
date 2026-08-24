#ifndef CLIENTHANDLERH
#define CLIENTHANDLERH

#include <vcl.h>
#include <Winsock2.h>
#include <vector>

using namespace std;

struct ClientSession {
    SOCKET socket;              // 客户端 socket
    AnsiString clientIP;        // 客户端 IP
    int clientPort;             // 客户端埠号
    AnsiString username;        // 已登入的用戶名
    bool isAuthenticated;       // 是否已驗證
    AnsiString currentDirectory; // 當前目錄
};

class ClientHandler {
private:
    ClientSession session;
    AnsiString sharedFolder;
    bool allowUpload;
    bool allowDownload;
    
    // FTP 命令處理
    void HandleUSER(const AnsiString& param);
    void HandlePASS(const AnsiString& param);
    void HandleQUIT(const AnsiString& param);
    void HandleLIST(const AnsiString& param);
    void HandleCWD(const AnsiString& param);
    void HandleRETR(const AnsiString& param);
    void HandleSTOR(const AnsiString& param);
    void HandleDELE(const AnsiString& param);
    void HandlePWD(const AnsiString& param);
    void HandleMKD(const AnsiString& param);
    void HandleRMD(const AnsiString& param);
    void HandleTYPE(const AnsiString& param);
    void HandlePASV(const AnsiString& param);
    void HandlePORT(const AnsiString& param);
    
    // 輔助函數
    void SendResponse(int code, const AnsiString& message);
    bool ValidatePath(const AnsiString& path);
    AnsiString NormalizePath(const AnsiString& path);
    
public:
    ClientHandler(SOCKET clientSocket, const AnsiString& clientIP, int clientPort);
    ~ClientHandler();
    
    // 處理用戶端連接
    void HandleClient();
    
    // 設定配置
    void SetSharedFolder(const AnsiString& folder) { sharedFolder = folder; }
    void SetAllowUpload(bool allow) { allowUpload = allow; }
    void SetAllowDownload(bool allow) { allowDownload = allow; }
    
    // 獲取會話信息
    const ClientSession& GetSession() const { return session; }
};

#endif