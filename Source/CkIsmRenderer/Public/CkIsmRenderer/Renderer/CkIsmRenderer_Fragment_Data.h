#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkGraphics/CkGraphics_Common.h"

#include <Engine/StaticMesh.h>

#include "CkIsmRenderer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKISMRENDERER_API FCk_Handle_IsmRenderer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IsmRenderer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IsmRenderer);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ism_RenderPolicy : uint8
{
    ISM UMETA (DisplayName = "Instanced Static Mesh"),
    HISM UMETA(DisplayName = "Hierarchical Instanced Static Mesh")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ism_RenderPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_IsmRenderer_CullingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IsmRenderer_CullingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _InstanceCullDistance_Start = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _InstanceCullDistance_End = 0.0f;

public:
    CK_PROPERTY_GET(_InstanceCullDistance_Start);
    CK_PROPERTY_GET(_InstanceCullDistance_End);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_IsmRenderer_PhysicsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IsmRenderer_PhysicsInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Collision _Collision = ECk_Collision::NoCollision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_Collision != ECk_Collision::NoCollision"))
    FCollisionProfileName _CollisionProfileName;

public:
    CK_PROPERTY_GET(_Collision);
    CK_PROPERTY_GET(_CollisionProfileName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_IsmRenderer_LightingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IsmRenderer_LightingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CastShadows = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_CastShadows);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_IsmRenderer_MaterialsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IsmRenderer_MaterialsInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "Slot #{_MaterialSlot}: {_ReplacementMaterial}"))
    TArray<FCk_MeshMaterialOverride> _MaterialOverrides;

public:
    CK_PROPERTY_GET(_MaterialOverrides);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Fragment_IsmRenderer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IsmRenderer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ism.Renderer"))
    FGameplayTag _RendererName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ism_RenderPolicy _RenderPolicy = ECk_Ism_RenderPolicy::ISM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_MaterialsInfo _MaterialsInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_PhysicsInfo _PhysicsInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_LightingInfo _LightingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_CullingInfo _CullingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _NumCustomData = 0;

public:
    CK_PROPERTY_GET(_RendererName);
    CK_PROPERTY_GET(_RenderPolicy);
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_MaterialsInfo);
    CK_PROPERTY_GET(_PhysicsInfo);
    CK_PROPERTY_GET(_LightingInfo);
    CK_PROPERTY_GET(_CullingInfo);
    CK_PROPERTY_GET(_NumCustomData);
};

// --------------------------------------------------------------------------------------------------------------------
