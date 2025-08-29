#pragma once

#include "CkCueBase_EntityScript.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include <Subsystems/EngineSubsystem.h>
#include <GameFramework/GameModeBase.h>

#include "CkCueSubsystem_Base.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCUE_API ACk_CueExecutor_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_CueExecutor_UE);

public:
    ACk_CueExecutor_UE();

public:
    UFUNCTION(Server, Unreliable)
    void
    Server_RequestExecuteCue(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

    UFUNCTION(NetMulticast, Unreliable)
    void
    Request_ExecuteCue(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

protected:
    auto BeginPlay() -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_CueExecutor_Subsystem_Base_UE> _Subsystem_CueExecutor;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_EcsWorld_Subsystem_UE> _Subsystem_EcsWorld;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, DisplayName = "CkSubsystem_CueExecutor_Base")
class CKCUE_API UCk_CueExecutor_Subsystem_Base_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

    friend class ACk_CueExecutor_UE;

public:
    CK_GENERATED_BODY(UCk_CueExecutor_Subsystem_Base_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

protected:
    virtual auto Get_CueSubsystemClass() const -> TSubclassOf<class UCk_CueSubsystem_Base_UE>
        PURE_VIRTUAL(UCk_CueExecutor_Subsystem_Base_UE::Get_CueSubsystemClass, return {};);

private:
    auto DoSpawnCueExecutorActorsForPlayerController(APlayerController* InPlayerController) -> void;
    auto OnPostLoadMapWithWorld(UWorld* InWorld) -> void;
    auto OnPostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_CueExecutor_UE>> _CueExecutors;
    int32 _NextAvailableExecutor = 0;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APlayerController>> _ValidPlayerControllers;

private:
    FDelegateHandle _PostLoadMapWithWorldDelegateHandle;
    FDelegateHandle _PostLoginEventDelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract)
class CKCUE_API UCk_CueSubsystem_Base_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueSubsystem_Base_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;

public:
    auto Request_PopulateAllCues() -> void;

protected:
    virtual auto Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript> PURE_VIRTUAL(UCk_CueSubsystem_Base_UE::Get_CueBaseClass, return {};);

private:
    auto DoOnEngineInitComplete() -> void;
    auto DoHandleAssetAddedDeleted(const FAssetData&) -> void;
    auto Request_ProcessAssetUpdate(const FAssetData& InAssetData) -> void;
    auto Request_PopulateBlueprintCues() -> void;
    auto DoHandleRenamed(const FAssetData&, const FString&) -> void;
    auto DoAssetUpdated(const FAssetData&) -> void;

public:
    auto Get_CueEntityScript(const FGameplayTag& InCueName) -> TSubclassOf<UCk_CueBase_EntityScript>;
    auto Get_DiscoveredCues() const -> const TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>>&;

protected:
    UPROPERTY(Transient)
    TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>> _DiscoveredCues;
};

// --------------------------------------------------------------------------------------------------------------------
