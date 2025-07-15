#pragma once

#include "Vfx/CkVfx_Fragment.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include <NiagaraSystem.h>

#include "CkVfx_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKFX_API UCk_Utils_Vfx_UE : public UBlueprintFunctionLibrary
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
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Vfx",
        DisplayName="[Ck][Vfx] Handle -> Vfx Handle",
        meta = (CompactNodeTitle = "<AsVfx>", BlueprintAutocast))
    static FCk_Handle_Vfx
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Vfx Handle",
        Category = "Ck|Utils|Vfx",
        meta = (CompactNodeTitle = "INVALID_VfxHandle", Keywords = "make"))
    static FCk_Handle_Vfx
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Vfx",
              DisplayName="[Ck][Vfx] Try Get Vfx")
    static FCk_Handle_Vfx
    TryGet_Vfx(
        const FCk_Handle& InVfxOwnerEntity,
        UPARAM(meta = (Categories = "Vfx")) FGameplayTag InVfxName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Vfx",
              DisplayName="[Ck][Vfx] Get Particle System")
    static UNiagaraSystem*
    Get_ParticleSystem(
        const FCk_Handle_Vfx& InVfxHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Vfx",
              DisplayName="[Ck][Vfx] Get Attachment Settings")
    static FCk_Vfx_AttachmentSettings
    Get_AttachmentSettings(
        const FCk_Handle_Vfx& InVfxHandle);

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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Vfx] Request Play (Attached)")
    static FCk_Handle_Vfx
    Request_PlayAttached(
        UPARAM(ref) FCk_Handle_Vfx& InVfxHandle,
        const FCk_Request_Vfx_PlayAttached& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Vfx] Request Play (At Location)")
    static FCk_Handle_Vfx
    Request_PlayAtLocation(
        UPARAM(ref) FCk_Handle_Vfx& InVfxHandle,
        const FCk_Request_Vfx_PlayAtLocation& InRequest);

public:
    static auto
    DoSet_NiagaraInstanceParameter(
        UNiagaraComponent* InVfx,
        const FCk_Vfx_InstanceParameterSettings& InInstanceParameterSettings) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
