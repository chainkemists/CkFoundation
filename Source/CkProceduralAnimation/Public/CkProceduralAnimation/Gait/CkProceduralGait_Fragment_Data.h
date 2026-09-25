#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"

#include "CkProceduralGait_Fragment_Data.generated.h"

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_ProceduralGait : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProceduralGait);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProceduralGait);

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_Leg
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_ProceduralGait_Leg);

private:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _HipLocal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _RestFootLocal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 1))
    float _PhaseOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _StepThresholdScale = 1.0f;
public:
    CK_PROPERTY(_Id);
    CK_PROPERTY(_HipLocal);
    CK_PROPERTY(_RestFootLocal);
    CK_PROPERTY(_PhaseOffset);
    CK_PROPERTY(_StepThresholdScale);
};

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Fragment_ProceduralGait_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_ProceduralGait_ParamsData);

private:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, TitleProperty = "_Id"))
    TArray<FCk_ProceduralGait_Leg> _Legs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _CycleDuration = FCk_Time{0.8};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _StepDuration = FCk_Time{0.22};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _StepHeight = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _StepThreshold = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _MaxSimultaneousSwings = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _CadenceSpeedRef = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _MaxCadenceScale = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ObstacleClearance = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _OutwardProbeLean = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _MaxVelocityLead = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ProbeUp = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ProbeDown = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _ContactGrace = FCk_Time{0.12};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Jolt_QueryFilter _QueryFilter;
public:
    CK_PROPERTY(_Legs);
    CK_PROPERTY(_CycleDuration);
    CK_PROPERTY(_StepDuration);
    CK_PROPERTY(_StepHeight);
    CK_PROPERTY(_StepThreshold);
    CK_PROPERTY(_MaxSimultaneousSwings);
    CK_PROPERTY(_CadenceSpeedRef);
    CK_PROPERTY(_MaxCadenceScale);
    CK_PROPERTY(_ObstacleClearance);
    CK_PROPERTY(_OutwardProbeLean);
    CK_PROPERTY(_MaxVelocityLead);
    CK_PROPERTY(_ProbeUp);
    CK_PROPERTY(_ProbeDown);
    CK_PROPERTY(_ContactGrace);
    CK_PROPERTY(_QueryFilter);
};

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralGait_Foot
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_ProceduralGait_Foot);

private:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _Id;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Position = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Normal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FQuat _Rotation = FQuat::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _SwingAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _Planted = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _ContactTrusted = false;
public:
    CK_PROPERTY(_Id);
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_SwingAlpha);
    CK_PROPERTY(_Planted);
    CK_PROPERTY(_ContactTrusted);
};
