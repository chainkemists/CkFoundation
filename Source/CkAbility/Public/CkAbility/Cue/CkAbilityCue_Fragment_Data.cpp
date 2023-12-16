#include "CkAbilityCue_Fragment_Data.h"

#include "CkCore/IO/CkIO_Utils.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Fragment_Data.h"

#include "Engine/AssetManager.h"
#include "Engine/ObjectLibrary.h"

#include "UObject/ObjectSaveContext.h"

// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
void
    UCk_AbilityCue_Aggregator_DA::PreSave(
        FObjectPreSaveContext ObjectSaveContext)
{
    _AbilityCues.Empty();

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

    // Iterate through the list of assets and load them
    //for (const FAssetData& Asset : AssetData)
    //{
    //    // Get the soft object path for the asset
    //    FSoftObjectPath AssetPath = Asset.ToSoftObjectPath();

    //    _AbilityCues.Emplace(TSoftObjectPtr<UCk_EntityBridge_Config_Base_PDA>{AssetPath});

    //    // Load the asset synchronously (replace with asynchronous loading if needed)
    //    //UObject* LoadedAsset = AssetPath.TryLoad();
    //    //if (LoadedAsset)
    //    //{
    //    //    // Do something with the loaded asset
    //    //    // ...
    //    //}
    //}




    //const auto ObjectLibrary = UObjectLibrary::CreateLibrary(UCk_EntityBridge_Config_Base_PDA::StaticClass(), true, GIsEditor && !IsRunningCommandlet());


    //const auto& ThisPath = this->GetPathName();
    //const auto& ExtractedPath = UCk_Utils_IO_UE::Get_ExtractPath(ThisPath);

    //ObjectLibrary->LoadBlueprintAssetDataFromPath(ExtractedPath);
    //ObjectLibrary->LoadAssetsFromAssetData();

    //TArray<FAssetData> AssetDataList;
    //ObjectLibrary->GetAssetDataList(AssetDataList);

    //TArray<UCk_EntityBridge_Config_Base_PDA*> Objects;
    //ObjectLibrary->GetObjects(Objects);

    //for (const auto Object : Objects)
    //{
    //    _AbilityCues.Emplace(Object);
    //}

    Super::PreSave(ObjectSaveContext);
}

void
    UCk_AbilityCue_Aggregator_DA::PostLoad()
{
    Super::PostLoad();
}
