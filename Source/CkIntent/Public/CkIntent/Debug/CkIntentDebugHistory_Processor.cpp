#include "CkIntentDebugHistory_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIntent/CkIntentSampler_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING

CK_REGISTER_PROCESSOR(ck::FProcessor_IntentDebugHistory_Record);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IntentDebugHistory_Record::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHistory,
            const FFragment_IntentDebugHistory_Params& InParams,
            FFragment_IntentDebugHistory& InHistoryComp,
            const FFragment_IntentSampler& InSamplerComp)
        -> void
    {
        const auto Sampler = UCk_Utils_IntentSampler_UE::CastChecked(InHistory);
        const auto FrameCount = UCk_Utils_IntentSampler_UE::Get_FrameCount(Sampler);

        // Oldest retained first, so a catch-up burst lands in order. The cursor makes each row copy exactly
        // once regardless of how many render frames a logic frame spanned.
        for (auto Offset = FrameCount - 1; Offset >= 0; --Offset)
        {
            const auto Row = UCk_Utils_IntentSampler_UE::TryGet_FrameAtOffset(Sampler, Offset);

            if (Row.Get_FrameIndex() <= InHistoryComp._LastRecordedFrame)
            { continue; }

            InHistoryComp._Rows.Add(Row);
            InHistoryComp._LastRecordedFrame = Row.Get_FrameIndex();
        }

        if (const auto Excess = InHistoryComp._Rows.Num() - InHistoryComp._Capacity; Excess > 0)
        { InHistoryComp._Rows.RemoveAt(0, Excess); }
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
