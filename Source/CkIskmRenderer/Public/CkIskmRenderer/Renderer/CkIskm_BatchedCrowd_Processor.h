#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

class ACk_Iskm_BatchedCrowd_Actor;

// --------------------------------------------------------------------------------------------------------------------
// Puts the batched crowd on the ECS clock: one controller entity per crowd carries
// FFragment_IskmCrowd_Controller, and FProcessor_IskmCrowd_Advance ticks it. The crowd actor never self-ticks —
// a second AActor::Tick clock left ECS far cosmetics ≤1 frame behind the member they follow.
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

    // MUST stay in FGroup_Transform_SyncFrom: after Gameplay_Script has set member world transforms, and before
    // FGroup_Transform applies requests, so a cosmetic's queued Request_SetTransform lands the SAME tick.
    class CKISKMRENDERER_API FProcessor_IskmCrowd_Advance : public ck_exp::TProcessor<
        FProcessor_IskmCrowd_Advance,
        FCk_Handle,
        TReadOnly<FFragment_IskmCrowd_Controller>>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;
        // Non-PIE playback is owned by the editor-preview subsystem, where per-user policy and a bounded
        // cadence avoid advancing every crowd through the main ECS graph on every editor frame.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

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
