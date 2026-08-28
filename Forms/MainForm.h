#ifndef MAINFORMH
#define MAINFORMH

#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Messages.hpp>
#include "../Source/FTPServer.h"

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
    void AddLogMessage(const AnsiString& message);
    void InitializeFTPServer();
    void CleanupFTPServer();
    void ShowTrayNotification(const AnsiString& title, const AnsiString& message);
    
public:
    __fastcall TMainForm(TComponent* Owner);
    virtual __fastcall ~TMainForm();
    
BEGIN_MESSAGE_MAP
    MESSAGE_HANDLER(WM_USER + 1, TWMUser, WMTrayNotification)
END_MESSAGE_MAP(TForm)
};

extern PACKAGE TMainForm *MainForm;

#endif
