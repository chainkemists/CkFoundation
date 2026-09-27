#pragma once

#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkJolt/Query/CkJoltQuery_Data.h"

#include "CkProceduralGait_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_ProceduralGait : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProceduralGait);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProceduralGait);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralGait_LegLossPolicy : uint8
{
    KeepAuthoredOffsets  UMETA(DisplayName = "Keep Authored Offsets (survivors keep their phase)"),
    RedistributeOffsets  UMETA(DisplayName = "Redistribute Offsets (survivors re-space evenly, blended)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralGait_LegLossPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralGait_Failure : uint8
{
    None,
    NanBody,
    DisabledPoseSync,
    ProbeAdvance,
    SolverReset,
    SolverStep
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralGait_Failure);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_Timing
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralGait_Timing);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _CycleDuration = FCk_Time{0.8};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _StepDuration = FCk_Time{0.22};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _CadenceSpeedRef = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _MaxCadenceScale = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _MaxSimultaneousSwings = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProceduralGait_LegLossPolicy _LegLossPolicy = ECk_ProceduralGait_LegLossPolicy::KeepAuthoredOffsets;

public:
    CK_PROPERTY(_CycleDuration);
    CK_PROPERTY(_StepDuration);
    CK_PROPERTY(_CadenceSpeedRef);
    CK_PROPERTY(_MaxCadenceScale);
    CK_PROPERTY(_MaxSimultaneousSwings);
    CK_PROPERTY(_LegLossPolicy);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralGait_Timing, IsValid_Policy_Default,
[=](const FCk_ProceduralGait_Timing& InTiming)
{
    return FMath::IsFinite(InTiming.Get_CycleDuration().Get_Seconds()) && InTiming.Get_CycleDuration() > FCk_Time{}
        && FMath::IsFinite(InTiming.Get_StepDuration().Get_Seconds()) && InTiming.Get_StepDuration() > FCk_Time{}
        && FMath::IsFinite(InTiming.Get_CadenceSpeedRef()) && InTiming.Get_CadenceSpeedRef() >= 0.0f
        && FMath::IsFinite(InTiming.Get_MaxCadenceScale()) && InTiming.Get_MaxCadenceScale() >= 1.0f
        && InTiming.Get_MaxSimultaneousSwings() >= 0;
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_Step
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralGait_Step);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Height = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Threshold = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ObstacleClearance = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _MaxVelocityLead = 60.0f;

    // Swing targets stay within this fraction of the leg's chain length from its hip, and every leg's rest foot must
    // lie within it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 1))
    float _TargetReachFraction = 0.8f;

    // A planted foot farther than this fraction of the chain length from its hip steps as an Emergency.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 1))
    float _ForceStepReachFraction = 0.92f;

    // A planted foot farther than this fraction of the chain length from its hip may step while another phase group
    // swings, so a body climbing away from its planted feet does not stretch them past their chains. It may equal the
    // force-step fraction; then every reach Emergency also steps beyond the schedule.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, ClampMax = 1.5))
    float _HardOverstretchReachFraction = 1.0f;

public:
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Threshold);
    CK_PROPERTY(_ObstacleClearance);
    CK_PROPERTY(_MaxVelocityLead);
    CK_PROPERTY(_TargetReachFraction);
    CK_PROPERTY(_ForceStepReachFraction);
    CK_PROPERTY(_HardOverstretchReachFraction);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralGait_Step, IsValid_Policy_Default,
[=](const FCk_ProceduralGait_Step& InStep)
{
    return FMath::IsFinite(InStep.Get_Height()) && InStep.Get_Height() >= 0.0f
        && FMath::IsFinite(InStep.Get_Threshold()) && InStep.Get_Threshold() > 0.0f
        && FMath::IsFinite(InStep.Get_ObstacleClearance()) && InStep.Get_ObstacleClearance() >= 0.0f
        && FMath::IsFinite(InStep.Get_MaxVelocityLead()) && InStep.Get_MaxVelocityLead() >= 0.0f
        && FMath::IsFinite(InStep.Get_TargetReachFraction()) && FMath::IsFinite(InStep.Get_ForceStepReachFraction())
        && InStep.Get_TargetReachFraction() > 0.0f
        && InStep.Get_TargetReachFraction() < InStep.Get_ForceStepReachFraction()
        && InStep.Get_ForceStepReachFraction() <= 1.0f
        && FMath::IsFinite(InStep.Get_HardOverstretchReachFraction())
        && InStep.Get_HardOverstretchReachFraction() >= 1.0f && InStep.Get_HardOverstretchReachFraction() <= 1.5f
        && InStep.Get_HardOverstretchReachFraction() >= InStep.Get_ForceStepReachFraction();
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_Probe
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralGait_Probe);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Up = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Down = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _OutwardLean = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ContactGrace = FCk_Time{0.12};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Jolt_QueryFilter _QueryFilter;

public:
    CK_PROPERTY(_Up);
    CK_PROPERTY(_Down);
    CK_PROPERTY(_OutwardLean);
    CK_PROPERTY(_ContactGrace);
    CK_PROPERTY(_QueryFilter);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralGait_Probe, IsValid_Policy_Default,
[=](const FCk_ProceduralGait_Probe& InProbe)
{
    return FMath::IsFinite(InProbe.Get_Up()) && InProbe.Get_Up() > 0.0f
        && FMath::IsFinite(InProbe.Get_Down()) && InProbe.Get_Down() > 0.0f
        && FMath::IsFinite(InProbe.Get_OutwardLean()) && InProbe.Get_OutwardLean() >= 0.0f
        && InProbe.Get_OutwardLean() <= 1.0f
        && FMath::IsFinite(InProbe.Get_ContactGrace().Get_Seconds()) && InProbe.Get_ContactGrace() >= FCk_Time{};
});

