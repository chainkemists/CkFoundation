#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkTransform_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECSEXT_API FCk_Handle_Transform : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Transform); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Transform);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECSEXT_API FCk_Handle_TransformInterpolation : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_TransformInterpolation); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_TransformInterpolation);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Transform_Interpolation_Settings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Transform_Interpolation_Settings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess, ValidEnumValues="Linear"))
    ECk_Interpolation_Strategy _Strategy = ECk_Interpolation_Strategy::Linear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _MaxSmoothUpdateDistance = 256.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _NoSmoothUpdateDistance = 384.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Time _SmoothLocationTime = FCk_Time{0.1f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Time _SmoothRotationTime = FCk_Time{0.05f};

public:
    CK_PROPERTY(_Strategy);
    CK_PROPERTY(_MaxSmoothUpdateDistance);
    CK_PROPERTY(_NoSmoothUpdateDistance);
    CK_PROPERTY(_SmoothLocationTime);
    CK_PROPERTY(_SmoothRotationTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Transform_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Transform_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Transform_Interpolation_Settings _InterpolationSettings;

public:
    CK_PROPERTY(_InterpolationSettings);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_TransformInterpolation_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TransformInterpolation_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Transform_Interpolation_Settings _InterpolationSettings;

public:
    CK_PROPERTY_GET(_InterpolationSettings);

    CK_DEFINE_CONSTRUCTORS(FCk_TransformInterpolation_ParamsData, _InterpolationSettings);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_Transform_SetLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NewLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_NewLocation)
    CK_PROPERTY(_LocalWorld)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Transform_SetLocation, _NewLocation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_Transform_AddLocationOffset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_AddLocationOffset);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _DeltaLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_DeltaLocation)
    CK_PROPERTY(_LocalWorld)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Transform_AddLocationOffset, _DeltaLocation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_Transform_SetRotation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetRotation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _NewRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_NewRotation)
    CK_PROPERTY(_LocalWorld)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Transform_SetRotation, _NewRotation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_Transform_AddRotationOffset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_AddRotationOffset);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _DeltaRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_DeltaRotation)
    CK_PROPERTY(_LocalWorld)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Transform_AddRotationOffset, _DeltaRotation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_Transform_SetScale
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetScale);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NewScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_NewScale)
    CK_PROPERTY(_LocalWorld)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Transform_SetScale, _NewScale);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSEXT_API FCk_Request_Transform_SetTransform
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetTransform);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _NewTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewTransform)
    CK_PROPERTY(_RelativeAbsolute)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Transform_SetTransform, _NewTransform);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Transform_OnUpdate,
    FCk_Handle, InHandle,
    FTransform, InTransform);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Transform_OnUpdate_MC,
    FCk_Handle, InHandle,
    FTransform, InTransform);

// --------------------------------------------------------------------------------------------------------------------

// TODO: Add AddOffset request variations
