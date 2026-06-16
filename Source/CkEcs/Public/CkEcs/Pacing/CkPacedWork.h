#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "Math/UnrealMathUtility.h"

// --------------------------------------------------------------------------------------------------------------------
// Spread a pump-eligible processor's work over multiple pump passes (and, for large batches, multiple
// frames) instead of draining it in one pass. Drive it from ForEachEntity with RunPacedSteps, which
// AddOrGets/Removes the dirty marker between passes to keep the scheduler pumping until the work drains.
//
// The marker is whatever fragment the consumer declares as MarkedDirtyBy (no SkipPump):
//   - Simple case: the marker IS FFragment_PacedWork — Add it to the work entity to start.
//   - Existing-queue case: the marker is a work-queue fragment every enqueue path already touches
//     (so each enqueue re-marks dirty); pass that type to RunPacedSteps<TMarkerFragment> and keep the
//     FFragment_PacedWork pacer fragment separate (AddOrGet it internally for budget only).
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

    // Runs InStepFn up to StepsPerPass times this pass, bounded by the per-frame budget (refilled on
    // the main Tick, DeltaT > 0). Returns true once InStepFn first reports Done, having removed
    // TMarkerFragment.
    //
    // TMarkerFragment is the dirty marker the consuming processor declares as MarkedDirtyBy — removed
    // on Done (ending pump-eligibility) and re-bumped between passes. It defaults to FFragment_PacedWork
    // (the simple case: the pacer fragment is also the marker). A processor that must stay dirty-marked
    // on a different fragment — e.g. an existing work queue that every enqueue path already touches —
    // passes that type and keeps the pacer fragment separate.
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

        // A bare ref mutation doesn't bump the dirty-marker version, so AddOrGet re-bumps it to keep
        // the scheduler pumping us; skipping it once the frame budget is spent lets the processor
        // quiesce until the next frame's main Tick.
        if (StepsDone > 0 && (Budget - StepsDone) > 0)
        { InHandle.AddOrGet<TMarkerFragment>(); }

        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
