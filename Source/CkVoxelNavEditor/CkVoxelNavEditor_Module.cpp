#include "CkVoxelNavEditor_Module.h"

#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EdMode.h"
#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EditorSubsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto FCkVoxelNavEditorModule::StartupModule() -> void
{
    (void)UCk_VoxelNavPreview_EdMode::StaticClass();
    UCk_VoxelNavPreview_EditorSubsystem_UE::StaticClass();
}

auto FCkVoxelNavEditorModule::ShutdownModule() -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FCkVoxelNavEditorModule, CkVoxelNavEditor)
