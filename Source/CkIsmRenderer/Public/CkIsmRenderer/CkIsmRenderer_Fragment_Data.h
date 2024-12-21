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
              meta = (AllowPrivateAccess = true, Categories = "Ism.RendererName"))
    FGameplayTag _RendererName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Collision _Collision = ECk_Collision::NoCollision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CastShadows = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_RendererName);
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_Collision);
    CK_PROPERTY_GET(_CastShadows);
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
        meta=(AllowPrivateAccess = true))
    FTransform _Transform;

public:
    CK_PROPERTY(_Transform);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKISMRENDERER_API FCk_Handle_IsmProxy : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IsmProxy); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IsmProxy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Fragment_IsmProxy_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IsmProxy_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ism.RendererName"))
    FGameplayTag _RendererName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Mobility _Mobility = ECk_Mobility::Static;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _MeshScale = FVector::OneVector;

public:
    CK_PROPERTY_GET(_RendererName);
    CK_PROPERTY_GET(_Mobility);
    CK_PROPERTY_GET(_MeshScale);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, BlueprintType)
class CKISMRENDERER_API UCk_Ism_Definition_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ism_Definition_PDA);

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

    auto GetPrimaryAssetId() const -> FPrimaryAssetId override;

private:
    UPROPERTY(EditDefaultsOnly,
        meta = (AllowPrivateAccess = true))
    FCk_Fragment_IsmRenderer_ParamsData _MeshParams;

    UPROPERTY(EditDefaultsOnly,
        Category="Developer Settings", AssetRegistrySearchable, AdvancedDisplay,
        meta = (AllowPrivateAccess = true))
    FName _AssetRegistryCategory = TEXT("InstancedStaticMeshes");

public:
    CK_PROPERTY_GET(_MeshParams);
    CK_PROPERTY_GET(_AssetRegistryCategory);
};

// --------------------------------------------------------------------------------------------------------------------
