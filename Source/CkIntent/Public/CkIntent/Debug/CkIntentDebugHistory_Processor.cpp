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
            FFragment_IntentDebugHistory_Current& InCurrent,
            const FFragment_IntentSampler_Current& InSamplerCurrent)
        -> void
    {
        const auto Sampler = UCk_Utils_IntentSampler_UE::CastChecked(InHistory);
        const auto FrameCount = UCk_Utils_IntentSampler_UE::Get_FrameCount(Sampler);

        // Oldest retained first, so a catch-up burst lands in order. The cursor makes each row copy exactly
        // once regardless of how many render frames a logic frame spanned.
        for (auto Offset = FrameCount - 1; Offset >= 0; --Offset)
        {
            const auto Row = UCk_Utils_IntentSampler_UE::TryGet_FrameAtOffset(Sampler, Offset);

            if (Row.Get_FrameIndex() <= InCurrent._LastRecordedFrame)
            { continue; }

            InCurrent._Rows.Add(Row);
            InCurrent._LastRecordedFrame = Row.Get_FrameIndex();
        }

        if (const auto Excess = InCurrent._Rows.Num() - InCurrent._Capacity; Excess > 0)
        { InCurrent._Rows.RemoveAt(0, Excess); }
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
