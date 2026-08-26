#include <vcl.h>
#pragma hdrstop

#include "Forms/MainForm.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd) {
    try {
        Application->Initialize();
        Application->MainFormOnTaskBar = true;
        Application->CreateForm(__classid(TMainForm), &MainForm);
        Application->Run();
        return 0;
    }
    catch (Exception &exception) {
        Application->ShowException(&exception);
        return 1;
    }
    catch (...) {
        try {
            throw Exception("");
        }
        catch (Exception &exception) {
            Application->ShowException(&exception);
            return 1;
        }
    }
}
//---------------------------------------------------------------------------
