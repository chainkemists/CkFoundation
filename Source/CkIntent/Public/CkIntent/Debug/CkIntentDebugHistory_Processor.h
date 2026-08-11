#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkIntent/Debug/CkIntentDebugHistory_Fragment.h"
#include "CkIntent/CkIntentSampler_Fragment.h"
#include "CkIntent/CkIntent_ProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------
// The whole recorder is compiled out in Shipping (the CkStateMachine/Debug precedent): the fragments are never
// composed there (Add stubs out), so a registered processor would be a per-frame view walk for a feature that
// cannot exist.
#if !UE_BUILD_SHIPPING

namespace ck
{
    /**
     * Copies each newly-sampled row out of the ring into the debug history — the consumer shape the module
     * doc's anti-pattern 5 prescribes for anything wanting more history than the sampler's working window, run
     * at the one place it can never miss a row.
     *
     * UNRATED, like the matcher, and for the same reason: it remembers the last frame index it recorded and
     * takes everything newer, so above 60 fps most passes copy nothing and below it a catch-up burst is captured
     * whole. Shares the matcher's group (both are independent post-Sample readers of the same ring; neither
     * consumes the other's output, so intra-group order between them is immaterial).
     */
    class CKINTENT_API FProcessor_IntentDebugHistory_Record : public ck_exp::TProcessor<
            FProcessor_IntentDebugHistory_Record,
            FCk_Handle_IntentDebugHistory,
            ck::TReadOnly<FFragment_IntentDebugHistory_Params>,
            ck::TReadWrite<FFragment_IntentDebugHistory_Current>,
            ck::TReadOnly<FFragment_IntentSampler_Current>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Intent_Match;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHistory,
            const FFragment_IntentDebugHistory_Params& InParams,
            FFragment_IntentDebugHistory_Current& InCurrent,
            const FFragment_IntentSampler_Current& InSamplerCurrent) -> void;
    };
}

#endif

// --------------------------------------------------------------------------------------------------------------------
