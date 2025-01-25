#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkShapeCapsule_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSHAPES_API FCk_Handle_ShapeCapsule : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ShapeCapsule); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ShapeCapsule);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_ShapeCapsule_Dimensions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ShapeCapsule_Dimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    float _HalfHeight = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    float _Radius = 50.0f;

public:
    CK_PROPERTY_GET(_HalfHeight);
    CK_PROPERTY_GET(_Radius);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Fragment_ShapeCapsule_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ShapeCapsule_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_ShapeCapsule_Dimensions _InitialDimensions;

public:
    CK_PROPERTY_GET(_InitialDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSHAPES_API FCk_Request_ShapeCapsule_UpdateDimensions : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ShapeCapsule_UpdateDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_ShapeCapsule_Dimensions _NewDimensions;

public:
    CK_PROPERTY_GET(_NewDimensions);
};

// --------------------------------------------------------------------------------------------------------------------