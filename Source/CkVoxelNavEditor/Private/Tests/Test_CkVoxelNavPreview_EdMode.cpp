#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EdMode.h"
#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EditorSubsystem.h"

#include <Editor.h>
#include <Misc/AutomationTest.h>
#include <Misc/ScopeExit.h>

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_VoxelNav_DebugSnapshot_LevelEditorOverlayToggle,
    "Ck.VoxelNav.DebugSnapshot.LevelEditorOverlayToggle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_VoxelNav_DebugSnapshot_LevelEditorOverlayToggle::RunTest(const FString& Parameters)
{
    if (NOT TestNotNull(TEXT("editor exists"), GEditor) || GEditor->PlayWorld != nullptr)
    { return false; }

    const auto WasEnabled = UCk_VoxelNavPreview_EdMode::Get_IsLevelOverlayEnabled();
    ON_SCOPE_EXIT
    {
        UCk_VoxelNavPreview_EdMode::Set_LevelOverlayEnabled(WasEnabled);
    };

    TestTrue(TEXT("outside-PIE overlay can be enabled"),
        UCk_VoxelNavPreview_EdMode::Set_LevelOverlayEnabled(true));
    TestTrue(TEXT("enabled state is reported by the editor mode manager"),
        UCk_VoxelNavPreview_EdMode::Get_IsLevelOverlayEnabled());

    const auto* PreviewSubsystem = UCk_VoxelNavPreview_EditorSubsystem_UE::Get();
    TestNotNull(TEXT("editor preview subsystem is available"), PreviewSubsystem);
    if (PreviewSubsystem != nullptr)
    {
        const auto Published = PreviewSubsystem->Get_RenderSnapshots();
        TestTrue(TEXT("renderer publication is always a valid immutable collection"), Published.IsValid());
    }

    TestTrue(TEXT("outside-PIE overlay can be disabled"),
        UCk_VoxelNavPreview_EdMode::Set_LevelOverlayEnabled(false));
    TestFalse(TEXT("disabled state is reported by the editor mode manager"),
        UCk_VoxelNavPreview_EdMode::Get_IsLevelOverlayEnabled());
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
