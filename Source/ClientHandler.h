#ifndef CLIENTHANDLERH
#define CLIENTHANDLERH

#include <vcl.h>
#include <Winsock2.h>
#include <vector>
#include <string>

using namespace std;

class FTPServiceManager; // 前向聲明

struct ClientSession {
    SOCKET socket;              // 客戶端 socket
    wstring clientIP;           // 客戶端 IP
    int clientPort;             // 客戶端埠號
    wstring username;           // 已登入的用戶名
    bool isAuthenticated;       // 是否已驗證
    wstring currentDirectory;   // 當前目錄
    SOCKET dataSocket;          // 數據連接 socket
    bool binaryMode;            // 傳輸模式 (false=ASCII, true=BINARY)
    __int64 restPosition;       // REST 命令設定的位置，用於斷點續傳
};

struct PASVInfo {
    SOCKET listenSocket;        // 監聽 socket
    unsigned short port;        // 被動模式監聽埠
    bool active;                // 是否已建立被動連接
};

struct PORTInfo {
    wstring hostIP;             // 客戶端 IP
    unsigned short port;        // 客戶端數據埠
    bool active;                // 是否已建立主動連接
};

// 文件傳輸信息
struct FileTransferInfo {
    wstring filePath;
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
    wstring sharedFolder;
    bool allowUpload;
    bool allowDownload;
    FTPServiceManager* serviceManager;
    wstring renameFromPath;     // 用於 RNFR 命令
    
    static const int MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
    static const int DATA_BUFFER_SIZE = 4096;
    
    // FTP 命令處理
    void HandleUSER(const wstring& param);
    void HandlePASS(const wstring& param);
    void HandleQUIT(const wstring& param);
    void HandleLIST(const wstring& param);
    void HandleNLST(const wstring& param);
    void HandleCWD(const wstring& param);
    void HandleRETR(const wstring& param);
    void HandleSTOR(const wstring& param);
    void HandleAPPE(const wstring& param);
    void HandleDELE(const wstring& param);
    void HandlePWD(const wstring& param);
    void HandleMKD(const wstring& param);
    void HandleRMD(const wstring& param);
    void HandleTYPE(const wstring& param);
    void HandlePASV(const wstring& param);
    void HandlePORT(const wstring& param);
    void HandleREST(const wstring& param);
    void HandleSIZE(const wstring& param);
    void HandleMDTM(const wstring& param);
    void HandleABOR(const wstring& param);
    void HandleRNFR(const wstring& param);
    void HandleRNTO(const wstring& param);
    void HandleSYST(const wstring& param);
    void HandleNOOP(const wstring& param);
    
    // 輔助函數
    void SendResponse(int code, const wstring& message);
    bool ValidatePath(const wstring& path);
    wstring NormalizePath(const wstring& path);
    bool AuthenticateUser(const wstring& username, const wstring& password);
    bool OpenDataConnection();
    bool CloseDataConnection();
    bool SendFileList(const wstring& path, bool nameOnly);
    bool SendFile(const wstring& path);
    bool ReceiveFile(const wstring& path, bool append);
    wstring GetFileInfo(const wstring& path);
    bool IsPathTraversal(const wstring& path);
    wstring GetAbsolutePath(const wstring& relativePath);
    SOCKET EstablishPASVConnection();
    SOCKET EstablishPORTConnection();
    wstring ConvertFileSizeToString(__int64 size);
    wstring ConvertTimeToFTPFormat(FILETIME ft);
    
    // 字符轉換輔助函數
    static wstring AnsiToWide(const string& ansiStr);
    static string WideToAnsi(const wstring& wideStr);
    
public:
    ClientHandler(SOCKET clientSocket, const wstring& clientIP, int clientPort, FTPServiceManager* manager);
    ~ClientHandler();
    
    // 處理客戶端連接
    void HandleClient();
    
    // 設定配置
    void SetSharedFolder(const wstring& folder) { sharedFolder = folder; }
    void SetAllowUpload(bool allow) { allowUpload = allow; }
    void SetAllowDownload(bool allow) { allowDownload = allow; }
    
    // 獲取會話信息
    const ClientSession& GetSession() const { return session; }
};

#endif
