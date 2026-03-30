#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/ContextReceiver/CkContextReceiver.h"

#include "CkContextReceiver_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ContextReceiver"))
class CKECS_API UCk_Utils_ContextReceiver_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ContextReceiver_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Request Inject Context")
    static FCk_Handle_ContextReceiver
    Request_InjectContext(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Handle& InContextEntity);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Request Clear Context")
    static FCk_Handle_ContextReceiver
    Request_ClearContext(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Has Valid Context")
    static bool
    Has_ValidContext(
        const FCk_Handle_ContextReceiver& InContextReceiver);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Get Context Entity")
    static FCk_Handle
    Get_ContextEntity(
        const FCk_Handle_ContextReceiver& InContextReceiver);

    // ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Bind To OnContextInjected")
    static FCk_Handle_ContextReceiver
    BindTo_OnContextInjected(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextInjected& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Unbind From OnContextInjected")
    static FCk_Handle_ContextReceiver
    UnbindFrom_OnContextInjected(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextInjected& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Bind To OnContextCleared")
    static FCk_Handle_ContextReceiver
    BindTo_OnContextCleared(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextCleared& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Unbind From OnContextCleared")
    static FCk_Handle_ContextReceiver
    UnbindFrom_OnContextCleared(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextCleared& InDelegate);

    // ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Request Unbind All")
    static void
    Request_UnbindAll(
        UPARAM(ref) FCk_Handle_ContextReceiver& InContextReceiver,
        UObject* InBoundObject);

    // ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ContextReceiver",
              DisplayName = "[Ck][ContextReceiver] Try Inject Context Into Object",
              meta = (DefaultToSelf = "InObject"))
    static ECk_ContextReceiver_InjectResult
    TryInjectContextIntoObject(
        UObject* InObject,
        const FCk_Handle& InContextEntity);
};

// --------------------------------------------------------------------------------------------------------------------
