#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkAcceleration_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_Acceleration_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Acceleration_ParamsData);

public:
    FCk_Fragment_Acceleration_ParamsData() = default;
    FCk_Fragment_Acceleration_ParamsData(
        ECk_LocalWorld InCoordinates,
        FVector InStartingAcceleration);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _Coordinates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartingAcceleration = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Coordinates);
    CK_PROPERTY_GET(_StartingAcceleration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData);

public:
    FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData() = default;
    explicit FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData(
        FCk_Fragment_Acceleration_ParamsData InAccelerationParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Acceleration_ParamsData _AccelerationParams;

public:
    CK_PROPERTY_GET(_AccelerationParams);
};

// --------------------------------------------------------------------------------------------------------------------
