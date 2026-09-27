#pragma once

#include "CkProceduralAnimation/CkProceduralAnimation_Fragment_Data.h"
#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"
#include "CkProceduralAnimation/Core/CkProceduralFoothold.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProceduralAnimation_Debug.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugPart
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugPart);

private:
    bool _Available = false;
    FTransform _Transform = FTransform::Identity;

public:
    CK_PROPERTY(_Available);
    CK_PROPERTY(_Transform);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugLegTargeting
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugLegTargeting);

private:
    FVector _HipWorld = FVector::ZeroVector;
    FVector _NeutralWorld = FVector::ZeroVector;
    FVector _QueryTarget = FVector::ZeroVector;
    FVector _IdealTarget = FVector::ZeroVector;
    bool _TargetValid = false;
    float _StepThreshold = 0.0f;

public:
    CK_PROPERTY(_HipWorld);
    CK_PROPERTY(_NeutralWorld);
    CK_PROPERTY(_QueryTarget);
    CK_PROPERTY(_IdealTarget);
    CK_PROPERTY(_TargetValid);
    CK_PROPERTY(_StepThreshold);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugFoot
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugFoot);

private:
    FVector _Position = FVector::ZeroVector;
    FQuat _Rotation = FQuat::Identity;
    FVector _Normal = FVector::UpVector;
    FVector _PlantedPosition = FVector::ZeroVector;
    FVector _SwingTarget = FVector::ZeroVector;
    float _SwingAlpha = 0.0f;
    float _PhaseOffset = 0.0f;
    bool _Planted = false;
    bool _ContactTrusted = false;

public:
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_PlantedPosition);
    CK_PROPERTY(_SwingTarget);
    CK_PROPERTY(_SwingAlpha);
    CK_PROPERTY(_PhaseOffset);
    CK_PROPERTY(_Planted);
    CK_PROPERTY(_ContactTrusted);
};

// --------------------------------------------------------------------------------------------------------------------

// Probe fields describe the last actual attempt, including rejected inside-origin hits.
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugProbe
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugProbe);

private:
    ck::EProceduralFootProbeState _State = ck::EProceduralFootProbeState::Grounded;
    FCk_Time _MissingContact;
    FVector _Start = FVector::ZeroVector;
    FVector _End = FVector::ZeroVector;
    FVector _HitPosition = FVector::ZeroVector;
    FVector _HitNormal = FVector::ZeroVector;
    float _HitFraction = 0.0f;
    int32 _AttemptCount = 0;
    bool _Hit = false;

public:
    CK_PROPERTY(_State);
    CK_PROPERTY(_MissingContact);
    CK_PROPERTY(_Start);
    CK_PROPERTY(_End);
    CK_PROPERTY(_HitPosition);
    CK_PROPERTY(_HitNormal);
    CK_PROPERTY(_HitFraction);
    CK_PROPERTY(_AttemptCount);
    CK_PROPERTY(_Hit);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugLegRig
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugLegRig);

private:
    bool _Composed = false;
    ECk_ProceduralAnimation_Status _Status = ECk_ProceduralAnimation_Status::PendingSetup;
    ECk_ProceduralRig_Failure _Failure = ECk_ProceduralRig_Failure::None;
    TArray<FCk_ProceduralAnimation_DebugPart> _Segments;
    FCk_ProceduralAnimation_DebugPart _Foot;

public:
    CK_PROPERTY(_Composed);
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Failure);
    CK_PROPERTY(_Segments);
    CK_PROPERTY(_Foot);
};

// --------------------------------------------------------------------------------------------------------------------

// One foothold candidate of a solve, in world space.
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugFoothold
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugFoothold);

private:
    FVector _Position = FVector::ZeroVector;
    FVector _Normal = FVector::UpVector;
    ck::EProceduralFootholdSource _Source = ck::EProceduralFootholdSource::None;
    ck::EProceduralFootholdVerdict _Verdict = ck::EProceduralFootholdVerdict::Miss;

public:
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_Source);
    CK_PROPERTY(_Verdict);
};

// --------------------------------------------------------------------------------------------------------------------

// Value-only diagnostic records remain readable after their source entity is destroyed. _LandingPointWorld is the point a
// swinging foot's landing ground was probed under this solve (the swing's landing point as of the previous solve) and
// _LandingProbe that ray; the probe is not attempted (zero attempts) while the leg is planted or on a catch step.
// _Footholds lists the candidates this solve validated, in the order they were cast, and _ChosenFoothold the one that
// became the target (INDEX_NONE when none was usable).
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugLeg
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugLeg);

