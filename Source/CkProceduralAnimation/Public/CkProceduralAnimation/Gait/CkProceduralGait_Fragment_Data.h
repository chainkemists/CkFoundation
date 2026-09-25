#pragma once

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

public:
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Threshold);
    CK_PROPERTY(_ObstacleClearance);
    CK_PROPERTY(_MaxVelocityLead);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralGait_Step, IsValid_Policy_Default,
[=](const FCk_ProceduralGait_Step& InStep)
{
    return FMath::IsFinite(InStep.Get_Height()) && InStep.Get_Height() >= 0.0f
        && FMath::IsFinite(InStep.Get_Threshold()) && InStep.Get_Threshold() > 0.0f
        && FMath::IsFinite(InStep.Get_ObstacleClearance()) && InStep.Get_ObstacleClearance() >= 0.0f
        && FMath::IsFinite(InStep.Get_MaxVelocityLead()) && InStep.Get_MaxVelocityLead() >= 0.0f;
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

public:
    CK_PROPERTY(_Timing);
    CK_PROPERTY(_Step);
    CK_PROPERTY(_Probe);
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

public:
    CK_PROPERTY_GET(_Timing);
    CK_PROPERTY_GET(_Step);
    CK_PROPERTY_GET(_Probe);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ProceduralGait_ApplyPreset, _Timing, _Step, _Probe);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_ProceduralGait_OnLegSetChanged,
    FCk_Handle_ProceduralGait, InGait,
    int32, InEnabledCount,
    int32, InTotalCount);

// --------------------------------------------------------------------------------------------------------------------
