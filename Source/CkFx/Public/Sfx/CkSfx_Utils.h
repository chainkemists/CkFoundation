#pragma once

#include "Sfx/CkSfx_Fragment.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkSfx_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKFX_API UCk_Utils_Sfx_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Sfx_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Sfx);

private:
    struct RecordOfSfx_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSfx> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Sfx] Add New Sfx")
    static FCk_Handle_Sfx
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Sfx_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sfx",
              DisplayName="[Ck][Sfx] Add Multiple New Sfx")
    static TArray<FCk_Handle_Sfx>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleSfx_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Sfx",
        DisplayName="[Ck][Sfx] Has Any Sfx")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Sfx",
        DisplayName="[Ck][Sfx] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Sfx
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Sfx",
        DisplayName="[Ck][Sfx] Handle -> Sfx Handle",
        meta = (CompactNodeTitle = "<AsSfx>", BlueprintAutocast))
    static FCk_Handle_Sfx
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Sfx Handle",
        Category = "Ck|Utils|Sfx",
        meta = (CompactNodeTitle = "INVALID_SfxHandle", Keywords = "make"))
    static FCk_Handle_Sfx
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sfx",
              DisplayName="[Ck][Sfx] Try Get Sfx")
    static FCk_Handle_Sfx
    TryGet_Sfx(
        const FCk_Handle& InSfxOwnerEntity,
        UPARAM(meta = (Categories = "Sfx")) FGameplayTag InSfxName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sfx",
              DisplayName="[Ck][Sfx] Get Sound Cue")
    static USoundBase*
    Get_SoundCue(
        const FCk_Handle_Sfx& InSfxHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sfx",
              DisplayName="[Ck][Sfx] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Sfx>
    ForEach_Sfx(
        UPARAM(ref) FCk_Handle& InSfxOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Sfx(
        FCk_Handle& InSfxOwnerEntity,
        const TFunction<void(FCk_Handle_Sfx)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Sfx] Request Play (Attached)")
    static FCk_Handle_Sfx
    Request_PlayAttached(
        UPARAM(ref) FCk_Handle_Sfx& InSfxHandle,
        const FCk_Request_Sfx_PlayAttached& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Sfx] Request Play (At Location)")
    static FCk_Handle_Sfx
    Request_PlayAtLocation(
        UPARAM(ref) FCk_Handle_Sfx& InSfxHandle,
        const FCk_Request_Sfx_PlayAtLocation& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