private:
    FName _Id;
    FString _LegEntityId;
    bool _Enabled = true;
    FCk_ProceduralAnimation_DebugLegTargeting _Targeting;
    FCk_ProceduralAnimation_DebugFoot _Foot;
    FCk_ProceduralAnimation_DebugProbe _Probe;
    FVector _LandingPointWorld = FVector::ZeroVector;
    FCk_ProceduralAnimation_DebugProbe _LandingProbe;
    FCk_ProceduralAnimation_DebugLegRig _Rig;
    TArray<FCk_ProceduralAnimation_DebugFoothold, TInlineAllocator<16>> _Footholds;
    int32 _ChosenFoothold = INDEX_NONE;
    ck::EProceduralFootholdSource _FootholdSource = ck::EProceduralFootholdSource::None;
    bool _PlantOccluded = false;

public:
    CK_PROPERTY(_Id);
    CK_PROPERTY(_LegEntityId);
    CK_PROPERTY(_Enabled);
    CK_PROPERTY(_Targeting);
    CK_PROPERTY(_Foot);
    CK_PROPERTY(_Probe);
    CK_PROPERTY(_LandingPointWorld);
    CK_PROPERTY(_LandingProbe);
    CK_PROPERTY(_Rig);
    CK_PROPERTY(_Footholds);
    CK_PROPERTY(_ChosenFoothold);
    CK_PROPERTY(_FootholdSource);
    CK_PROPERTY(_PlantOccluded);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugStatus
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugStatus);

private:
    bool _Available = false;
    bool _HasAcceptedSample = false;
    ECk_ProceduralAnimation_Status _GaitStatus = ECk_ProceduralAnimation_Status::PendingSetup;
    ECk_ProceduralGait_Failure _GaitFailure = ECk_ProceduralGait_Failure::None;
    bool _HasSurfaceMotion = false;
    bool _HasRig = false;
    ECk_ProceduralAnimation_Status _RigStatus = ECk_ProceduralAnimation_Status::PendingSetup;
    ECk_ProceduralRig_Failure _RigFailure = ECk_ProceduralRig_Failure::None;

public:
    CK_PROPERTY(_Available);
    CK_PROPERTY(_HasAcceptedSample);
    CK_PROPERTY(_GaitStatus);
    CK_PROPERTY(_GaitFailure);
    CK_PROPERTY(_HasSurfaceMotion);
    CK_PROPERTY(_HasRig);
    CK_PROPERTY(_RigStatus);
    CK_PROPERTY(_RigFailure);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugFreshness
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugFreshness);

private:
    bool _GaitFresh = false;
    bool _MotionMatchesGaitFrame = false;
    bool _RigMatchesGaitSequence = false;
    bool _RigPosePending = false;

public:
    CK_PROPERTY(_GaitFresh);
    CK_PROPERTY(_MotionMatchesGaitFrame);
    CK_PROPERTY(_RigMatchesGaitSequence);
    CK_PROPERTY(_RigPosePending);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugSample
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugSample);

private:
    uint64 _FrameNumber = 0;
    uint64 _Sequence = 0;
    FCk_Time _Time;

public:
    CK_PROPERTY(_FrameNumber);
    CK_PROPERTY(_Sequence);
    CK_PROPERTY(_Time);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugGait
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugGait);

private:
    FTransform _BodyTransform = FTransform::Identity;
    float _Clock = 0.0f;
    float _CadenceScale = 1.0f;
    float _CadenceSpeed = 0.0f;
    bool _Airborne = false;
    FCk_Time _RestTime;
    FVector _Velocity = FVector::ZeroVector;
    FVector _SupportNormal = FVector::UpVector;
    // The cadence speed reference the solver runs with: the authored one, lowered to the reach floor when that is
    // smaller. The floor is 0 when no enabled leg bounds it; skipped legs are enabled legs too wide to stride along X.
    float _CadenceSpeedRef = 0.0f;
    float _ReachCadenceFloor = 0.0f;
    int32 _ReachSkippedLegs = 0;
    int32 _MissedLandingLifts = 0;
    int32 _RaysLastSolve = 0;

public:
    CK_PROPERTY(_BodyTransform);
    CK_PROPERTY(_Clock);
    CK_PROPERTY(_CadenceScale);
    CK_PROPERTY(_CadenceSpeed);
    CK_PROPERTY(_Airborne);
    CK_PROPERTY(_RestTime);
    CK_PROPERTY(_Velocity);
    CK_PROPERTY(_SupportNormal);
    CK_PROPERTY(_CadenceSpeedRef);
    CK_PROPERTY(_ReachCadenceFloor);
    CK_PROPERTY(_ReachSkippedLegs);
    CK_PROPERTY(_MissedLandingLifts);
    CK_PROPERTY(_RaysLastSolve);
};

