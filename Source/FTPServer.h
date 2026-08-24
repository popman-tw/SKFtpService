#ifndef FTPSERVERH
#define FTPSERVERH

#include <vcl.h>
#include <map>
#include <vector>
#include <memory>
#include "FTPServiceManager.h"
#include "SystemTray.h"

using namespace std;

class FTPServer {
private:
    FTPServiceManager* serviceManager;
    SystemTray* systemTray;
    bool isRunning;
    HANDLE serverThread;
    
public:
    FTPServer();
    ~FTPServer();
    
    // 初始化 FTP 伺服器
    bool Initialize();
    
    // 啟動 FTP 服務
    bool StartService();
    
    // 停止 FTP 服務
    bool StopService();
    
    // 獲取服務狀態
    bool IsServiceRunning() const { return isRunning; }
    
    // 設定 FTP 埠號
    void SetPort(int port);
    
    // 設定共享目錄
    void SetSharedFolder(const AnsiString& folder);
    
    // 獲取當前配置
    FTPServiceManager* GetServiceManager() { return serviceManager; }
};

#endif