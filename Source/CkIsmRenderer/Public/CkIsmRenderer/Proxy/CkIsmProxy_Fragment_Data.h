#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkIsmProxy_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_IsmRenderer_Data;

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
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_IsmRenderer_Data> _IsmRenderer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _LocalLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _LocalRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ScaleMultiplier = FVector::OneVector;

public:
    CK_PROPERTY_GET(_IsmRenderer);
    CK_PROPERTY_GET(_LocalLocationOffset);
    CK_PROPERTY_GET(_LocalRotationOffset);
    CK_PROPERTY_GET(_ScaleMultiplier);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_SetCustomData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_SetCustomData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<float> _CustomData;

public:
    CK_PROPERTY_GET(_CustomData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_SetCustomDataValue
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_SetCustomDataValue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _CustomDataIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _CustomDataValue = 0.0f;

public:
    CK_PROPERTY_GET(_CustomDataIndex);
    CK_PROPERTY_GET(_CustomDataValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IsmProxy_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

//UCLASS(Abstract, NotBlueprintable, BlueprintType)
//class CKISMRENDERER_API UCk_Ism_Definition_PDA : public UCk_DataAsset_PDA
//{
//    GENERATED_BODY()
//
//public:
//    CK_GENERATED_BODY(UCk_Ism_Definition_PDA);
//
//protected:
//#if WITH_EDITOR
//    auto IsDataValid(
//        class FDataValidationContext& Context) const -> EDataValidationResult override;
//#endif
//
//    auto GetPrimaryAssetId() const -> FPrimaryAssetId override;
//
//private:
//    UPROPERTY(EditDefaultsOnly,
//        meta = (AllowPrivateAccess = true))
//    FCk_Fragment_IsmRenderer_ParamsData _MeshParams;
//
//    UPROPERTY(EditDefaultsOnly,
//        Category="Developer Settings", AssetRegistrySearchable, AdvancedDisplay,
//        meta = (AllowPrivateAccess = true))
//    FName _AssetRegistryCategory = TEXT("InstancedStaticMeshes");
//
//public:
//    CK_PROPERTY_GET(_MeshParams);
//    CK_PROPERTY_GET(_AssetRegistryCategory);
//};

// --------------------------------------------------------------------------------------------------------------------
