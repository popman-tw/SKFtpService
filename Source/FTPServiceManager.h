#ifndef FTPSERVICEMANAGERH
#define FTPSERVICEMANAGERH

#include <vcl.h>
#include <Winsock2.h>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <winldap.h>

using namespace std;

struct ADConfig {
    bool enabled;               // 是否啟用 AD 認證
    wstring server;             // LDAP 伺服器地址 (ldap://server:389)
    int port;                   // LDAP 埠 (預設 389)
    wstring baseDN;             // 基礎 DN
    wstring allowedGroups;      // 允許的 AD 群組 (用分號分隔)
    wstring allowedUsers;       // 允許的 AD 用戶 (用分號分隔)
    int timeout;                // 連接逾時時間 (秒)
};

struct FTPConfig {
    int port;                   // FTP 埠號 (預設 21)
    wstring sharedFolder;       // 共享目錄
    int maxConnections;         // 最大並發連接數
    bool enableAnonymous;       // 是否啟用匿名登入
    bool allowDownload;         // 是否允許下載
    bool allowUpload;           // 是否允許上傳
    ADConfig adConfig;          // AD 認證配置
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
    bool LoadADConfig(const wstring& configFile);
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
    void SetSharedFolder(const wstring& folder) { config.sharedFolder = folder; }
    void SetMaxConnections(int max) { config.maxConnections = max; }
    void SetEnableAnonymous(bool enable) { config.enableAnonymous = enable; }
    void SetAllowDownload(bool allow) { config.allowDownload = allow; }
    void SetAllowUpload(bool allow) { config.allowUpload = allow; }
    
    // AD 認證相關設定
    void SetADEnabled(bool enable) { config.adConfig.enabled = enable; }
    void SetADServer(const wstring& server) { config.adConfig.server = server; }
    void SetADBaseDN(const wstring& dn) { config.adConfig.baseDN = dn; }
    void SetADAllowedGroups(const wstring& groups) { config.adConfig.allowedGroups = groups; }
    void SetADAllowedUsers(const wstring& users) { config.adConfig.allowedUsers = users; }
    
    // 獲取配置
    const FTPConfig& GetConfig() const { return config; }
    const ADConfig& GetADConfig() const { return config.adConfig; }
    bool IsRunning() const { return isRunning; }
    
    // 獲取連接統計
    int GetActiveConnections() const { return clientSockets.size(); }
    
    // AD 認證方法 (公開給 ClientHandler 使用)
    bool VerifyADCredentials(const wstring& username, const wstring& password);
    bool IsUserInAllowedList(const wstring& username);
};

#endif
