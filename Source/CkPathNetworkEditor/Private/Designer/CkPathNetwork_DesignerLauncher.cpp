#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerLauncher.h"

#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerEdMode.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Editor.h>
#include <Engine/World.h>
#include <Framework/Docking/TabManager.h>
#include <LevelEditor.h>
#include <Modules/ModuleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer
{
    auto
    Open_Designer() -> void
    {
        if (NOT Can_OpenDesigner())
        { return; }

        auto& ModeTools = GLevelEditorModeTools();
        ModeTools.ActivateMode(
            UCk_PathNetworkDesigner_EdMode::EM_CkPathNetworkDesignerModeId);

        auto& LevelEditor =
            FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        if (const auto TabManager = LevelEditor.GetLevelEditorTabManager();
            TabManager.IsValid()
            && TabManager->HasTabSpawner(
                LevelEditorTabIds::LevelEditorToolBox))
        {
            TabManager->TryInvokeTab(LevelEditorTabIds::LevelEditorToolBox);
        }
    }

    auto
    Can_OpenDesigner() -> bool
    {
        if (ck::Is_NOT_Valid(GEditor)
            || GEditor->PlayWorld != nullptr)
        { return false; }

        const auto* World = GEditor->GetEditorWorldContext().World();
        return ck::IsValid(World)
            && World->WorldType == EWorldType::Editor;
    }
}

// --------------------------------------------------------------------------------------------------------------------
