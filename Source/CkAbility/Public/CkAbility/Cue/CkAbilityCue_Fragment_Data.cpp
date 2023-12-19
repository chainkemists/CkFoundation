#include "CkAbilityCue_Fragment_Data.h"

#include "CkCore/IO/CkIO_Utils.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Fragment_Data.h"

#include "Engine/AssetManager.h"
#include "Engine/ObjectLibrary.h"

#include "UObject/ObjectSaveContext.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Aggregator_PDA::
    Request_Populate() -> void
{
    _AbilityCues.Empty();

    const auto& ThisPath = this->GetPathName();
    const auto& ExtractedPath = UCk_Utils_IO_UE::Get_ExtractPath(ThisPath);

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath{ConfigType::StaticClass()});
    Filter.PackagePaths.Add(*ExtractedPath);
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
    {
        _AbilityCues.Emplace(InAssetData.GetSoftObjectPath());
        return true;
    });
}

// --------------------------------------------------------------------------------------------------------------------
void
    UCk_AbilityCue_Aggregator_PDA::PreSave(
        FObjectPreSaveContext ObjectSaveContext)
{
    Request_Populate();

    Super::PreSave(ObjectSaveContext);
}
