#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkVelocity_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_Velocity_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Velocity_ParamsData);

public:
    FCk_Fragment_Velocity_ParamsData() = default;
    FCk_Fragment_Velocity_ParamsData(
        ECk_LocalWorld InCoordinates,
        FVector InStartingVelocity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _Coordinates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartingVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Coordinates);
    CK_PROPERTY_GET(_StartingVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_VelocityModifier_SingleTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VelocityModifier_SingleTarget_ParamsData);

public:
    FCk_Fragment_VelocityModifier_SingleTarget_ParamsData() = default;
    FCk_Fragment_VelocityModifier_SingleTarget_ParamsData(
        FCk_Fragment_Velocity_ParamsData InVelocityParams,
        FCk_Handle InTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Velocity_ParamsData _VelocityParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Target;

public:
    CK_PROPERTY_GET(_VelocityParams);
    CK_PROPERTY_GET(_Target);
};

// --------------------------------------------------------------------------------------------------------------------
