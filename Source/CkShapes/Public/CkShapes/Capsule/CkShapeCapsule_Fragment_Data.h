#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapeCapsule_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ShapeCapsule_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_ShapeCapsule : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ShapeCapsule); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ShapeCapsule);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeCapsule_ShapeData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeCapsule_ShapeData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _HalfHeight = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    float _Radius = 50.0f;

public:
    CK_PROPERTY_GET(_HalfHeight);
    CK_PROPERTY_GET(_Radius);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ShapeCapsule_ShapeData, _HalfHeight, _Radius);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeCapsule_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeCapsule_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Fragment_ShapeCapsule_ShapeData _Shape;

public:
    CK_PROPERTY_GET(_Shape);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Request_ShapeCapsule_UpdateShape : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_ShapeCapsule_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_ShapeCapsule_UpdateShape);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FCk_Fragment_ShapeCapsule_ShapeData _NewShape;

public:
    CK_PROPERTY_GET(_NewShape);
};

// --------------------------------------------------------------------------------------------------------------------
