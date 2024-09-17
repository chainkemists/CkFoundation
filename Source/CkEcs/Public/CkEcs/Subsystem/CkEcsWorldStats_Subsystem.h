#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Processor/CkProcessorScript.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkEcsWorldStats_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_EcsWorldWithStats_MinimalInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EcsWorldWithStats_MinimalInfo);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _DisplayName = NAME_None;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTag _EcsWorldTickingGroup = FGameplayTag::EmptyTag;

public:
    CK_PROPERTY_GET(_DisplayName);
    CK_PROPERTY_GET(_EcsWorldTickingGroup);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_EcsWorldWithStats_MinimalInfo, _DisplayName, _EcsWorldTickingGroup);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API ACk_EcsWorld_StatReplicatorActor_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_EcsWorld_StatReplicatorActor_UE);

public:
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    ACk_EcsWorld_StatReplicatorActor_UE();

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

public:
    auto
    Initialize(
        const FGameplayTag& InAssociatedEcsWorldTickingGroup) -> void;

protected:
    auto
    BeginPlay() -> void override;

private:
    UFUNCTION()
    void
    OnRep_AssociatedEcsWorldTickingGroupAverageCycleMs();

    UFUNCTION()
    void
    OnRep_AssociatedEcsWorldTickingGroup();

private:
    UPROPERTY(ReplicatedUsing = OnRep_AssociatedEcsWorldTickingGroup)
    FGameplayTag _AssociatedEcsWorldTickingGroup;

    UPROPERTY(ReplicatedUsing = OnRep_AssociatedEcsWorldTickingGroupAverageCycleMs)
    float _AssociatedEcsWorldTickingGroupAverageCycleMs = 0.0f;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_EcsWorld_Stats_Subsystem_UE> _EcsWorldStatsSubsystem;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKECS_API UCk_EcsWorld_Stats_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Stats_Subsystem_UE);

public:
    friend class ACk_EcsWorld_StatReplicatorActor_UE;

private:
    struct FEcsWorldStatsData
    {
        FCk_EcsWorldWithStats_MinimalInfo EcsWorldMinimalInfo;
        FString TickStatName;
        float TickInclusiveAverageCycleMs = 0.0f;
    };

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

    auto
    ShouldCreateSubsystem(
        UObject* Outer) const -> bool override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto
    OnWorldBeginPlay(
        UWorld& InWorld) -> void override;

    auto
    Tick(
        float InDeltaTime) -> void override;

    auto
    GetStatId() const -> TStatId override;

private:
    auto
    OnNewFrame(
        int64 InNewFrame) -> void;

    auto
    DoTryUpdateEcsWorldStatData(
        const FComplexStatMessage& InStatMessage) -> void;

    auto
    DoTryEnableEcsWorldStat() const -> void;

private:
    UFUNCTION(BlueprintCallable)
    TArray<FCk_EcsWorldWithStats_MinimalInfo>
    Get_EcsWorldsCollectingStats(
        ECk_ClientServer InStatCollectionSource) const;

    UFUNCTION(BlueprintCallable)
    float
    Get_StatDataForEcsWorldTickingGroup(
        UPARAM(meta = (Categories = EcsWorldTickingGroup)) FGameplayTag InEcsWorld,
        ECk_ClientServer InStatCollectionSource) const;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TWeakObjectPtr<ACk_EcsWorld_StatReplicatorActor_UE>> _StatReplicatorActors;

private:
    FDelegateHandle _OnNewFrameDelegateHandle;
    TArray<FEcsWorldStatsData> _EcsWorldsCollectingStats_OnClient;
    TArray<FEcsWorldStatsData> _EcsWorldsCollectingStats_OnServer;
};

// --------------------------------------------------------------------------------------------------------------------
