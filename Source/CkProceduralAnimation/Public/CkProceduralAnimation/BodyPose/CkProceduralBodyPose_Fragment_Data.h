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
    MalformedSupport,
    MalformedConform
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralBodyPose_Failure);

// --------------------------------------------------------------------------------------------------------------------

// Engine spring-interpolator parameters (VectorSpringInterp / QuaternionSpringInterp). MaxAttitudeLag, in degrees, limits
// how far carrying the body's tilt steps may hold the drawn body behind the rotation its spring follows; 0 carries none.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralBodyPose_Spring
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralBodyPose_Spring);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.01))
    float _Stiffness = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _CriticalDampingFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.01))
    float _Mass = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MaxAttitudeLag = 22.0f;

public:
    CK_PROPERTY(_Stiffness);
    CK_PROPERTY(_CriticalDampingFactor);
    CK_PROPERTY(_Mass);
    CK_PROPERTY(_MaxAttitudeLag);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralBodyPose_Spring, IsValid_Policy_Default,
[=](const FCk_ProceduralBodyPose_Spring& InSpring)
{
    return FMath::IsFinite(InSpring.Get_Stiffness()) && InSpring.Get_Stiffness() > 0.0f
        && FMath::IsFinite(InSpring.Get_CriticalDampingFactor()) && InSpring.Get_CriticalDampingFactor() >= 0.0f
        && FMath::IsFinite(InSpring.Get_Mass()) && InSpring.Get_Mass() > 0.0f
        && FMath::IsFinite(InSpring.Get_MaxAttitudeLag()) && InSpring.Get_MaxAttitudeLag() >= 0.0f;
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

UENUM(BlueprintType)
enum class ECk_ProceduralBodyPose_ConformMode : uint8
{
    None,
    PlantedFeet
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralBodyPose_ConformMode);

// --------------------------------------------------------------------------------------------------------------------

// PlantedFeet tilts the drawn body, up to MaxTilt degrees, toward a plane fitted through its feet in the body's frame, and
// moves it along its up by HeightWeight times that plane's height under the body, up to MaxHeight centimetres. The target
// the springs chase turns toward that fit at no more than MaxTiltRate degrees per second and moves at no more than
// MaxHeightRate centimetres per second. The critically damped rotation spring trails a target turning at a steady rate by
// about twice the rate over its natural frequency sqrt(Stiffness / Mass), so keep 2 * MaxTiltRate / sqrt(Stiffness / Mass)
// within the spring's MaxAttitudeLag: with the default spring, 60 degrees per second trails by about 19 degrees against 22.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralBodyPose_Conform
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralBodyPose_Conform);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProceduralBodyPose_ConformMode _Mode = ECk_ProceduralBodyPose_ConformMode::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 89))
    float _MaxTilt = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 1))
    float _HeightWeight = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MaxHeight = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.01))
    float _MaxTiltRate = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.01))
    float _MaxHeightRate = 60.0f;

public:
    CK_PROPERTY(_Mode);
    CK_PROPERTY(_MaxTilt);
    CK_PROPERTY(_HeightWeight);
    CK_PROPERTY(_MaxHeight);
    CK_PROPERTY(_MaxTiltRate);
    CK_PROPERTY(_MaxHeightRate);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralBodyPose_Conform, IsValid_Policy_Default,
[=](const FCk_ProceduralBodyPose_Conform& InConform)
{
    return FMath::IsFinite(InConform.Get_MaxTilt()) && InConform.Get_MaxTilt() >= 0.0f && InConform.Get_MaxTilt() <= 89.0f
        && FMath::IsFinite(InConform.Get_HeightWeight()) && InConform.Get_HeightWeight() >= 0.0f && InConform.Get_HeightWeight() <= 1.0f
        && FMath::IsFinite(InConform.Get_MaxHeight()) && InConform.Get_MaxHeight() >= 0.0f
        && FMath::IsFinite(InConform.Get_MaxTiltRate()) && InConform.Get_MaxTiltRate() > 0.0f
        && FMath::IsFinite(InConform.Get_MaxHeightRate()) && InConform.Get_MaxHeightRate() > 0.0f;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralBodyPose_Conform _Conform;

public:
    CK_PROPERTY_GET(_Presentation);
    CK_PROPERTY(_Spring);
    CK_PROPERTY(_Support);
    CK_PROPERTY(_Conform);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralBodyPose_Spec, _Presentation);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralBodyPose_Spec, IsValid_Policy_Default,
[=](const FCk_ProceduralBodyPose_Spec& InParams)
{
    return ck::IsValid(InParams.Get_Presentation())
        && ck::IsValid(InParams.Get_Spring())
        && ck::IsValid(InParams.Get_Support())
        && ck::IsValid(InParams.Get_Conform());
});

// --------------------------------------------------------------------------------------------------------------------
