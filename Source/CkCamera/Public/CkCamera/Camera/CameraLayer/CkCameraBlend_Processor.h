#pragma once

#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------
// Auto-blend: advances each layer's blend weight and rewrites the effective delta of every modifier it acquired, from
// the layer-authored TARGET delta scaled by the weight (per operation). Runs in FGroup_Gameplay_TimeDelta — strictly
// BEFORE the attribute RecomputeAll/Compute processors in FGroup_Gameplay — so the camera POV (FGroup_Gameplay_Camera,
// much later) reads fully up-to-date attribute finals the SAME frame (zero blend lag). The layer author never touches
// blend weights; they only set the TARGET delta via Acquire_CameraModifier_<Tuner>.
//
// Effective delta (against the underlying revocable Add/Multiply modifier — revocable Override is unsupported, so the
// user-facing Override op is realized as an Add modifier whose delta drives Final from base toward target):
//   Additive       (Add)      → target * alpha
//   Multiplicative (Multiply) → Lerp(1, target, alpha)
//   Override       (Add)      → alpha * (target - base)      [→ Final = base + that = Lerp(base, target, alpha)]
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCAMERA_API FProcessor_CameraLayer_Blend : public ck_exp::TProcessor<
            FProcessor_CameraLayer_Blend,
            FCk_Handle_CameraLayer,
            TReadWrite<FFragment_CameraLayer_Blend>,
            TReadWrite<FFragment_CameraLayer_AcquiredModifiers>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InLayer,
            FFragment_CameraLayer_Blend& InBlend,
            FFragment_CameraLayer_AcquiredModifiers& InAcquired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
