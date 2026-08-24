#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"

#pragma package(smart_init)
#pragma resource "*.dfm"

TMainForm *MainForm;

__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TForm(Owner), ftpServer(NULL), isMinimizedToTray(false) {
}

__fastcall TMainForm::~TMainForm() {
    CleanupFTPServer();
}

void __fastcall TMainForm::FormCreate(TObject *Sender) {
    // 設定窗口屬性
    Caption = "SKFtpService - FTP 服務";
    Width = 600;
    Height = 500;
    Position = poScreenCenter;
    
    // 初始化 FTP 伺服器
    InitializeFTPServer();
    
    // 更新狀態
    UpdateStatusLabel();
    
    AddLogMessage("[系統] 應用程式已啟動");
}

void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action) {
    if (isMinimizedToTray) {
        Action = caNone;  // 阻止關閉，只最小化到托盤
        ShowWindow(SW_HIDE);
    } else {
        Action = caFree;  // 允許關閉
    }
}

void __fastcall TMainForm::FormCloseQuery(TObject *Sender, bool &CanClose) {
    if (ftpServer && ftpServer->IsServiceRunning()) {
        int result = MessageBox(Handle,
            L"FTP 服務仍在運行。\n\n您想要停止服務並退出嗎？",
            L"確認退出",
            MB_YESNO | MB_ICONQUESTION);
        
        if (result == IDYES) {
            ftpServer->StopService();
            AddLogMessage("[系統] FTP 服務已停止");
            CanClose = true;
        } else {
            CanClose = false;
        }
    } else {
        CanClose = true;
    }
}

void __fastcall TMainForm::ButtonStartClick(TObject *Sender) {
    if (ftpServer) {
        if (ftpServer->StartService()) {
            AddLogMessage("[系統] FTP 服務已啟動");
            UpdateStatusLabel();
        } else {
            AddLogMessage("[錯誤] FTP 服務啟動失敗");
            MessageBox(Handle, L"無法啟動 FTP 服務", L"錯誤", MB_OK | MB_ICONERROR);
        }
    }
}

void __fastcall TMainForm::ButtonStopClick(TObject *Sender) {
    if (ftpServer) {
        if (ftpServer->StopService()) {
            AddLogMessage("[系統] FTP 服務已停止");
            UpdateStatusLabel();
        } else {
            AddLogMessage("[錯誤] FTP 服務停止失敗");
        }
    }
}

void __fastcall TMainForm::ButtonSettingsClick(TObject *Sender) {
    AddLogMessage("[系統] 打開設定對話框...");
    MessageBox(Handle, L"設定功能即將推出", L"訊息", MB_OK | MB_ICONINFORMATION);
}

void __fastcall TMainForm::ButtonAboutClick(TObject *Sender) {
    MessageBox(Handle,
        L"SKFtpService v1.0.0\n\nC++ Builder FTP 服務應用程式\n\n" 
        L"功能: 輕量級 FTP 伺服器，支援系統托盤最小化",
        L"關於",
        MB_OK | MB_ICONINFORMATION);
}

void __fastcall TMainForm::TimerUpdateUITimer(TObject *Sender) {
    // 定期更新 UI
    UpdateStatusLabel();
}

void __fastcall TMainForm::MenuShowWindowClick(TObject *Sender) {
    ShowWindow(SW_SHOW);
    SetForegroundWindow(Handle);
    isMinimizedToTray = false;
}

void __fastcall TMainForm::MenuExitClick(TObject *Sender) {
    isMinimizedToTray = false;
    Close();
}

void __fastcall TMainForm::WMTrayNotification(TWMUser &Message) {
    // 處理托盤圖示點擊事件
    if (Message.LParam == WM_LBUTTONDBLCLK) {
        MenuShowWindowClick(NULL);
    } else if (Message.LParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        PopupMenuTray->Popup(pt.x, pt.y);
    }
    Message.Result = 0;
}

void TMainForm::UpdateStatusLabel() {
    if (ftpServer && ftpServer->IsServiceRunning()) {
        LabelStatus->Caption = "狀態: 執行中 ✓";
        LabelStatus->Font->Color = clGreen;
        ButtonStart->Enabled = false;
        ButtonStop->Enabled = true;
    } else {
        LabelStatus->Caption = "狀態: 已停止 ✗";
        LabelStatus->Font->Color = clRed;
        ButtonStart->Enabled = true;
        ButtonStop->Enabled = false;
    }
}

void TMainForm::AddLogMessage(const AnsiString& message) {
    TDateTime now = Now();
    AnsiString timestamp = FormatDateTime("hh:mm:ss", now);
    MemoLog->Lines->Add("[" + timestamp + "] " + message);
    
    // 自動滾動到最新消息
    MemoLog->SelStart = MemoLog->Text.Length();
}

void TMainForm::InitializeFTPServer() {
    if (!ftpServer) {
        ftpServer = new FTPServer();
        if (!ftpServer->Initialize()) {
            AddLogMessage("[錯誤] FTP 服務初始化失敗");
            delete ftpServer;
            ftpServer = NULL;
        } else {
            AddLogMessage("[系統] FTP 服務已初始化");
        }
    }
}

void TMainForm::CleanupFTPServer() {
    if (ftpServer) {
        if (ftpServer->IsServiceRunning()) {
            ftpServer->StopService();
        }
        delete ftpServer;
        ftpServer = NULL;
    }
}

void TMainForm::ShowTrayNotification(const AnsiString& title, const AnsiString& message) {
    // TODO: 實現系統通知
    // 使用 Windows API 顯示托盤通知
}