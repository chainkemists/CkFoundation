#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

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

UENUM(BlueprintType)
enum class ECk_Ism_InstanceUpdatePolicy : uint8
{
    Recreate UMETA (DisplayName = "Destroy and Recreate"),
    Update UMETA(DisplayName = "Update in-place")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ism_InstanceUpdatePolicy);

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

UCLASS(BlueprintType)
class CKISMRENDERER_API UCk_IsmRenderer_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IsmRenderer_Data);

protected:
#if WITH_EDITOR
    auto
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Mobility _Mobility = ECk_Mobility::Static;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true, EditCondition="_Mobility == ECk_Mobility::Movable", EditConditionHides))
    ECk_Ism_InstanceUpdatePolicy _UpdatePolicy = ECk_Ism_InstanceUpdatePolicy::Update;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_MaterialsInfo _MaterialsInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_PhysicsInfo _PhysicsInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_LightingInfo _LightingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_IsmRenderer_CullingInfo _CullingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Ism_RenderPolicy _RenderPolicy = ECk_Ism_RenderPolicy::ISM;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _NumCustomData = 0;

    UPROPERTY(EditAnywhere,
              meta = (AllowPrivateAccess = true))
    FCustomPrimitiveData _CustomPrimitiveDataDefaults;

public:
    CK_PROPERTY_GET(_Mobility);
    CK_PROPERTY_GET(_UpdatePolicy);
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_MaterialsInfo);
    CK_PROPERTY_GET(_PhysicsInfo);
    CK_PROPERTY_GET(_LightingInfo);
    CK_PROPERTY_GET(_CullingInfo);
    CK_PROPERTY_GET(_RenderPolicy);
    CK_PROPERTY_GET(_NumCustomData);
    CK_PROPERTY_GET(_CustomPrimitiveDataDefaults);
};

// --------------------------------------------------------------------------------------------------------------------
