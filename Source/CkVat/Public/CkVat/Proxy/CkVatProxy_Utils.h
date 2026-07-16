#pragma once

#include "CkVat/Proxy/CkVatProxy_Fragment.h"
#include "CkVat/Proxy/CkVatProxy_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVatProxy_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VatProxy"))
class CKVAT_API UCk_Utils_VatProxy_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VatProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VatProxy);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // The collection must be loaded AND baked (soft references: async-load first, mirror
    // CkIskmRenderer's contract).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][VatProxy] Add Feature")
    static FCk_Handle_VatProxy
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VatProxy_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VatProxy",
        DisplayName="[Ck][VatProxy] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VatProxy",
        DisplayName="[Ck][VatProxy] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VatProxy
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VatProxy",
        DisplayName="[Ck][VatProxy] Handle -> VatProxy Handle",
        meta = (CompactNodeTitle = "<AsVatProxy>", BlueprintAutocast))
    static FCk_Handle_VatProxy
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VatProxy Handle",
        Category = "Ck|Utils|VatProxy",
        meta = (CompactNodeTitle = "INVALID_VatProxyHandle", Keywords = "make"))
    static FCk_Handle_VatProxy
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VatProxy",
              DisplayName="[Ck][VatProxy] Request Play Clip")
    static FCk_Handle_VatProxy
    Request_PlayClip(
        UPARAM(ref) FCk_Handle_VatProxy& InHandle,
        const FCk_Request_VatProxy_PlayClip& InRequest);

    // Freezes playback on the current frame. Resume by issuing PlayClip (or SetPlayRate != 0).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VatProxy",
              DisplayName="[Ck][VatProxy] Request Stop")
    static FCk_Handle_VatProxy
    Request_Stop(
        UPARAM(ref) FCk_Handle_VatProxy& InHandle);

    // Preserves the playback position across the rate change. Rate 0 freezes (same contract as Stop).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VatProxy",
              DisplayName="[Ck][VatProxy] Request Set Play Rate")
    static FCk_Handle_VatProxy
    Request_SetPlayRate(
        UPARAM(ref) FCk_Handle_VatProxy& InHandle,
        float InPlayRate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VatProxy",
              DisplayName="[Ck][VatProxy] Get Collection")
    static UCk_VatCollection_Data*
    Get_Collection(
        const FCk_Handle_VatProxy& InHandle);

    // Name of the actively playing baked clip, or None while holding the reference pose.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VatProxy",
              DisplayName="[Ck][VatProxy] Get Active Clip Name")
    static FName
    Get_ActiveClipName(
        const FCk_Handle_VatProxy& InHandle);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|VatProxy",
        DisplayName="[Ck][VatProxy] Bind To OnClipFinished")
    static FCk_Handle_VatProxy
    BindTo_OnClipFinished(
        UPARAM(ref) FCk_Handle_VatProxy& InHandle,
        const FCk_Delegate_VatProxy_OnClipFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|VatProxy",
        DisplayName="[Ck][VatProxy] Unbind From OnClipFinished")
    static FCk_Handle_VatProxy
    UnbindFrom_OnClipFinished(
        UPARAM(ref) FCk_Handle_VatProxy& InHandle,
        const FCk_Delegate_VatProxy_OnClipFinished& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
