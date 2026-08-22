#include "CkJoltEditor_Module.h"

#include "CkJoltEditor/Cook/CkJoltCook_EditorSubsystem.h"

#include <ToolMenus.h>

#define LOCTEXT_NAMESPACE "FCkJoltEditorModule"

namespace ck_jolt_editor_module
{
    static auto DoAdd_CookEntry(
        FToolMenuSection& InSection,
        FName InEntryName,
        const FText& InLabel,
        const FText& InTooltip,
        TFunction<void(UCk_JoltCook_EditorSubsystem_UE&)> InAction) -> void
    {
        InSection.AddMenuEntry(
            InEntryName,
            InLabel,
            InTooltip,
            FSlateIcon{},
            FUIAction{FExecuteAction::CreateLambda([Action = MoveTemp(InAction)]()
            {
                if (auto* Subsystem = GEditor->GetEditorSubsystem<UCk_JoltCook_EditorSubsystem_UE>())
                { Action(*Subsystem); }
            })});
    }
}

void FCkJoltEditorModule::StartupModule()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
    {
        using namespace ck_jolt_editor_module;

        auto* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
        if (ToolsMenu == nullptr)
        { return; }

        auto& Section = ToolsMenu->FindOrAddSection("CkJolt", LOCTEXT("CkJoltSection", "CkJolt"));

        DoAdd_CookEntry(Section,
            "CkJolt_CookCurrentWorld_Incremental",
            LOCTEXT("CookJoltStaticWorldIncremental", "Cook Jolt Static World (Current Map, Incremental)"),
            LOCTEXT("CookJoltStaticWorldIncremental_Tooltip",
                "Rewrites only the bake-grid cells whose actors changed since the last cook. Leaves cooked "
                "actors in unloaded sublevels untouched. Runs in the background with a progress notification."),
            [](UCk_JoltCook_EditorSubsystem_UE& InSubsystem)
            { InSubsystem.Request_CookStaticWorld(); });

        DoAdd_CookEntry(Section,
            "CkJolt_CookCurrentWorld",
            LOCTEXT("CookJoltStaticWorld", "Cook Jolt Static World (Current Map, Full)"),
            LOCTEXT("CookJoltStaticWorld_Tooltip",
                "Rebuilds every cell of the current map's cooked Jolt data from scratch. Only the levels "
                "loaded right now end up in the index."),
            [](UCk_JoltCook_EditorSubsystem_UE& InSubsystem)
            { InSubsystem.Cook_CurrentWorld(); });

        DoAdd_CookEntry(Section,
            "CkJolt_CookMeshShapes",
            LOCTEXT("CookJoltMeshShapes", "Cook Jolt Mesh Shapes (Baked Roots)"),
            LOCTEXT("CookJoltMeshShapes_Tooltip",
                "Refreshes the pre-baked per-mesh Jolt shapes under the configured BakedMeshShapeRoots. "
                "Runs in the background with a progress notification."),
            [](UCk_JoltCook_EditorSubsystem_UE& InSubsystem)
            { InSubsystem.Request_CookMeshShapes(); });

        DoAdd_CookEntry(Section,
            "CkJolt_ValidateCurrentWorld",
            LOCTEXT("ValidateJoltStaticWorld", "Validate Cooked Jolt Static World (Current Map)"),
            LOCTEXT("ValidateJoltStaticWorld_Tooltip",
                "Reports which of the current map's actors are stale in the cooked data. Writes nothing."),
            [](UCk_JoltCook_EditorSubsystem_UE& InSubsystem)
            { InSubsystem.Validate_CurrentWorld(); });
    }));
}

void FCkJoltEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkJoltEditorModule, CkJoltEditor)
