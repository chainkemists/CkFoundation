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
    auto Tick(float InDeltaSeconds) -> void override;

private:
    // World-subsystem Initialize fires before UNavigationSystemV1 is created in the world,
    // so we can't bind the regen delegate during Initialize. Tick polls until NavSys exists,
    // binds the delegate (and grabs NavData if it's up too), then latches.
    auto DoTryBindNavSystem() -> void;

    // P3-3 / Pass-3.1 N3: navmesh-regen delegate handler with pointer-equality check + debounce.
    UFUNCTION()
    void HandleNavmeshRegenerated(ANavigationData* InNavData);

    // P3-3: debounced rebuild path — fires from the timer set in HandleNavmeshRegenerated.
    UFUNCTION()
    void DoExecuteCrowdRebuild();

    // Extracted from Initialize so HandleNavmeshRegenerated can re-run the alloc/publish flow.
    auto DoReallocateCrowdAndPublishContext() -> bool;

    // Re-stamps FTag_Nav_NeedsSetup on every nav agent so the FProcessor_Nav_CrowdSetup
    // dirty marker re-fires. Called both during regen-driven rebuilds AND the saved-bake
    // fast path — agents added before _Crowd was first allocated had their dirty event
    // consumed by an early-returning CrowdSetup tick, and won't be re-queued otherwise.
    auto DoReMarkAgentsForCrowdSetup() -> void;

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

    // Latch — once the regen delegate is bound, stop polling in Tick.
    bool _NavSystemBound = false;
};

// --------------------------------------------------------------------------------------------------------------------
