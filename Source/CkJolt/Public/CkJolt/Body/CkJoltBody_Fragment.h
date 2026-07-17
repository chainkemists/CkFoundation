#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Set by the Jolt step's pose-apply pass (FJoltWorld::DoApplyPoseBuffer_GameThread) whenever an
    // active body's StepPose was refreshed this frame. The consuming JoltBody quartet (added later)
    // clears it after pushing the pose onto the entity's Transform.
    CK_DEFINE_ECS_TAG(FTag_JoltBody_TransformDirty);

    // --------------------------------------------------------------------------------------------------------------------

    // Latest + previous simulated pose (UE-space) for an active Jolt body, captured post-step off the
    // pose buffer. Prev/Curr let a downstream interpolator blend by the step alpha. Minimal on purpose —
    // the JoltBody feature quartet extends this file later.
    struct CKJOLT_API FFragment_JoltBody_StepPose
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltBody_StepPose);

    private:
        FVector _PrevLocation = FVector::ZeroVector;
        FQuat   _PrevRotation = FQuat::Identity;
        FVector _CurrLocation = FVector::ZeroVector;
        FQuat   _CurrRotation = FQuat::Identity;

    public:
        CK_PROPERTY(_PrevLocation);
        CK_PROPERTY(_PrevRotation);
        CK_PROPERTY(_CurrLocation);
        CK_PROPERTY(_CurrRotation);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_JoltBody_StepPose, _PrevLocation, _PrevRotation, _CurrLocation, _CurrRotation);
    };
}

// --------------------------------------------------------------------------------------------------------------------
