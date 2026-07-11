#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

class ACk_Iskm_BatchedCrowd_Actor;

// ====================================================================================================================
//  inc-4 §4 — put the batched crowd on the ECS clock.
//
//  The crowd is a plain AActor that historically advanced its members in its OWN AActor::Tick — a second clock,
//  unordered against the ECS world-actor tick. Far cosmetics are ECS entities synced by the transform pipeline, so
//  that two-clock split left them ≤1 frame behind the member they follow.
//
//  Fix: one controller entity per crowd carries FFragment_IskmCrowd_Controller; FProcessor_IskmCrowd_Advance ticks
//  it in FGroup_Transform_SyncFrom — AFTER the flip driver's Gameplay_Script member-world writes (so the member is
//  current, not agent-lagged) and BEFORE FGroup_Transform's HandleRequests (so a cosmetic Request_SetTransform lands
//  the SAME tick). It advances member animation (AdvanceAnimation) and places far cosmetics (DriveCosmetics, via the
//  deferred Request path — the same cross-entity write the old flip driver used, safe against the scheduler's
//  write-ordering). Member PushTile + cosmetic both reach FGroup_PostTransform's render flush the same frame, in
//  lockstep. The actor no longer self-ticks.
// ====================================================================================================================
namespace ck
{
    struct CKISKMRENDERER_API FFragment_IskmCrowd_Controller
    {
    public:
        CK_GENERATED_BODY(FFragment_IskmCrowd_Controller);

    public:
        FFragment_IskmCrowd_Controller() = default;
        // Defined out-of-line (CkIskm_BatchedCrowd_Processor.cpp): the TWeakObjectPtr member ctor
        // static_asserts on a complete UObject type outside the unity blob.
        explicit FFragment_IskmCrowd_Controller(ACk_Iskm_BatchedCrowd_Actor* InCrowd);

    private:
        TWeakObjectPtr<ACk_Iskm_BatchedCrowd_Actor> _Crowd;

    public:
        CK_PROPERTY_GET(_Crowd);
    };

    // Runs in FGroup_Transform_SyncFrom (after Gameplay_Script sets member world, before FGroup_Transform applies
    // requests): advances each crowd's members (PushTile) and queues its far cosmetics via Request_SetTransform.
    // HandleRequests applies + tags them the same tick, so the PostTransform render flush picks the cosmetics up the
    // same frame the member is PushTiled — no cross-clock trail.
    class CKISKMRENDERER_API FProcessor_IskmCrowd_Advance : public ck_exp::TProcessor<
        FProcessor_IskmCrowd_Advance,
        FCk_Handle,
        TReadOnly<FFragment_IskmCrowd_Controller>>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmCrowd_Controller& InController) const -> void;
    };
}
