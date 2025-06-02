#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapeBox_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_ShapeBox : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ShapeBox); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ShapeBox);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_ShapeBox_Dimensions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ShapeBox_Dimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FVector _HalfExtents = FVector::OneVector;

    // Useful in some cases, example in physics engine use (Jolt) to improve collision detection
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    float _ConvexRadius = 0.0f;

public:
    CK_PROPERTY_GET(_HalfExtents);
    CK_PROPERTY(_ConvexRadius);

    CK_DEFINE_CONSTRUCTORS(FCk_ShapeBox_Dimensions, _HalfExtents);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeBox_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeBox_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_ShapeBox_Dimensions _InitialDimensions;

public:
    CK_PROPERTY_GET(_InitialDimensions);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ShapeBox_ParamsData, _InitialDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Request_ShapeBox_UpdateDimensions : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ShapeBox_UpdateDimensions);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ShapeBox_UpdateDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_ShapeBox_Dimensions _NewDimensions;

public:
    CK_PROPERTY_GET(_NewDimensions);
};

// --------------------------------------------------------------------------------------------------------------------