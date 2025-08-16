#pragma once

#include "CkCueBase_EntityScript.h"
#include "CkCue_Fragment_Data.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <Subsystems/EngineSubsystem.h>
#include <GameFramework/GameModeBase.h>

#include "CkCueSubsystem_Base.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCUE_API ACk_CueReplicator_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_CueReplicator_UE);

public:
    ACk_CueReplicator_UE();

public:
    UFUNCTION(Server, Unreliable)
    void
    Server_RequestExecuteCue(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

    UFUNCTION(NetMulticast, Unreliable)
    void
    Request_ExecuteCue(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

protected:
    auto BeginPlay() -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_CueSubsystem_Base_UE> _Subsystem_Cue;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_EcsWorld_Subsystem_UE> _Subsystem_EcsWorld;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_CueReplicator_Base")
class CKCUE_API UCk_CueReplicator_Subsystem_Base_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

    friend class ACk_CueReplicator_UE;

public:
    CK_GENERATED_BODY(UCk_CueReplicator_Subsystem_Base_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

    UFUNCTION(BlueprintCallable)
    void Request_ExecuteCue(
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(BlueprintCallable)
    void Request_ExecuteCue_Local(
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

private:
    auto DoSpawnCueReplicatorActorsForPlayerController(APlayerController* InPlayerController) -> void;
    auto OnPostLoadMapWithWorld(UWorld* InWorld) -> void;
    auto OnPostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_CueReplicator_UE>> _CueReplicators;
    int32 _NextAvailableReplicator = 0;

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
    auto DoHandleRenamed(const FAssetData&, const FString&) -> void;
    auto DoAssetUpdated(const FAssetData&) -> void;

public:
    auto Get_CueEntityScript(const FGameplayTag& InCueName) -> TSubclassOf<UCk_CueBase_EntityScript>;
    auto Get_DiscoveredCues() const -> const TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>>& { return _DiscoveredCues; }

protected:
    UPROPERTY(Transient)
    TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>> _DiscoveredCues;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCUE_API UCk_Utils_CueSubsystem_Base_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    template<typename SubsystemType>
    static auto Request_PopulateCues() -> void
    {
        GEngine->GetEngineSubsystem<SubsystemType>()->Request_PopulateAllCues();
    }
};

// --------------------------------------------------------------------------------------------------------------------
