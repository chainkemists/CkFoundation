#include "CkJoltEditor_Module.h"

#include "CkJoltEditor/Cook/CkJoltCook_EditorSubsystem.h"

#include <ToolMenus.h>

#define LOCTEXT_NAMESPACE "FCkJoltEditorModule"

void FCkJoltEditorModule::StartupModule()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
    {
        auto* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
        if (ToolsMenu == nullptr)
        { return; }

        auto& Section = ToolsMenu->FindOrAddSection("CkJolt", LOCTEXT("CkJoltSection", "CkJolt"));

        Section.AddMenuEntry(
            "CkJolt_CookCurrentWorld",
            LOCTEXT("CookJoltStaticWorld", "Cook Jolt Static World (Current Map)"),
            LOCTEXT("CookJoltStaticWorld_Tooltip",
                "Extracts the current map's static collision into cooked Jolt data assets"),
            FSlateIcon{},
            FUIAction{FExecuteAction::CreateLambda([]()
            {
                if (auto* Subsystem = GEditor->GetEditorSubsystem<UCk_JoltCook_EditorSubsystem_UE>())
                { Subsystem->Cook_CurrentWorld(); }
            })});
    }));
}

void FCkJoltEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkJoltEditorModule, CkJoltEditor)
