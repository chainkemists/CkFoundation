#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkIsmRenderer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKISMRENDERER_API FCk_Handle_IsmRenderer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IsmRenderer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IsmRenderer);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Fragment_IsmRenderer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IsmRenderer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Collision _Collision = ECk_Collision::NoCollision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CastShadows = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _MeshScale = FVector::OneVector;

public:
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_Collision);
    CK_PROPERTY_GET(_CastShadows);
    CK_PROPERTY_GET(_MeshScale);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmRenderer_NewInstance
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmRenderer_NewInstance);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FTransform _Transform;

public:
    CK_PROPERTY(_Transform);
};

