#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapeSphere_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ShapeSphere_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_ShapeSphere : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ShapeSphere); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ShapeSphere);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeSphere_ShapeData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeSphere_ShapeData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _Radius;

public:
    CK_PROPERTY_GET(_Radius);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeSphere_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeSphere_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Fragment_ShapeSphere_ShapeData _Shape;

public:
    CK_PROPERTY_GET(_Shape);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Request_ShapeSphere_UpdateShape : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_ShapeSphere_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_ShapeSphere_UpdateShape);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Fragment_ShapeSphere_ShapeData _NewShape;

public:
    CK_PROPERTY_GET(_NewShape);
};

// --------------------------------------------------------------------------------------------------------------------
