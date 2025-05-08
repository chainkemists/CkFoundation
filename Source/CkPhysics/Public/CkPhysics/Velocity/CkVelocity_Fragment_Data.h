#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "GameplayTagContainer.h"

#include "CkVelocity_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPHYSICS_API FCk_Handle_Velocity : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Velocity); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Velocity);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_Velocity_MinMax_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Velocity_MinMax_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasMinSpeed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasMinSpeed"))
    float _MinSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasMaxSpeed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasMaxSpeed"))
    float _MaxSpeed = 0.0f;

public:
    CK_PROPERTY_GET(_HasMinSpeed);
    CK_PROPERTY_GET(_MinSpeed);
    CK_PROPERTY_GET(_HasMaxSpeed);
    CK_PROPERTY_GET(_MaxSpeed);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Velocity_MinMax_ParamsData, _HasMinSpeed, _MinSpeed, _HasMaxSpeed, _MaxSpeed);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_Velocity_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Velocity_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _Coordinates = ECk_LocalWorld::Local;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartingVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Velocity_MinMax_ParamsData _VelocityMinMax;

public:
    CK_PROPERTY_GET(_Coordinates);
    CK_PROPERTY_GET(_StartingVelocity);
    CK_PROPERTY_GET(_VelocityMinMax);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Velocity_ParamsData, _Coordinates, _StartingVelocity, _VelocityMinMax);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_VelocityModifier_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VelocityModifier_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Velocity_ParamsData _VelocityParams;

public:
    CK_PROPERTY_GET(_VelocityParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VelocityModifier_ParamsData, _VelocityParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_BulkVelocityModifier_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VelocityModifier_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Velocity_ParamsData _VelocityParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TargetChannels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ExtentScope _ModifierScope = ECk_ExtentScope::Bounded;

public:
    CK_PROPERTY_GET(_VelocityParams);
    CK_PROPERTY_GET(_TargetChannels);
    CK_PROPERTY_GET(_ModifierScope);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_BulkVelocityModifier_ParamsData, _VelocityParams, _TargetChannels, _ModifierScope);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Request_BulkVelocityModifier_AddTarget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_BulkVelocityModifier_AddTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TargetEntity;

public:
    CK_PROPERTY_GET(_TargetEntity)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_BulkVelocityModifier_AddTarget, _TargetEntity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Request_BulkVelocityModifier_RemoveTarget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_BulkVelocityModifier_RemoveTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TargetEntity;

public:
    CK_PROPERTY_GET(_TargetEntity)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_BulkVelocityModifier_RemoveTarget, _TargetEntity);
};

// --------------------------------------------------------------------------------------------------------------------
