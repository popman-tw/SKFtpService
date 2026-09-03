#ifndef MAINFORMH
#define MAINFORMH

#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Menus.hpp>
#include <windows.h>
#include <string>
#include "../Source/FTPServer.h"

using namespace std;

// 自訂訊息結構 - 用於托盤通知
struct TWMUser {
    unsigned int Msg;
    WPARAM WParam;
    LPARAM LParam;
    LRESULT Result;
};

class TMainForm : public TForm {
__published:
    // UI 組件
    TPanel *PanelTop;
    TLabel *LabelStatus;
    TButton *ButtonStart;
    TButton *ButtonStop;
    TButton *ButtonSettings;
    TButton *ButtonAbout;
    TPanel *PanelLog;
    TMemo *MemoLog;
    TTimer *TimerUpdateUI;
    TPopupMenu *PopupMenuTray;
    TMenuItem *MenuShowWindow;
    TMenuItem *MenuSeparator;
    TMenuItem *MenuExit;
    
    // 事件處理器
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
    void __fastcall ButtonStartClick(TObject *Sender);
    void __fastcall ButtonStopClick(TObject *Sender);
    void __fastcall ButtonSettingsClick(TObject *Sender);
    void __fastcall ButtonAboutClick(TObject *Sender);
    void __fastcall TimerUpdateUITimer(TObject *Sender);
    void __fastcall MenuShowWindowClick(TObject *Sender);
    void __fastcall MenuExitClick(TObject *Sender);
    void __fastcall WMTrayNotification(TWMUser &Message);
    
private:
    FTPServer* ftpServer;
    bool isMinimizedToTray;
    
    void UpdateStatusLabel();
    void AddLogMessage(const wstring& message);
    void InitializeFTPServer();
    void CleanupFTPServer();
    void ShowTrayNotification(const wstring& title, const wstring& message);
    
protected:
    // 訊息處理
    virtual LRESULT __fastcall WndProc(TMessage &Message);
    
public:
    __fastcall TMainForm(TComponent* Owner);
    virtual __fastcall ~TMainForm();
};

extern PACKAGE TMainForm *MainForm;

#endif
