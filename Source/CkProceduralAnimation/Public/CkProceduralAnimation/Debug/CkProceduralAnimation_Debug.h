#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProceduralAnimation_Debug.generated.h"

class UWorld;

// Value-only diagnostic records remain readable after their source entity is destroyed.
// Probe fields describe the last actual attempt, including rejected inside-origin hits.
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugLeg
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugLeg);

private:
    FName _Id;
    FVector _HipWorld = FVector::ZeroVector;
    FVector _NeutralWorld = FVector::ZeroVector;
    FVector _QueryTarget = FVector::ZeroVector;
    FVector _IdealTarget = FVector::ZeroVector;
    FVector _PlantedPosition = FVector::ZeroVector;
    FVector _SwingTarget = FVector::ZeroVector;
    FVector _FootPosition = FVector::ZeroVector;
    FQuat _FootRotation = FQuat::Identity;
    FVector _Normal = FVector::UpVector;
    float _SwingAlpha = 0.0f;
    float _PhaseOffset = 0.0f;
    float _StepThreshold = 0.0f;
    bool _Planted = false;
    bool _TargetValid = false;
    bool _ContactTrusted = false;
    ck::EProceduralFootProbeState _ProbeState = ck::EProceduralFootProbeState::Grounded;
    FCk_Time _MissingContact;
    FVector _ProbeStart = FVector::ZeroVector;
    FVector _ProbeEnd = FVector::ZeroVector;
    FVector _ProbeHitPosition = FVector::ZeroVector;
    FVector _ProbeHitNormal = FVector::ZeroVector;
    float _ProbeHitFraction = 0.0f;
    int32 _ProbeAttemptCount = 0;
    bool _ProbeHit = false;
    bool _HasRig = false;
    bool _UpperAvailable = false;
    bool _LowerAvailable = false;
    bool _FootAvailable = false;
    FTransform _UpperTransform = FTransform::Identity;
    FTransform _LowerTransform = FTransform::Identity;
    FTransform _FootTransform = FTransform::Identity;

public:
    CK_PROPERTY(_Id);
    CK_PROPERTY(_HipWorld);
    CK_PROPERTY(_NeutralWorld);
    CK_PROPERTY(_QueryTarget);
    CK_PROPERTY(_IdealTarget);
    CK_PROPERTY(_PlantedPosition);
    CK_PROPERTY(_SwingTarget);
    CK_PROPERTY(_FootPosition);
    CK_PROPERTY(_FootRotation);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_SwingAlpha);
    CK_PROPERTY(_PhaseOffset);
    CK_PROPERTY(_StepThreshold);
    CK_PROPERTY(_Planted);
    CK_PROPERTY(_TargetValid);
    CK_PROPERTY(_ContactTrusted);
    CK_PROPERTY(_ProbeState);
    CK_PROPERTY(_MissingContact);
    CK_PROPERTY(_ProbeStart);
    CK_PROPERTY(_ProbeEnd);
    CK_PROPERTY(_ProbeHitPosition);
    CK_PROPERTY(_ProbeHitNormal);
    CK_PROPERTY(_ProbeHitFraction);
    CK_PROPERTY(_ProbeAttemptCount);
    CK_PROPERTY(_ProbeHit);
    CK_PROPERTY(_HasRig);
    CK_PROPERTY(_UpperAvailable);
    CK_PROPERTY(_LowerAvailable);
    CK_PROPERTY(_FootAvailable);
    CK_PROPERTY(_UpperTransform);
    CK_PROPERTY(_LowerTransform);
    CK_PROPERTY(_FootTransform);
};

// Body, motion, probe and solver values belong to the last accepted advancing solve.
// Current status/freshness flags and actual rig transforms are overlaid at query time.
// RigPosePending marks parts whose deferred transform requests have not yet applied.
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugSnapshot
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugSnapshot);

private:
    FName _EntityName;
    FString _EntityId;
    bool _Available = false;
    bool _HasAcceptedSample = false;
    bool _GaitReady = false;
    bool _GaitFailed = false;
    bool _HasSurfaceMotion = false;
    bool _HasRig = false;
    bool _RigReady = false;
    ECk_ProceduralRig_Failure _RigFailure = ECk_ProceduralRig_Failure::None;
    bool _GaitFresh = false;
    bool _MotionMatchesGaitFrame = false;
    bool _RigMatchesGaitSequence = false;
    bool _RigPosePending = false;
    uint64 _FrameNumber = 0;
    uint64 _Sequence = 0;
    FCk_Time _Time;
    FTransform _BodyTransform = FTransform::Identity;
    float _GaitClock = 0.0f;
    float _CadenceScale = 1.0f;
    float _CadenceSpeed = 0.0f;
    bool _Airborne = false;
    FCk_Time _RestTime;
    FVector _Velocity = FVector::ZeroVector;
    FVector _MotionVelocity = FVector::ZeroVector;
    FVector _SupportNormal = FVector::UpVector;
    FVector _RequestedDirection = FVector::ZeroVector;
    float _RequestedSpeed = 0.0f;
    bool _Grounded = false;
    bool _TrustedContact = false;
    FCk_Time _MissingContact;
    TArray<FCk_ProceduralAnimation_DebugLeg> _Legs;

public:
    CK_PROPERTY(_EntityName);
    CK_PROPERTY(_EntityId);
    CK_PROPERTY(_Available);
    CK_PROPERTY(_HasAcceptedSample);
    CK_PROPERTY(_GaitReady);
    CK_PROPERTY(_GaitFailed);
    CK_PROPERTY(_HasSurfaceMotion);
    CK_PROPERTY(_HasRig);
    CK_PROPERTY(_RigReady);
    CK_PROPERTY(_RigFailure);
    CK_PROPERTY(_GaitFresh);
    CK_PROPERTY(_MotionMatchesGaitFrame);
    CK_PROPERTY(_RigMatchesGaitSequence);
    CK_PROPERTY(_RigPosePending);
    CK_PROPERTY(_FrameNumber);
    CK_PROPERTY(_Sequence);
    CK_PROPERTY(_Time);
    CK_PROPERTY(_BodyTransform);
    CK_PROPERTY(_GaitClock);
    CK_PROPERTY(_CadenceScale);
    CK_PROPERTY(_CadenceSpeed);
    CK_PROPERTY(_Airborne);
    CK_PROPERTY(_RestTime);
    CK_PROPERTY(_Velocity);
    CK_PROPERTY(_MotionVelocity);
    CK_PROPERTY(_SupportNormal);
    CK_PROPERTY(_RequestedDirection);
    CK_PROPERTY(_RequestedSpeed);
    CK_PROPERTY(_Grounded);
    CK_PROPERTY(_TrustedContact);
    CK_PROPERTY(_MissingContact);
    CK_PROPERTY(_Legs);
};

UCLASS(NotBlueprintable)
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralAnimation_Debug_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralAnimation_Debug_UE);

    // Game-thread only. No queries, simulation, asset loads or entity mutation.
    static auto
    Get_Snapshot(
        const FCk_Handle& InHandle) -> FCk_ProceduralAnimation_DebugSnapshot;

    // Discovery returns live handles; history must retain only copied snapshots.
    static auto
    Get_Entities(
        UWorld* InWorld) -> TArray<FCk_Handle>;
};
