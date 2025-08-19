#pragma once

#include "CkDeferredEntity_Fragment.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkDeferredEntity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_DeferredEntity"))
class CKECS_API UCk_Utils_DeferredEntity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DeferredEntity_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_DeferredEntity);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Create Deferred Entity")
    static FCk_Handle_DeferredEntity
    Create(
        const FCk_Handle& InContextEntity);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Add Deferred To Entity")
    static FCk_Handle_DeferredEntity
    Add(
        UPARAM(ref) FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Is Deferred")
    static bool
    Is_Deferred(
        const FCk_Handle_DeferredEntity& InDeferredEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Get Pending Count")
    static int32
    Get_PendingCount(
        const FCk_Handle_DeferredEntity& InDeferredEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_DeferredEntity
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Handle -> Deferred Handle",
        meta = (CompactNodeTitle = "<AsDeferredEntity>", BlueprintAutocast))
    static FCk_Handle_DeferredEntity
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName = "[Ck][DeferredEntity] Get Invalid Handle",
        meta = (CompactNodeTitle = "INVALID_DeferredEntityHandle", Keywords = "make"))
    static FCk_Handle_DeferredEntity
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Complete Setup")
    static void
    Request_CompleteSetup(
        UPARAM(ref) FCk_Handle_DeferredEntity& InDeferredEntity);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Bind To OnSetupComplete")
    static void
    BindTo_OnSetupComplete(
        UPARAM(ref) FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Bind To OnFullyComplete")
    static void
    BindTo_OnFullyComplete(
        UPARAM(ref) FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnFullyComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Unbind From OnSetupComplete")
    static void
    UnbindFrom_OnSetupComplete(
        UPARAM(ref) FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|DeferredEntity",
        DisplayName="[Ck][DeferredEntity] Unbind From OnFullyComplete")
    static void
    UnbindFrom_OnFullyComplete(
        UPARAM(ref) FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnFullyComplete& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------