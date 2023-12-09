#include "CkEditorStyle.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"

#define LOCTEXT_NAMESPACE "FCkEditorStyleModule"

auto
    FCkEditorStyleModule::
    StartupModule()
    -> void
{
    UCk_Utils_EditorStyle_UE::DoRegister_SlateStyle();
    UCk_Utils_EditorStyle_UE::DoReloadTextures();
}

auto
    FCkEditorStyleModule::
    ShutdownModule()
    -> void
{
    UCk_Utils_EditorStyle_UE::DoUnregister_SlateStyle();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEditorStyleModule, CkEditorStyle)
