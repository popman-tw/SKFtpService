#ifndef SYSTEMTRAYH
#define SYSTEMTRAYH

#include <vcl.h>
#include <shellapi.h>

class SystemTray {
private:
    HWND hWnd;              // 窗口句柄
    NOTIFYICONDATA nid;     // 托盤圖示數據
    bool isVisible;         // 窗口可見性狀態
    bool serviceRunning;    // 服務運行狀態
    
public:
    SystemTray();
    ~SystemTray();
    
    // 初始化系統托盤
    bool Initialize();
    
    // 添加托盤圖示
    bool AddIcon(HWND hWindow, UINT uID, HICON hIcon, const AnsiString& tooltip = "");
    
    // 移除托盤圖示
    bool RemoveIcon();
    
    // 更新托盤圖示
    bool UpdateIcon(HICON hIcon, const AnsiString& tooltip = "");
    
    // 設定服務狀態
    void SetServiceStatus(bool running) { serviceRunning = running; }
    
    // 獲取服務狀態
    bool GetServiceStatus() const { return serviceRunning; }
    
    // 顯示主窗口
    void ShowWindow();
    
    // 隱藏主窗口到托盤
    void HideToTray();
    
    // 獲取窗口可見性
    bool IsWindowVisible() const { return isVisible; }
};

#endif