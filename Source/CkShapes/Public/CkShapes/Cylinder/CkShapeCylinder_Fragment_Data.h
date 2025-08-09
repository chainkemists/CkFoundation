#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapeCylinder_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_ShapeCylinder : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ShapeCylinder); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ShapeCylinder);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_ShapeCylinder_Dimensions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ShapeCylinder_Dimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    float _HalfHeight = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    float _Radius = 50.0f;

    // Useful in some cases, example in physics engine use (Jolt) to improve collision detection
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    float _ConvexRadius = 0.0f;

public:
    CK_PROPERTY_GET(_HalfHeight);
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY(_ConvexRadius);

    CK_DEFINE_CONSTRUCTORS(FCk_ShapeCylinder_Dimensions, _HalfHeight, _Radius);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeCylinder_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeCylinder_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_ShapeCylinder_Dimensions _InitialDimensions;

public:
    CK_PROPERTY_GET(_InitialDimensions);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ShapeCylinder_ParamsData, _InitialDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Request_ShapeCylinder_UpdateDimensions : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ShapeCylinder_UpdateDimensions);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ShapeCylinder_UpdateDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_ShapeCylinder_Dimensions _NewDimensions;

public:
    CK_PROPERTY_GET(_NewDimensions);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_ShapeCylinder_UpdateDimensions, _NewDimensions);
};

// --------------------------------------------------------------------------------------------------------------------
