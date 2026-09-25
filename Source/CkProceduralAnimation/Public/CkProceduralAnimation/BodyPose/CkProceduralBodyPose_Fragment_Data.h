#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralBodyPose_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_ProceduralBodyPose : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProceduralBodyPose);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProceduralBodyPose);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralBodyPose_Failure : uint8
{
    None,
    MissingPresentation,
    MalformedSupport
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralBodyPose_Failure);

// --------------------------------------------------------------------------------------------------------------------

// Engine spring-interpolator parameters (VectorSpringInterp / QuaternionSpringInterp).
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralBodyPose_Spring
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralBodyPose_Spring);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _Stiffness = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _CriticalDampingFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _Mass = 1.0f;

public:
    CK_PROPERTY(_Stiffness);
    CK_PROPERTY(_CriticalDampingFactor);
    CK_PROPERTY(_Mass);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralBodyPose_Spring, IsValid_Policy_Default,
[=](const FCk_ProceduralBodyPose_Spring& InSpring)
{
    return FMath::IsFinite(InSpring.Get_Stiffness()) && InSpring.Get_Stiffness() > 0.0f
        && FMath::IsFinite(InSpring.Get_CriticalDampingFactor()) && InSpring.Get_CriticalDampingFactor() >= 0.0f
        && FMath::IsFinite(InSpring.Get_Mass()) && InSpring.Get_Mass() > 0.0f;
});

// --------------------------------------------------------------------------------------------------------------------

// How far the body drops, in centimetres, with no supporting leg, and how far it tilts, in degrees, toward a side
// that has lost all of its support.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralBodyPose_Support
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralBodyPose_Support);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _CollapseDrop = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 89))
    float _MaxTilt = 25.0f;

public:
    CK_PROPERTY(_CollapseDrop);
    CK_PROPERTY(_MaxTilt);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralBodyPose_Support, IsValid_Policy_Default,
[=](const FCk_ProceduralBodyPose_Support& InSupport)
{
    return FMath::IsFinite(InSupport.Get_CollapseDrop()) && InSupport.Get_CollapseDrop() >= 0.0f
        && FMath::IsFinite(InSupport.Get_MaxTilt()) && InSupport.Get_MaxTilt() >= 0.0f
        && InSupport.Get_MaxTilt() <= 89.0f;
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralBodyPose_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralBodyPose_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Presentation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralBodyPose_Spring _Spring;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralBodyPose_Support _Support;

public:
    CK_PROPERTY_GET(_Presentation);
    CK_PROPERTY(_Spring);
    CK_PROPERTY(_Support);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralBodyPose_Spec, _Presentation);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralBodyPose_Spec, IsValid_Policy_Default,
[=](const FCk_ProceduralBodyPose_Spec& InParams)
{
    return ck::IsValid(InParams.Get_Presentation())
        && ck::IsValid(InParams.Get_Spring())
        && ck::IsValid(InParams.Get_Support());
});

// --------------------------------------------------------------------------------------------------------------------
