#ifndef FTPSERVICEMANAGERH
#define FTPSERVICEMANAGERH

#include <vcl.h>
#include <Winsock2.h>
#include <vector>
#include <memory>
#include <map>

using namespace std;

struct FTPConfig {
    int port;              // FTP 埠號 (預設 21)
    AnsiString sharedFolder; // 共享目錄
    int maxConnections;    // 最大並發連接數
    bool enableAnonymous;  // 是否啟用匿名登入
    bool allowDownload;    // 是否允許下載
    bool allowUpload;      // 是否允許上傳
};

class FTPServiceManager {
private:
    SOCKET listenSocket;
    FTPConfig config;
    bool isRunning;
    vector<SOCKET> clientSockets;
    CRITICAL_SECTION csClientList;
    
    // 私有方法
    bool LoadConfig();
    bool SaveConfig();
    static unsigned int __stdcall AcceptClientThread(void* param);
    static unsigned int __stdcall HandleClientThread(void* param);
    
public:
    FTPServiceManager();
    ~FTPServiceManager();
    
    // 初始化服務
    bool Initialize();
    
    // 啟動 FTP 伺服器
    bool Start();
    
    // 停止 FTP 伺服器
    bool Stop();
    
    // 設定配置
    void SetPort(int port) { config.port = port; }
    void SetSharedFolder(const AnsiString& folder) { config.sharedFolder = folder; }
    void SetMaxConnections(int max) { config.maxConnections = max; }
    void SetEnableAnonymous(bool enable) { config.enableAnonymous = enable; }
    void SetAllowDownload(bool allow) { config.allowDownload = allow; }
    void SetAllowUpload(bool allow) { config.allowUpload = allow; }
    
    // 獲取配置
    const FTPConfig& GetConfig() const { return config; }
    bool IsRunning() const { return isRunning; }
    
    // 獲取連接統計
    int GetActiveConnections() const { return clientSockets.size(); }
};

#endif