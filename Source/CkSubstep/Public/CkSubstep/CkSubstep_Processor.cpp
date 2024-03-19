#include "CkSubstep_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Substep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            const FCk_Handle_Substep& InHandle,
            const FFragment_Substep_Params& InParams,
            FFragment_Substep_Current& InCurrent) const
        -> void
    {
        auto AdjustedTickRate = InDeltaT + InCurrent.Get_DeltaOverflowFromLastFrame();

        if (InParams.Get_Data().Get_TickRate() == TimeType::ZeroSecond())
        {
            constexpr auto StepNumber = 1;
            UUtils_Signal_OnSubstepUpdate::Broadcast(InHandle, MakePayload(InHandle, InDeltaT, StepNumber, InDeltaT));
            UUtils_Signal_OnSubstepFrameEnd::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
            return;
        }

        const auto& TickRate = InParams.Get_Data().Get_TickRate();

        auto StepNumber = 0;
        while(AdjustedTickRate >= TickRate)
        {
            ++StepNumber;
            AdjustedTickRate -= TickRate;
            UUtils_Signal_OnSubstepUpdate::Broadcast(InHandle, MakePayload(InHandle, TickRate, StepNumber, InDeltaT));
        }

        InCurrent._DeltaOverflowFromLastFrame = AdjustedTickRate;
        UUtils_Signal_OnSubstepFrameEnd::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
    }
}

// --------------------------------------------------------------------------------------------------------------------