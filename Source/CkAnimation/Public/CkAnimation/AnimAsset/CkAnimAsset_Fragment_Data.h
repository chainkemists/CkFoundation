#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/Public/CkProvider/CkProvider_Data.h"

#include <GameplayTagContainer.h>
#include <Animation/AnimationAsset.h>

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

public:
    FCk_AnimAsset_Animation() = default;
    FCk_AnimAsset_Animation(
        const FGameplayTag& InID,
        UAnimationAsset* InAnimation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "AnimAsset"))
    FGameplayTag _ID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimationAsset> _Animation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _SectionName = NAME_None;

public:
    CK_PROPERTY_GET(_Animation);
    CK_PROPERTY_GET(_ID);
    CK_PROPERTY_GET(_SectionName);
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