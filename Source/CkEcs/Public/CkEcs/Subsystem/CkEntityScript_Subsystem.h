#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/UserDefinedStruct.h>

#include "CkEntityScript_Subsystem.generated.h"

// ----------------------------------------------------------------------------------------------------------

/**
 * Subsystem to manage EntityScript configuration structs
 * This subsystem creates and manages on-disk structs for EntityScript configurations
 */
UCLASS(DisplayName = "CkSubsystem_EntityScript")
class CKECS_API UCk_EntityScript_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:

    CK_GENERATED_BODY(UCk_EntityScript_Subsystem_UE);

public:
    [[nodiscard]] auto GetOrCreate_SpawnParamsStructForEntity(UClass* InEntityScriptClass, bool InForceRecreate = false) -> UUserDefinedStruct*;

protected:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

private:
    auto OnObjectSaved(UObject* Object, FObjectPreSaveContext Context) -> void;
    auto OnFilesLoaded() -> void;
    auto OnAssetAdded(const FAssetData& InAssetData) -> void;
    auto OnAssetRenamed(const FAssetData& InAssetData, const FString& InOldObjectPath) -> void;
    auto OnAssetRemoved(const FAssetData& InAssetData) -> void;
    auto RegisterForBlueprintChanges() -> void;
    auto ScanForExistingEntityParamsStructInPath(const FString& InPathToScan) -> void;

    [[nodiscard]] auto Get_StructPathForEntityScriptPath(const FString& InEntityScriptFullPath) -> FString;

private:
    static auto GenerateEntitySpawnParamsStructName(const UClass* InEntityScriptClass) -> FName;
    static auto UpdateStructProperties(UUserDefinedStruct* InStruct, const TArray<FProperty*>& InNewProperties) -> bool;
    static auto IsTemporaryAsset(const FString& InAssetName) -> bool;
    static auto IsEntityScriptStructData(const FAssetData& AssetData) -> bool;
    static auto SaveStruct(UUserDefinedStruct* InStructToSave) -> void;

#if WITH_EDITOR
    static auto DecodePropertyAsPinType(const FProperty* InProperty) -> FEdGraphPinType;
#endif

private:
    UPROPERTY()
    FString _EntitySpawnParams_StructFolderName;

    UPROPERTY()
    TSet<UUserDefinedStruct*> _EntitySpawnParams_Structs;

    UPROPERTY()
    TMap<FName, UUserDefinedStruct*> _EntitySpawnParams_StructsByName;

    UPROPERTY()
    TSet<UUserDefinedStruct*> _EntitySpawnParams_StructsToSave;

private:
    FDelegateHandle _OnFilesLoaded_DelegateHandle;
    FDelegateHandle _OnAssetAdded_DelegateHandle;
    FDelegateHandle _OnAssetRemoved_DelegateHandle;
    FDelegateHandle _OnAssetRenamed_DelegateHandle;
    FDelegateHandle _OnObjectPreSave_DelegateHandle;
    FDelegateHandle _OnBlueprintCompiled_DelegateHandle;
    FDelegateHandle _OnBlueprintReinstanced_DelegateHandle;

    static constexpr const TCHAR* _SpawnParamsStructName_Prefix = TEXT("EntitySpawnParams_");
};

// ----------------------------------------------------------------------------------------------------------