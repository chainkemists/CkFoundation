#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CoreMinimal.h"
#include "Engine/UserDefinedStruct.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SharedPointer.h"
#include "Async/Future.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "CkEntityScript_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_EntityScript_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_Subsystem_UE);

public:
    // Main API - returns nullptr if compilation in progress (deferred)
    UFUNCTION(BlueprintCallable, Category = "Ck|EntityScript")
    UUserDefinedStruct* GetOrCreate_SpawnParamsStructForEntity(
        UClass* InEntityScriptClass,
        bool InForceRecreate = false);

    // Async API - always returns a future
    auto GetOrCreate_SpawnParamsStructForEntity_Async(
        UClass* InEntityScriptClass,
        bool InForceRecreate = false) -> TFuture<UUserDefinedStruct*>;

public:
    // Subsystem overrides
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;

private:
    // Blueprint compilation safety tracking
    struct FPendingSpawnParamsRequest
    {
        TWeakObjectPtr<UClass> EntityScriptClass;
        bool ForceRecreate = false;
        TArray<TWeakPtr<TPromise<UUserDefinedStruct*>>> Promises;
    };

    TArray<FPendingSpawnParamsRequest> _PendingSpawnParamsRequests;
    bool _IsCompilationInProgress = false;
    int32 _ActiveCompilations = 0;

#if WITH_EDITOR
    FDelegateHandle _BlueprintPreCompileHandle;
    FDelegateHandle _BlueprintCompiledHandle;
#endif

    // Core implementation - only called when safe
    auto DoGetOrCreate_SpawnParamsStructForEntity_Internal(
        UClass* InEntityScriptClass,
        bool InForceRecreate) -> UUserDefinedStruct*;

    auto Request_ProcessPendingSpawnParamsRequests() -> void;

    // Compilation event handlers
#if WITH_EDITOR
    UFUNCTION()
    void OnBlueprintPreCompile(UBlueprint* InBlueprint);

    UFUNCTION()
    void OnBlueprintCompiled();
#endif

    // Asset scanning
    auto ScanForExistingEntityParamsStructInPath(const FString& InPathToScan) -> TArray<UUserDefinedStruct*>;

    // Struct creation and management
    auto Request_CreateNewSpawnParamsStruct(UClass* InEntityScriptClass) -> UUserDefinedStruct*;
    auto DoesEntityScriptNeedSpawnParamsStruct(UClass* InEntityScriptClass) -> bool;
    auto Request_AddPropertiesToStruct(UUserDefinedStruct* InStruct, UClass* InEntityScriptClass) -> void;

    // Caching and tracking
    TMap<TWeakObjectPtr<UClass>, TWeakObjectPtr<UUserDefinedStruct>> _EntitySpawnParams_StructsByClass;
    TMap<FString, TWeakObjectPtr<UUserDefinedStruct>> _EntitySpawnParams_StructsByName;
    TSet<TWeakObjectPtr<UUserDefinedStruct>> _EntitySpawnParams_Structs;
    TSet<TWeakObjectPtr<UUserDefinedStruct>> _EntitySpawnParams_StructsToSave;

    // Utility functions
    auto Get_SpawnParamsStructName(UClass* InEntityScriptClass) -> FString;
    auto Get_SpawnParamsStructPackagePath(UClass* InEntityScriptClass) -> FString;
    auto Get_ExpectedEntitySpawnParamsStructPath() -> FString;
    auto Request_SaveStructAsset(UUserDefinedStruct* InStruct) -> bool;

public:
    CK_PROPERTY_GET(_EntitySpawnParams_Structs);
    CK_PROPERTY_GET(_EntitySpawnParams_StructsByClass);
    CK_PROPERTY_GET(_EntitySpawnParams_StructsByName);
};

// --------------------------------------------------------------------------------------------------------------------
