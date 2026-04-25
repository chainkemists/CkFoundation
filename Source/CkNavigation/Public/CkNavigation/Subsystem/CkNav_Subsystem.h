#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <CoreMinimal.h>
#include <Engine/TimerHandle.h>

#include "CkNav_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ARecastNavMesh;
class dtCrowd;
class dtNavMesh;

// --------------------------------------------------------------------------------------------------------------------

// Custom deleter for TSharedPtr<dtCrowd>: dtCrowd is a raw C++ type without ref-counting,
// so we wrap it in a TSharedPtr with a custom deleter that calls dtFreeCrowd. This lets us
// publish TWeakPtr<dtCrowd> into the registry context (mirrors the JPH::PhysicsSystem pattern
// in CkSpatialQuery_Subsystem.cpp:551).
struct FCk_DtCrowd_Deleter
{
    void operator()(dtCrowd* InCrowd) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Navigation")
class CKNAVIGATION_API UCk_Navigation_Subsystem : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Navigation_Subsystem);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

private:
    // P3-3 / Pass-3.1 N3: navmesh-regen delegate handler with pointer-equality check + debounce.
    UFUNCTION()
    void HandleNavmeshRegenerated(ANavigationData* InNavData);

    // P3-3: debounced rebuild path — fires from the timer set in HandleNavmeshRegenerated.
    UFUNCTION()
    void DoExecuteCrowdRebuild();

    // Extracted from Initialize so HandleNavmeshRegenerated can re-run the alloc/publish flow.
    auto DoReallocateCrowdAndPublishContext() -> bool;

    // Configures all 4 obstacle-avoidance tiers (Low/Med/High/Best). Without distinct values,
    // the ECk_Nav_AvoidanceQuality enum would be a no-op.
    static auto DoConfigureObstacleAvoidanceProfiles(dtCrowd& InCrowd) -> void;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(Transient)
    TWeakObjectPtr<ARecastNavMesh> _NavMesh;

    // Owning ref. Custom deleter calls dtFreeCrowd.
    TSharedPtr<dtCrowd> _Crowd;

    // P3-3 identity cache: compare against current dtNavMesh* on regen-fire to ignore tile-only updates.
    void* _CachedDetourNavMesh = nullptr;

    // P3-3 debounce timer for full-mesh rebuild coalescing.
    FTimerHandle _RebuildTimerHandle;
};

// --------------------------------------------------------------------------------------------------------------------
