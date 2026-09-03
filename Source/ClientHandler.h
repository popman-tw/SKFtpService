#ifndef CLIENTHANDLERH
#define CLIENTHANDLERH

#include <vcl.h>
#include <Winsock2.h>
#include <vector>

using namespace std;

class FTPServiceManager; // 前向聲明

struct ClientSession {
    SOCKET socket;              // 客戶端 socket
    AnsiString clientIP;        // 客戶端 IP
    int clientPort;             // 客戶端埠號
    AnsiString username;        // 已登入的用戶名
    bool isAuthenticated;       // 是否已驗證
    AnsiString currentDirectory; // 當前目錄
    SOCKET dataSocket;          // 數據連接 socket
    bool binaryMode;            // 傳輸模式 (false=ASCII, true=BINARY)
    __int64 restPosition;           // REST 命令設定的位置，用於斷點續傳
};

struct PASVInfo {
    SOCKET listenSocket;        // 監聽 socket
    unsigned short port;        // 被動模式監聽埠
    bool active;                // 是否已建立被動連接
};

struct PORTInfo {
    AnsiString hostIP;          // 客戶端 IP
    unsigned short port;        // 客戶端數據埠
    bool active;                // 是否已建立主動連接
};

// 文件傳輸信息
struct FileTransferInfo {
    AnsiString filePath;
    HANDLE fileHandle;
    __int64 fileSize;
    __int64 transferredSize;
    bool isUpload;
};

class ClientHandler {
private:
    ClientSession session;
    PASVInfo pasvInfo;
    PORTInfo portInfo;
    FileTransferInfo transferInfo;
    AnsiString sharedFolder;
    bool allowUpload;
    bool allowDownload;
    FTPServiceManager* serviceManager;
    AnsiString renameFromPath;  // 用於 RNFR 命令
    
    static const int MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
    static const int DATA_BUFFER_SIZE = 4096;
    
    // FTP 命令處理
    void HandleUSER(const AnsiString& param);
    void HandlePASS(const AnsiString& param);
    void HandleQUIT(const AnsiString& param);
    void HandleLIST(const AnsiString& param);
    void HandleNLST(const AnsiString& param);
    void HandleCWD(const AnsiString& param);
    void HandleRETR(const AnsiString& param);
    void HandleSTOR(const AnsiString& param);
    void HandleAPPE(const AnsiString& param);
    void HandleDELE(const AnsiString& param);
    void HandlePWD(const AnsiString& param);
    void HandleMKD(const AnsiString& param);
    void HandleRMD(const AnsiString& param);
    void HandleTYPE(const AnsiString& param);
    void HandlePASV(const AnsiString& param);
    void HandlePORT(const AnsiString& param);
    void HandleREST(const AnsiString& param);
    void HandleSIZE(const AnsiString& param);
    void HandleMDTM(const AnsiString& param);
    void HandleABOR(const AnsiString& param);
    void HandleRNFR(const AnsiString& param);
    void HandleRNTO(const AnsiString& param);
    void HandleSYST(const AnsiString& param);
    void HandleNOOP(const AnsiString& param);
    
    // 輔助函數
    void SendResponse(int code, const AnsiString& message);
    bool ValidatePath(const AnsiString& path);
    AnsiString NormalizePath(const AnsiString& path);
    bool AuthenticateUser(const AnsiString& username, const AnsiString& password);
    bool OpenDataConnection();
    bool CloseDataConnection();
    bool SendFileList(const AnsiString& path, bool nameOnly);
    bool SendFile(const AnsiString& path);
    bool ReceiveFile(const AnsiString& path, bool append);
    AnsiString GetFileInfo(const AnsiString& path);
    bool IsPathTraversal(const AnsiString& path);
    AnsiString GetAbsolutePath(const AnsiString& relativePath);
    SOCKET EstablishPASVConnection();
    SOCKET EstablishPORTConnection();
    AnsiString ConvertFileSizeToString(__int64 size);
    AnsiString ConvertTimeToFTPFormat(FILETIME ft);
    
public:
    ClientHandler(SOCKET clientSocket, const AnsiString& clientIP, int clientPort, FTPServiceManager* manager);
    ~ClientHandler();
    
    // 處理客戶端連接
    void HandleClient();
    
    // 設定配置
    void SetSharedFolder(const AnsiString& folder) { sharedFolder = folder; }
    void SetAllowUpload(bool allow) { allowUpload = allow; }
    void SetAllowDownload(bool allow) { allowDownload = allow; }
    
    // 獲取會話信息
    const ClientSession& GetSession() const { return session; }
};

#endif