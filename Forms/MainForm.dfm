object MainForm: TMainForm
  Left = 0
  Top = 0
  Caption = 'SKFtpService - FTP Server'
  ClientHeight = 500
  ClientWidth = 600
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object PanelTop: TPanel
    Left = 0
    Top = 0
    Width = 600
    Height = 100
    Align = alTop
    BevelOuter = bvNone
    Color = clWhite
    ParentBackground = False
    TabOrder = 0
    object LabelStatus: TLabel
      Left = 16
      Top = 16
      Width = 568
      Height = 25
      AutoSize = False
      Caption = 'Status: Stopped'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -16
      Font.Name = 'Tahoma'
      Font.Style = [fsBold]
      ParentFont = False
      Layout = tlCenter
    end
    object ButtonStart: TButton
      Left = 16
      Top = 55
      Width = 75
      Height = 25
      Caption = 'Start'
      OnClick = ButtonStartClick
      TabOrder = 0
    end
    object ButtonStop: TButton
      Left = 97
      Top = 55
      Width = 75
      Height = 25
      Caption = 'Stop'
      Enabled = False
      OnClick = ButtonStopClick
      TabOrder = 1
    end
    object ButtonSettings: TButton
      Left = 178
      Top = 55
      Width = 75
      Height = 25
      Caption = 'Settings'
      OnClick = ButtonSettingsClick
      TabOrder = 2
    end
    object ButtonAbout: TButton
      Left = 509
      Top = 55
      Width = 75
      Height = 25
      Caption = 'About'
      OnClick = ButtonAboutClick
      TabOrder = 3
    end
  end
  object PanelLog: TPanel
    Left = 0
    Top = 100
    Width = 600
    Height = 400
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 1
    object MemoLog: TMemo
      Left = 0
      Top = 0
      Width = 600
      Height = 400
      Align = alClient
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Courier New'
      Font.Style = []
      ParentFont = False
      ReadOnly = True
      ScrollBars = ssBoth
      TabOrder = 0
    end
  end
  object TimerUpdateUI: TTimer
    OnTimer = TimerUpdateUITimer
    Left = 16
    Top = 136
  end
  object PopupMenuTray: TPopupMenu
    Left = 48
    Top = 136
    object MenuShowWindow: TMenuItem
      Caption = 'Show Window'
      OnClick = MenuShowWindowClick
    end
    object MenuSeparator: TMenuItem
      Caption = '-'
    end
    object MenuExit: TMenuItem
      Caption = 'Exit'
      OnClick = MenuExitClick
    end
  end
end