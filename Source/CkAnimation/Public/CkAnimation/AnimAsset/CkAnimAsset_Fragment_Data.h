#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkProvider/Public/CkProvider/CkProvider_Data.h"

#include <GameplayTagContainer.h>
#include <Animation/AnimationAsset.h>

#include "CkAnimAsset_Fragment_Data.generated.h"

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _ID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimationAsset> _Animation;

public:
    CK_PROPERTY_GET(_Animation);
    CK_PROPERTY_GET(_ID);
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

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKANIMATION_API UCk_Provider_AnimAsset_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_AnimAsset_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|AnimAsset")
    FCk_Fragment_AnimAsset_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKANIMATION_API UCk_Provider_AnimAsset_ParamsData_Literal_PDA : public UCk_Provider_AnimAsset_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_AnimAsset_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_AnimAsset_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_AnimAsset_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATION_API FCk_Provider_AnimAsset_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_AnimAsset_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_AnimAsset_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKANIMATION_API UCk_Provider_MultipleAnimAsset_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleAnimAsset_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|AnimAsset")
    FCk_Fragment_MultipleAnimAsset_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKANIMATION_API UCk_Provider_MultipleAnimAsset_ParamsData_Literal_PDA : public UCk_Provider_MultipleAnimAsset_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleAnimAsset_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MultipleAnimAsset_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MultipleAnimAsset_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------