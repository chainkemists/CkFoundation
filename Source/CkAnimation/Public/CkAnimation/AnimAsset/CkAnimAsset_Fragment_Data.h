#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <Animation/AnimationAsset.h>

#include "CkAnimAsset_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Animation_AssetInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Animation_AssetInfo);

public:
    FCk_Animation_AssetInfo() = default;
    FCk_Animation_AssetInfo(
        const FGameplayTag& InAlias,
        UAnimationAsset* InAnimation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _Alias;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimationAsset> _Animation;

public:
    CK_PROPERTY_GET(_Animation);
    CK_PROPERTY_GET(_Alias);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Fragment_AnimAsset_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AnimAsset_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Animation_AssetInfo _AnimationAssetInfo;

public:
    CK_PROPERTY_GET(_AnimationAssetInfo);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AnimAsset_ParamsData, _AnimationAssetInfo);
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