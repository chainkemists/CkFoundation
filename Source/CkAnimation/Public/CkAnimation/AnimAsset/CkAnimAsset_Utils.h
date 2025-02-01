#pragma once

#include "CkAnimation/AnimAsset/CkAnimAsset_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include <InstancedStruct.h>

#include "CkAnimAsset_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKANIMATION_API UCk_Utils_AnimAsset_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AnimAsset_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AnimAsset);

private:
    struct RecordOfAnimAssets_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAnimAssets> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimAsset] Add New Animation")
    static FCk_Handle_AnimAsset
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AnimAsset_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Add Multiple New Animations")
    static TArray<FCk_Handle_AnimAsset>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleAnimAsset_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AnimAsset",
        DisplayName="[Ck][AnimAsset] Has AnimAsset")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AnimAsset",
        DisplayName="[Ck][AnimAsset] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AnimAsset
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AnimAsset",
        DisplayName="[Ck][AnimAsset] Handle -> AnimAsset Handle",
        meta = (CompactNodeTitle = "<AsAnimAsset>", BlueprintAutocast))
    static FCk_Handle_AnimAsset
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid AnimAsset Handle",
        Category = "Ck|Utils|AnimAsset",
        meta = (CompactNodeTitle = "INVALID_AnimAssetHandle", Keywords = "make"))
    static FCk_Handle_AnimAsset
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Try Get AnimAsset")
    static FCk_Handle_AnimAsset
    TryGet_AnimAsset(
        const FCk_Handle& InAnimAssetOwnerEntity,
        UPARAM(meta = (Categories = "AnimAsset")) FGameplayTag InAnimAssetName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_AnimAsset>
    ForEach_AnimAsset(
        UPARAM(ref) FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    static auto
    ForEach_AnimAsset(
        FCk_Handle& InHandle,
        const TFunction<void(FCk_Handle_AnimAsset&)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Get AnimAsset Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_AnimAsset& InAnimAssetEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimAsset",
              DisplayName="[Ck][AnimAsset] Get AnimAsset Info")
    static FCk_AnimAsset_Animation
    Get_Animation(
        const FCk_Handle_AnimAsset& InAnimAssetEntity);
};

// --------------------------------------------------------------------------------------------------------------------

