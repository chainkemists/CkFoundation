#pragma once

#include "CkVat/CkVat_Fragment.h"
#include "CkVat/CkVat_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVat_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Vat"))
class CKVAT_API UCk_Utils_Vat_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vat_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Vat);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // The collection must be loaded AND baked (soft references: async-load first, mirror
    // CkIskmRenderer's contract). The rendering hookup lands in Gate 3 — until then a Vat
    // entity carries playback state without a visual.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Vat] Add New Vat")
    static FCk_Handle_Vat
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Vat_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Vat",
        DisplayName="[Ck][Vat] Has Vat")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Vat",
        DisplayName="[Ck][Vat] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Vat
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Vat",
        DisplayName="[Ck][Vat] Handle -> Vat Handle",
        meta = (CompactNodeTitle = "<AsVat>", BlueprintAutocast))
    static FCk_Handle_Vat
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Vat Handle",
        Category = "Ck|Utils|Vat",
        meta = (CompactNodeTitle = "INVALID_VatHandle", Keywords = "make"))
    static FCk_Handle_Vat
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Vat",
              DisplayName="[Ck][Vat] Request Play Clip")
    static FCk_Handle_Vat
    Request_PlayClip(
        UPARAM(ref) FCk_Handle_Vat& InHandle,
        const FCk_Request_Vat_PlayClip& InRequest);

    // Freezes playback on the current frame. Resume by issuing PlayClip (or SetPlayRate != 0).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Vat",
              DisplayName="[Ck][Vat] Request Stop")
    static FCk_Handle_Vat
    Request_Stop(
        UPARAM(ref) FCk_Handle_Vat& InHandle);

    // Preserves the playback position across the rate change. Rate 0 freezes (same contract as Stop).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Vat",
              DisplayName="[Ck][Vat] Request Set Play Rate")
    static FCk_Handle_Vat
    Request_SetPlayRate(
        UPARAM(ref) FCk_Handle_Vat& InHandle,
        float InPlayRate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Vat",
              DisplayName="[Ck][Vat] Get Collection")
    static UCk_VatCollection_Data*
    Get_Collection(
        const FCk_Handle_Vat& InHandle);

    // Name of the actively playing baked clip, or None while holding the reference pose.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Vat",
              DisplayName="[Ck][Vat] Get Active Clip Name")
    static FName
    Get_ActiveClipName(
        const FCk_Handle_Vat& InHandle);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Vat",
        DisplayName="[Ck][Vat] Bind To OnClipFinished")
    static FCk_Handle_Vat
    BindTo_OnClipFinished(
        UPARAM(ref) FCk_Handle_Vat& InHandle,
        const FCk_Delegate_Vat_OnClipFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Vat",
        DisplayName="[Ck][Vat] Unbind From OnClipFinished")
    static FCk_Handle_Vat
    UnbindFrom_OnClipFinished(
        UPARAM(ref) FCk_Handle_Vat& InHandle,
        const FCk_Delegate_Vat_OnClipFinished& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
