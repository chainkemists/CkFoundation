#pragma once

#include "CkAnimation/AnimAsset/CkAnimAsset_Fragment.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAnimAsset_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKANIMATION_API UCk_Utils_AnimAsset_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AnimAsset_UE);

private:
    struct RecordOfAnimAssets_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAnimAssets> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Add New Animation")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AnimAsset_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Add Multiple New Animation")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const TArray<FCk_Fragment_AnimAsset_ParamsData>& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Has Animation")
    static bool
    Has(
        FCk_Handle InAnimAssetOwnerEntity,
        FGameplayTag InAnimAssetName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Has Any Animation")
    static bool
    Has_Any(
        FCk_Handle InAnimAssetOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Ensure Has Animation")
    static bool
    Ensure(
        FCk_Handle InAnimAssetOwnerEntity,
        FGameplayTag InAnimAssetName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Ensure Has Any Animation")
    static bool
    Ensure_Any(
        FCk_Handle InAnimAssetOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Get All Animations")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InAnimAssetOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="Get Animation Asset Info")
    static FCk_AnimAsset_Animation
    Get_Animation(
        FCk_Handle InAnimAssetOwnerEntity,
        FGameplayTag InAnimName);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

