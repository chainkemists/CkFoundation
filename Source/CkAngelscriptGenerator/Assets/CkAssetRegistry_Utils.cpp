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

auto
    UCk_Utils_AssetRegistry_UE::
    Get_AssetRegistrySubsystem()
    -> UCkAssetRegistrySubsystem*
{
    if (NOT ck::IsValid(GEditor))
    { return nullptr; }

    return GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AssetRegistry_UE::
    BindTo_AssetRegistryProgress(
        const FOnAssetRegistryProgressDelegate& InDelegate)
    -> void
{
    auto AssetRegistrySubsystem = Get_AssetRegistrySubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(AssetRegistrySubsystem),
        TEXT("Asset Registry Subsystem not found - cannot bind to progress delegate"))
    { return; }

    AssetRegistrySubsystem->OnAssetRegistryProgress.Add(InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AssetRegistry_UE::
    UnbindFrom_AssetRegistryProgress(
        const FOnAssetRegistryProgressDelegate& InDelegate)
    -> void
{
    auto AssetRegistrySubsystem = Get_AssetRegistrySubsystem();

    if(ck::Is_NOT_Valid(AssetRegistrySubsystem))
    { return; }

    AssetRegistrySubsystem->OnAssetRegistryProgress.Remove(InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AssetRegistry_UE::
    BindTo_AssetRegistryComplete(
        const FOnAssetRegistryCompleteDelegate& InDelegate)
    -> void
{
    auto AssetRegistrySubsystem = Get_AssetRegistrySubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(AssetRegistrySubsystem),
        TEXT("Asset Registry Subsystem not found - cannot bind to complete delegate"))
    { return; }

    AssetRegistrySubsystem->OnAssetRegistryComplete.Add(InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AssetRegistry_UE::
    UnbindFrom_AssetRegistryComplete(
        const FOnAssetRegistryCompleteDelegate& InDelegate)
    -> void
{
    auto AssetRegistrySubsystem = Get_AssetRegistrySubsystem();

    if(ck::Is_NOT_Valid(AssetRegistrySubsystem))
    { return; }

    AssetRegistrySubsystem->OnAssetRegistryComplete.Remove(InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AssetRegistry_UE::
    UnbindAll_AssetRegistryDelegates()
    -> void
{
    auto AssetRegistrySubsystem = Get_AssetRegistrySubsystem();

    if(ck::Is_NOT_Valid(AssetRegistrySubsystem))
    { return; }

    AssetRegistrySubsystem->OnAssetRegistryProgress.Clear();
    AssetRegistrySubsystem->OnAssetRegistryComplete.Clear();
}

// --------------------------------------------------------------------------------------------------------------------
