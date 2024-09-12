#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Processor/CkProcessorScript.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkEcsWorldStats_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKECS_API UCk_EcsWorld_Stats_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Stats_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

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

private:
    UFUNCTION(BlueprintCallable)
    TArray<FGameplayTag>
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

private:
    struct FEcsWorldStatsData
    {
        FGameplayTag EcsWorldTickingGroup = FGameplayTag::EmptyTag;
        FString EcsWorldTickStatName;
        float TickInclusiveAverageCycleMs = 0.0f;
    };

private:
    FDelegateHandle _OnNewFrameDelegateHandle;
    TArray<FEcsWorldStatsData> _EcsWorldsCollectingStats_OnClient;
    TArray<FEcsWorldStatsData> _EcsWorldsCollectingStats_OnServer;
};

// --------------------------------------------------------------------------------------------------------------------
