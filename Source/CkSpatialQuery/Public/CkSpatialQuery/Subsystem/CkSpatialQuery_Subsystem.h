#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/CkJolt_ContactEvent.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include "CkSpatialQuery_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * Bridge between the CkJolt-owned physics world and CkSpatialQuery's Probe feature.
 *
 * The Jolt world itself (PhysicsSystem, JobSystem, listeners, debug renderer, the per-tick Update)
 * lives in UCk_Jolt_Subsystem (CkJolt module). This subsystem's job is the Probe-specific consumption:
 *  - translating drained Jolt contact events into Probe Begin/Updated/EndOverlap requests
 *  - gating CkJolt's debug draw on CkSpatialQuery's user settings (PreviewAllProbesUsingJolt)
 *
 * It is deliberately non-tickable: translation runs inside UCk_Jolt_Subsystem's Tick via the
 * drained-events broadcast, preserving the pre-split timing (contacts processed on the game thread
 * BEFORE this frame's physics update).
 */
UCLASS(DisplayName = "CkSubsystem_SpatialQuery")
class CKSPATIALQUERY_API UCk_SpatialQuery_Subsystem : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SpatialQuery_Subsystem);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

private:
    auto
    ProcessQueuedContacts(
        const TArray<FCk_Jolt_ContactEvent>& InEvents) -> void;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_Jolt_Subsystem> _JoltSubsystem;

    FDelegateHandle _ContactEventsHandle;
};

// --------------------------------------------------------------------------------------------------------------------
