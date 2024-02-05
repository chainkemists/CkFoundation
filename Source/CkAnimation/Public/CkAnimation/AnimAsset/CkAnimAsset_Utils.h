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
              DisplayName="[Ck][AnimAsset] Add New Animation")
    static FCk_Handle_AnimAsset
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AnimAsset_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Add Multiple New Animations")
    static FCk_Handle_AnimAsset
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleAnimAsset_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AnimAsset",
        DisplayName="[Ck][AnimAsset] Has AnimAsset")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AnimAsset",
        DisplayName="[Ck][AnimAsset] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AnimAsset
    Cast(
        const FCk_Handle&    InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AnimAsset",
        DisplayName="[Ck][AnimAsset] Handle -> AnimAsset Handle",
        meta = (CompactNodeTitle = "As AnimAssetHandle", BlueprintAutocast))
    static FCk_Handle_AnimAsset
    Conv_HandleToAnimAsset(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Get Handle")
    static FCk_Handle_AnimAsset
    TryGet_AnimAsset(
        const FCk_Handle& InAnimAssetOwnerEntity,
        FGameplayTag      InAnimAssetName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Get All Animations")
    static TArray<FCk_Handle_AnimAsset>
    Get_All(
        const FCk_Handle& InAnimAssetOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Get AnimAsset Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_AnimAsset& InAnimAssetEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Get Animation Asset Info")
    static FCk_AnimAsset_Animation
    Get_Animation(
        const FCk_Handle_AnimAsset& InAnimAssetEntity);
};

// --------------------------------------------------------------------------------------------------------------------

