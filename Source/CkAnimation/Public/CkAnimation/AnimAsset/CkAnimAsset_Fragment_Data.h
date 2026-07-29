#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/Public/CkProvider/CkProvider_Data.h"

#include <GameplayTagContainer.h>
#include <Animation/AnimationAsset.h>
#include <Animation/BlendSpace.h>

#include "CkAnimAsset_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANIMATION_API FCk_Handle_AnimAsset : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AnimAsset); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AnimAsset);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_AnimAsset_Animation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AnimAsset_Animation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "AnimAsset"))
    FGameplayTag _ID;

    // Soft by design: path-serialized, so it is safe to author in DataAssets/Blueprints (a weak ref
    // silently saves as null there; a hard ref force-loads the asset with the owning package), and a
    // fragment-held hard pointer would root nothing anyway (UE GC never walks the EnTT registry).
    // AnimAsset is pure path data — it kicks no loads and roots nothing; consumers resolve through
    // their own CkResourceLoader consumer id, or read resident-or-null via .Get().
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UAnimationAsset> _Animation;

    // Soft by design — see _Animation above.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UBlendSpace> _BlendSpace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _SectionName = NAME_None;

public:
    CK_PROPERTY_GET(_Animation);
    CK_PROPERTY_GET(_ID);
    CK_PROPERTY_GET(_BlendSpace);
    CK_PROPERTY(_SectionName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_AnimAsset_Animation, _ID, _Animation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_AnimAsset_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AnimAsset_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, TitleProperty = "_ID"))
    FCk_AnimAsset_Animation _AnimationAsset;

public:
    CK_PROPERTY_GET(_AnimationAsset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AnimAsset_ParamsData, _AnimationAsset);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_MultipleAnimAsset_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleAnimAsset_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_AnimAsset_ParamsData> _AnimAssetParams;

public:
    CK_PROPERTY_GET(_AnimAssetParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleAnimAsset_ParamsData, _AnimAssetParams);
};

// --------------------------------------------------------------------------------------------------------------------