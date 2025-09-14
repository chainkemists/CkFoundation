#include "CkAssetRegistry_Utils.h"

#include "CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include <Editor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AssetRegistry_UE::
    GenerateAllAssetRegistries()
    -> void
{
    ck::angelscriptgenerator::Log(TEXT("=== Triggering Asset Registry Generation ==="));

    auto AssetRegistrySubsystem = GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();

    CK_ENSURE_IF_NOT(ck::IsValid(AssetRegistrySubsystem),
        TEXT("Asset Registry Subsystem not found - is the module loaded?"))
    { return; }

    AssetRegistrySubsystem->GenerateAllAssetRegistries();
}

// --------------------------------------------------------------------------------------------------------------------