// --------------------------------------------------------------------------------------------------------------------

// _CandidateNormal and _CandidateSeen mirror the contact waiting out its confirmation: a turn beyond the confirm angle is
// pending while _CandidateSeen is above zero, and adopted once it reaches the confirm time.
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugMotion
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugMotion);

private:
    FVector _Velocity = FVector::ZeroVector;
    FVector _RequestedDirection = FVector::ZeroVector;
    float _RequestedSpeed = 0.0f;
    bool _Grounded = false;
    bool _TrustedContact = false;
    FCk_Time _MissingContact;
    ECk_SurfaceMotion_ContactSource _ContactSource = ECk_SurfaceMotion_ContactSource::None;
    FVector _CandidateNormal = FVector::UpVector;
    FCk_Time _CandidateSeen;

public:
    CK_PROPERTY(_Velocity);
    CK_PROPERTY(_RequestedDirection);
    CK_PROPERTY(_RequestedSpeed);
    CK_PROPERTY(_Grounded);
    CK_PROPERTY(_TrustedContact);
    CK_PROPERTY(_MissingContact);
    CK_PROPERTY(_ContactSource);
    CK_PROPERTY(_CandidateNormal);
    CK_PROPERTY(_CandidateSeen);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugBodyPose
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugBodyPose);

private:
    bool _Composed = false;
    ECk_ProceduralAnimation_Status _Status = ECk_ProceduralAnimation_Status::PendingSetup;
    FTransform _Offset = FTransform::Identity;
    bool _Conforms = false;
    FTransform _ConformTarget = FTransform::Identity;
    FTransform _AppliedConformTarget = FTransform::Identity;

public:
    CK_PROPERTY(_Composed);
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Offset);
    CK_PROPERTY(_Conforms);
    CK_PROPERTY(_ConformTarget);
    CK_PROPERTY(_AppliedConformTarget);
};

// --------------------------------------------------------------------------------------------------------------------

// Body, motion, probe and solver values belong to the last accepted advancing solve.
// Current status/freshness flags and actual rig transforms are overlaid at query time.
// RigPosePending marks parts whose deferred transform requests have not yet applied.
struct CKPROCEDURALANIMATION_API FCk_ProceduralAnimation_DebugSnapshot
{
    CK_GENERATED_BODY(FCk_ProceduralAnimation_DebugSnapshot);

private:
    FName _EntityName;
    FString _EntityId;
    FCk_ProceduralAnimation_DebugStatus _Status;
    FCk_ProceduralAnimation_DebugFreshness _Freshness;
    FCk_ProceduralAnimation_DebugSample _Sample;
    FCk_ProceduralAnimation_DebugGait _Gait;
    FCk_ProceduralAnimation_DebugMotion _Motion;
    FCk_ProceduralAnimation_DebugBodyPose _BodyPose;
    TArray<FCk_ProceduralAnimation_DebugLeg> _Legs;

public:
    CK_PROPERTY(_EntityName);
    CK_PROPERTY(_EntityId);
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Freshness);
    CK_PROPERTY(_Sample);
    CK_PROPERTY(_Gait);
    CK_PROPERTY(_Motion);
    CK_PROPERTY(_BodyPose);
    CK_PROPERTY(_Legs);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralAnimation_Debug_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralAnimation_Debug_UE);

public:
    // Game-thread only. No queries, simulation, asset loads or entity mutation.
    static auto
    Get_Snapshot(
        const FCk_Handle& InHandle)
        -> FCk_ProceduralAnimation_DebugSnapshot;

    // Discovery returns live handles; history must retain only copied snapshots.
    static auto
    Get_Entities(
        UWorld* InWorld)
        -> TArray<FCk_Handle>;

public:
    // Every ray the gait's last update cast: foothold candidates and their occlusion traces, planted-foot traces, and the
    // clearance and landing rays of swinging feet. 0 for an invalid handle or a gait that has not updated.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralAnimation|Debug",
              DisplayName="[Ck][ProceduralAnimation] Get Rays Last Solve")
    static int32
    Get_RaysLastSolve(
        const FCk_Handle_ProceduralGait& InGait);
};

// --------------------------------------------------------------------------------------------------------------------
