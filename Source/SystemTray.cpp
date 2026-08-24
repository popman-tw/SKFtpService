#include "SystemTray.h"
#include <stdio.h>

SystemTray::SystemTray() : hWnd(NULL), isVisible(true), serviceRunning(false) {
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
}

SystemTray::~SystemTray() {
    RemoveIcon();
}

bool SystemTray::Initialize() {
    // 托盤功能初始化
    // 實際的窗口句柄將在 AddIcon 時設定
    return true;
}

bool SystemTray::AddIcon(HWND hWindow, UINT uID, HICON hIcon, const AnsiString& tooltip) {
    hWnd = hWindow;
    
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = uID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP + 1;  // 自定義消息
    nid.hIcon = hIcon;
    
    if (!tooltip.IsEmpty()) {
        strncpy(nid.szTip, tooltip.c_str(), sizeof(nid.szTip) - 1);
    } else {
        strcpy(nid.szTip, "SKFtpService");
    }
    
    return Shell_NotifyIcon(NIM_ADD, &nid) ? true : false;
}

bool SystemTray::RemoveIcon() {
    if (hWnd == NULL) {
        return false;
    }
    
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    
    return Shell_NotifyIcon(NIM_DELETE, &nid) ? true : false;
}

bool SystemTray::UpdateIcon(HICON hIcon, const AnsiString& tooltip) {
    if (hWnd == NULL) {
        return false;
    }
    
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP;
    nid.hIcon = hIcon;
    
    if (!tooltip.IsEmpty()) {
        strncpy(nid.szTip, tooltip.c_str(), sizeof(nid.szTip) - 1);
    }
    
    return Shell_NotifyIcon(NIM_MODIFY, &nid) ? true : false;
}

void SystemTray::ShowWindow() {
    if (hWnd != NULL) {
        ::ShowWindow(hWnd, SW_SHOW);
        ::SetForegroundWindow(hWnd);
        isVisible = true;
    }
}

void SystemTray::HideToTray() {
    if (hWnd != NULL) {
        ::ShowWindow(hWnd, SW_HIDE);
        isVisible = false;
    }
}