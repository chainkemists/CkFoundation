#pragma once

#include "Vfx/CkVfx_Fragment.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkVfx_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKFX_API UCk_Utils_Vfx_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vfx_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Vfx);

private:
    struct RecordOfVfx_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVfx> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Vfx] Add New Vfx")
    static FCk_Handle_Vfx
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Vfx_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Vfx",
              DisplayName="[Ck][Vfx] Add Multiple New Vfx")
    static TArray<FCk_Handle_Vfx>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleVfx_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Vfx",
        DisplayName="[Ck][Vfx] Has Any Vfx")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Vfx",
        DisplayName="[Ck][Vfx] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Vfx
    DoCast(
        FCk_Handle InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Vfx",
        DisplayName="[Ck][Vfx] Handle -> Vfx Handle",
        meta = (CompactNodeTitle = "<AsVfx>", BlueprintAutocast))
    static FCk_Handle_Vfx
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Vfx",
              DisplayName="[Ck][Vfx] Try Get Vfx")
    static FCk_Handle_Vfx
    TryGet_Vfx(
        const FCk_Handle& InVfxOwnerEntity,
        FGameplayTag InVfxName);

public:
    // TODO: Add getters

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Vfx",
              DisplayName="[Ck][Vfx] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Vfx>
    ForEach_Vfx(
        UPARAM(ref) FCk_Handle& InVfxOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Vfx(
        FCk_Handle& InVfxOwnerEntity,
        const TFunction<void(FCk_Handle_Vfx)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
