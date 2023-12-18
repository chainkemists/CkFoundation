#include "CkAbilityCue_Fragment_Data.h"

#include "CkCore/IO/CkIO_Utils.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Fragment_Data.h"

#include "Engine/AssetManager.h"
#include "Engine/ObjectLibrary.h"

#include "UObject/ObjectSaveContext.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Aggregator_DA::
    Request_Populate() -> void
{
    _AbilityCues.Empty();

    const auto& ThisPath = this->GetPathName();
    const auto& ExtractedPath = UCk_Utils_IO_UE::Get_ExtractPath(ThisPath);

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath{UCk_EntityBridge_Config_Base_PDA::StaticClass()});
    Filter.PackagePaths.Add(*ExtractedPath);
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
    {
        _AbilityCues.Emplace(TSoftObjectPtr<UCk_EntityBridge_Config_Base_PDA>{InAssetData.GetSoftObjectPath()});
        return true;
    });
}

// --------------------------------------------------------------------------------------------------------------------
void
    UCk_AbilityCue_Aggregator_DA::PreSave(
        FObjectPreSaveContext ObjectSaveContext)
{
    _AbilityCues.Empty();

    // calculate normal of a vector

    const auto& ThisPath = this->GetPathName();
    const auto& ExtractedPath = UCk_Utils_IO_UE::Get_ExtractPath(ThisPath);

    // Get the AssetRegistryModule
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Create a filter to specify the asset class and root path
    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath{UCk_EntityBridge_Config_Base_PDA::StaticClass()});
    Filter.PackagePaths.Add(*ExtractedPath);
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    // Use the AssetRegistryModule to get a list of assets that match the filter
    TArray<FAssetData> AssetData;
    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
    {
        _AbilityCues.Emplace(TSoftObjectPtr<UCk_EntityBridge_Config_Base_PDA>{InAssetData.GetSoftObjectPath()});
        return true;
    });

    Super::PreSave(ObjectSaveContext);
}
