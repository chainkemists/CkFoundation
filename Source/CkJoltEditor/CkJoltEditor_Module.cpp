#include "CkJoltEditor_Module.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"

#include "CkJoltEditor/AssetAction/CkJoltMeshShapeCook_AssetAction.h"
#include "CkJoltEditor/Cook/CkJoltCook_EditorSubsystem.h"

#include <Editor.h>
#include <Engine/Engine.h>
#include <Engine/World.h>
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

    /*
     * Undo and redo restore and remove level actors without broadcasting OnLevelActorAdded /
     * OnLevelActorDeleted / OnActorMoved, and this hook names no actors — so every undo/redo pays a full
     * editor-world re-extract. A world with no static-world subsystem has the editor static world mode
     * set to Disabled and is skipped, not ensured.
     */
    static auto DoHandle_PostUndoRedo() -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(GEngine),
            TEXT("Undo/redo fired with no GEngine — the editor worlds cannot be enumerated, so the Jolt "
                 "static world cannot be re-derived"))
        { return; }

        for (const auto& WorldContext : GEngine->GetWorldContexts())
        {
            auto* World = WorldContext.World();

            if (ck::Is_NOT_Valid(World) || World->WorldType != EWorldType::Editor)
            { continue; }

            auto* StaticWorldSubsystem = World->GetSubsystem<UCk_JoltStaticWorld_Subsystem_UE>();

            if (ck::Is_NOT_Valid(StaticWorldSubsystem))
            { continue; }

            StaticWorldSubsystem->Request_ResweepAllLevels();
        }
    }
}

void FCkJoltEditorModule::StartupModule()
{
    _PostUndoRedoHandle = FEditorDelegates::PostUndoRedo.AddStatic(
        &ck_jolt_editor_module::DoHandle_PostUndoRedo);

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

        ck::jolt::cook::RegisterMeshShapeCookContextMenu();
    }));
}

void FCkJoltEditorModule::ShutdownModule()
{
    FEditorDelegates::PostUndoRedo.Remove(_PostUndoRedoHandle);
    _PostUndoRedoHandle.Reset();

    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkJoltEditorModule, CkJoltEditor)
