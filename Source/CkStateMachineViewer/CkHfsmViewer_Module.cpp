#include "CkHfsmViewer_Module.h"

#include "CkHfsmViewer_Log.h"
#include "CkStateMachineViewer/CkHfsmViewer_Window.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

void FCkHfsmViewerModule::StartupModule()
{
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Ck.HfsmViewer"),
        TEXT("Opens the HFSM Viewer window"),
        FConsoleCommandDelegate::CreateRaw(this, &FCkHfsmViewerModule::OpenViewer));

    ck::hfsmviewer::Log(TEXT("CkStateMachineViewer module loaded"));
}

// --------------------------------------------------------------------------------------------------------------------

void FCkHfsmViewerModule::ShutdownModule()
{
    IConsoleManager::Get().UnregisterConsoleObject(TEXT("Ck.HfsmViewer"));

    if (_Window.IsValid())
    {
        _Window->Close();
        _Window.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewerModule::
    OpenViewer()
    -> void
{
    if (_Window.IsValid() && _Window->IsOpen())
    {
        _Window->Close();
        _Window.Reset();
        return;
    }

    _Window = MakeShared<FCkHfsmViewer_Window>();
    _Window->Open();
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FCkHfsmViewerModule, CkStateMachineViewer)