// --------------------------------------------------------------------------------------------------------------------

// The foothold search that runs when a leg's ideal target is missed, out of reach or occluded. The two radii derive
// from the leg when 0: the search ring from 0.3 of its reach, the keep radius of a held foothold from 1.5 step
// thresholds.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_Foothold
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralGait_Foothold);

private:
    // Centimetres from the ideal target to the eight ring candidates.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _SearchRadius = 0.0f;

    // Centimetres a held foothold may lie from the ideal target before the search runs again.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _KeepRadius = 0.0f;

    // Degrees from the support up; applies to search candidates only, the ideal target and a held foothold accept any
    // surface they do not see from behind.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 90))
    float _MaxAngle = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _SlopeWeight = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ContinuityWeight = 0.25f;

    // Centimetres: a hip-to-foothold trace that hits a solid farther than this from the foothold is occluded.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _OcclusionTolerance = 3.0f;

public:
    CK_PROPERTY(_SearchRadius);
    CK_PROPERTY(_KeepRadius);
    CK_PROPERTY(_MaxAngle);
    CK_PROPERTY(_SlopeWeight);
    CK_PROPERTY(_ContinuityWeight);
    CK_PROPERTY(_OcclusionTolerance);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralGait_Foothold, IsValid_Policy_Default,
[=](const FCk_ProceduralGait_Foothold& InFoothold)
{
    return FMath::IsFinite(InFoothold.Get_SearchRadius()) && InFoothold.Get_SearchRadius() >= 0.0f
        && FMath::IsFinite(InFoothold.Get_KeepRadius()) && InFoothold.Get_KeepRadius() >= 0.0f
        && FMath::IsFinite(InFoothold.Get_MaxAngle()) && InFoothold.Get_MaxAngle() >= 0.0f && InFoothold.Get_MaxAngle() <= 90.0f
        && FMath::IsFinite(InFoothold.Get_SlopeWeight()) && InFoothold.Get_SlopeWeight() >= 0.0f
        && FMath::IsFinite(InFoothold.Get_ContinuityWeight()) && InFoothold.Get_ContinuityWeight() >= 0.0f
        && FMath::IsFinite(InFoothold.Get_OcclusionTolerance()) && InFoothold.Get_OcclusionTolerance() >= 0.0f;
});

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKPROCEDURALANIMATION_API UCk_ProceduralGait_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ProceduralGait_Data);

protected:
#if WITH_EDITOR
    auto
    IsDataValid(class FDataValidationContext& InContext) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Timing _Timing;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Step _Step;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Probe _Probe;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Foothold _Foothold;

public:
    CK_PROPERTY(_Timing);
    CK_PROPERTY(_Step);
    CK_PROPERTY(_Probe);
    CK_PROPERTY(_Foothold);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Request_ProceduralGait_ApplyPreset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ProceduralGait_ApplyPreset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ProceduralGait_ApplyPreset);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Timing _Timing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Step _Step;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Probe _Probe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralGait_Foothold _Foothold;

public:
    CK_PROPERTY_GET(_Timing);
    CK_PROPERTY_GET(_Step);
    CK_PROPERTY_GET(_Probe);
    CK_PROPERTY_GET(_Foothold);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ProceduralGait_ApplyPreset, _Timing, _Step, _Probe, _Foothold);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_ProceduralGait_OnLegSetChanged,
    FCk_Handle_ProceduralGait, InGait,
    int32, InEnabledCount,
    int32, InTotalCount);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralGait_FeetPlane : uint8
{
    None    UMETA(DisplayName = "None (no plane the body can ride)"),
    Fitted  UMETA(DisplayName = "Fitted (through this solve's supporting feet)"),
    Held    UMETA(DisplayName = "Held (the last fitted plane, while too few feet support the body)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralGait_FeetPlane);

// The plane the gait's last solve fitted through the supporting feet, in world space: a point on it and its unit normal.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_FeetPlane
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralGait_FeetPlane);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _Point = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _Normal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_ProceduralGait_FeetPlane _State = ECk_ProceduralGait_FeetPlane::None;

public:
    CK_PROPERTY(_Point);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_State);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    enum class EProceduralGaitFeetPlane : uint8
    {
        None,
        Fitted,
        Held
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The solver settings built from a gait's tunables and its enabled legs, with the reach cadence floor the build applied
    // (0 when no enabled leg bounds it) and the count of enabled legs too wide to stride along body X. The floor and the
    // count are diagnostics: only the debug fragment keeps them.
    struct CKPROCEDURALANIMATION_API FProceduralGaitBuiltSettings
    {
    public:
        CK_GENERATED_BODY(FProceduralGaitBuiltSettings);

    private:
        FProceduralGaitSettings _Settings;
        float _ReachCadenceFloor = 0.0f;
        int32 _ReachSkippedLegs = 0;

    public:
        CK_PROPERTY_GET(_Settings);
        CK_PROPERTY_GET(_ReachCadenceFloor);
        CK_PROPERTY_GET(_ReachSkippedLegs);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralGaitBuiltSettings, _Settings, _ReachCadenceFloor, _ReachSkippedLegs);
    };
}

// --------------------------------------------------------------------------------------------------------------------
