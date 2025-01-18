#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapeBox_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ShapeBox_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_ShapeBox : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ShapeBox); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ShapeBox);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeBox_ShapeData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeBox_ShapeData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FVector _Extent = FVector(1.0f, 1.0f, 1.0f);

    // useful in some cases, example in physics engine use (Jolt) to improve collision detection
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _ConvexRadius = 0.0f;

public:
    CK_PROPERTY_GET(_Extent);
    CK_PROPERTY_GET(_ConvexRadius);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeBox_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeBox_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Fragment_ShapeBox_ShapeData _Shape;

public:
    CK_PROPERTY_GET(_Shape);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Request_ShapeBox_UpdateShape : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_ShapeBox_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_ShapeBox_UpdateShape);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Fragment_ShapeBox_ShapeData _NewShape;

public:
    CK_PROPERTY_GET(_NewShape);
};

// --------------------------------------------------------------------------------------------------------------------