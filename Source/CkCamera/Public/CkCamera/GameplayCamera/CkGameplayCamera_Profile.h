#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkGameplayCamera_Profile.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// The composed camera profile — the accumulated result of every active modifier's contribution.
//
// M0: a minimal schema (FOV + boom length + framing offset) sufficient to drive a trivial third-person POV
// and prove the plumbing. M1 expands this to full parity with the reference profile (Rig/Springs/Sensor/
// Noise/OrientationControl/AutoReorient/Collision/DoF). Modifiers blend into this via DoContributeToProfile.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile);

private:
    // Field of View (degrees)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _FOV = 90.0f;

    // Fixed distance the camera sits behind the pivot.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _BoomArmLength = 300.0f;

    // Local-space framing offset applied at the boom end.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _FramingOffset = FVector::ZeroVector;

public:
    CK_PROPERTY(_FOV);
    CK_PROPERTY(_BoomArmLength);
    CK_PROPERTY(_FramingOffset);
};

// --------------------------------------------------------------------------------------------------------------------
