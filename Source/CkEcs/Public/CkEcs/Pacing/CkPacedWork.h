#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "Math/UnrealMathUtility.h"

// --------------------------------------------------------------------------------------------------------------------
// Spreads a pump-eligible processor's work over multiple pump passes (and, for large batches, multiple
// frames) instead of draining it in one pass. Drive it from ForEachEntity with RunPacedSteps, which
// AddOrGets/Removes the dirty marker between passes to keep the scheduler pumping until the work drains.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    enum class EPacedStepResult : uint8
    {
        Continue,
        Done
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_PacedWork
    {
        CK_GENERATED_BODY(FFragment_PacedWork);

    public:
        FFragment_PacedWork();
        FFragment_PacedWork(int32 InStepsPerPass, int32 InMaxStepsPerFrame);

    private:
        int32 _StepsPerPass = 1;
        int32 _MaxStepsPerFrame = 1;
        int32 _StepsRemainingThisFrame = 1;

    public:
        CK_PROPERTY(_StepsPerPass);
        CK_PROPERTY(_MaxStepsPerFrame);
        CK_PROPERTY(_StepsRemainingThisFrame);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runs InStepFn up to StepsPerPass times this pass, bounded by the per-frame budget (refilled on the
    // main Tick, DeltaT > 0). Returns true once InStepFn first reports Done, having removed
    // TMarkerFragment — the dirty marker the consuming processor declares as MarkedDirtyBy.
    template <typename TMarkerFragment = FFragment_PacedWork, typename TStepFn>
    auto RunPacedSteps(
        FCk_Handle& InHandle,
        FFragment_PacedWork& InPacer,
        FCk_Time InDeltaT,
        TStepFn InStepFn) -> bool
    {
        if (InDeltaT.Get_Seconds() > 0.0)
        { InPacer.Set_StepsRemainingThisFrame(InPacer.Get_MaxStepsPerFrame()); }

        const auto Budget        = InPacer.Get_StepsRemainingThisFrame();
        const auto StepsThisPass = FMath::Max(0, FMath::Min(InPacer.Get_StepsPerPass(), Budget));

        auto StepsDone = 0;
        for (; StepsDone < StepsThisPass; ++StepsDone)
        {
            if (InStepFn() == EPacedStepResult::Done)
            {
                InHandle.Remove<TMarkerFragment>();
                return true;
            }
        }

        InPacer.Set_StepsRemainingThisFrame(Budget - StepsDone);

        // A bare ref mutation doesn't bump the dirty-marker version, so AddOrGet re-bumps it to keep the
        // scheduler pumping; skipping it once the budget is spent quiesces us until the next main Tick.
        if (StepsDone > 0 && (Budget - StepsDone) > 0)
        { InHandle.AddOrGet<TMarkerFragment>(); }

        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
