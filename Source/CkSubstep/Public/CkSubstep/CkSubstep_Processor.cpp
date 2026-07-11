#include "CkSubstep_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkSubstep/CkSubstep_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DWORD_COUNTER_STAT(TEXT("Substep Steps Executed"), STAT_Substep_StepsExecuted, STATGROUP_CkSubstep);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Substep_Update);

namespace ck
{
    auto
        FProcessor_Substep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_Substep& InHandle,
            const FFragment_Substep_Params& InParams,
            FFragment_Substep_Current& InCurrent) const
        -> void
    {
        // Zero-dt tick = settle pass: do no time-dependent work and consume no one-shot markers. Early-out before
        // the FirstUpdate consume and the Substep signal broadcasts so a settle pump neither advances substeps nor
        // burns the FirstUpdate marker.
        if (InDeltaT.Get_Seconds() <= 0.0f)
        { return; }

        if (InHandle.Has<FTag_Substep_FirstUpdate>())
        {
            UUtils_Signal_OnSubstepFirstUpdate::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
            InHandle.Remove<FTag_Substep_FirstUpdate>();
            InCurrent._DeltaOverflowFromLastFrame = FCk_Time::ZeroSecond();
        }

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
            INC_DWORD_STAT(STAT_Substep_StepsExecuted);
            AdjustedTickRate -= TickRate;
            UUtils_Signal_OnSubstepUpdate::Broadcast(InHandle, MakePayload(InHandle, TickRate, StepNumber, InDeltaT));

            if (NOT InHandle.Has<FTag_Substep_Update>())
            { break; }
        }

        InCurrent._DeltaOverflowFromLastFrame = AdjustedTickRate;
        UUtils_Signal_OnSubstepFrameEnd::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
    }
}

// --------------------------------------------------------------------------------------------------------------------