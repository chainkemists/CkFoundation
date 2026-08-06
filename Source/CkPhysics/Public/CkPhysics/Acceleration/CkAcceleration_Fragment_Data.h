#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkAcceleration_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPHYSICS_API FCk_Handle_Acceleration : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Acceleration); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Acceleration);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Acceleration_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Acceleration_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _Coordinates = ECk_LocalWorld::Local;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartingAcceleration = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Coordinates);
    CK_PROPERTY_GET(_StartingAcceleration);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Acceleration_Spec, _Coordinates, _StartingAcceleration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_AccelerationModifier_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AccelerationModifier_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Acceleration_Spec _AccelerationParams;

public:
    CK_PROPERTY_GET(_AccelerationParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AccelerationModifier_Spec, _AccelerationParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_BulkAccelerationModifier_Spec
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_BulkAccelerationModifier_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Acceleration_Spec _AccelerationParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TargetChannels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ExtentScope _ModifierScope = ECk_ExtentScope::Bounded;

public:
    CK_PROPERTY_GET(_AccelerationParams);
    CK_PROPERTY_GET(_TargetChannels);
    CK_PROPERTY_GET(_ModifierScope);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_BulkAccelerationModifier_Spec, _AccelerationParams, _TargetChannels, _ModifierScope);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Request_BulkAccelerationModifier_AddTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_BulkAccelerationModifier_AddTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_BulkAccelerationModifier_AddTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TargetEntity;

public:
    CK_PROPERTY_GET(_TargetEntity)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_BulkAccelerationModifier_AddTarget, _TargetEntity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Request_BulkAccelerationModifier_RemoveTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_BulkAccelerationModifier_RemoveTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_BulkAccelerationModifier_RemoveTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TargetEntity;

public:
    CK_PROPERTY_GET(_TargetEntity)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_BulkAccelerationModifier_RemoveTarget, _TargetEntity);
};

// --------------------------------------------------------------------------------------------------------------------
