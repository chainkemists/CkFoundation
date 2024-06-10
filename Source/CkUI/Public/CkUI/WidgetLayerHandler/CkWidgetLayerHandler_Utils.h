#pragma once

#include "CkUI/WidgetLayerHandler/CkWidgetLayerHandler_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkWidgetLayerHandler_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_Utils_WidgetLayerHandler_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_WidgetLayerHandler_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_WidgetLayerHandler);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName="[Ck][WidgetLayerHandler] Add Feature")
    static FCk_Handle_WidgetLayerHandler
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_WidgetLayerHandler_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName="[Ck][WidgetLayerHandler] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName="[Ck][WidgetLayerHandler] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_WidgetLayerHandler
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName="[Ck][WidgetLayerHandler] Handle -> WidgetLayerHandler Handle",
        meta = (CompactNodeTitle = "<AsWidgetLayerHandler>", BlueprintAutocast))
    static FCk_Handle_WidgetLayerHandler
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        meta = (AutoCreateRefTerm = "InDelegate"),
        DisplayName="[Ck][WidgetLayerHandler] Request Push To Layer")
    static FCk_Handle_WidgetLayerHandler
    Request_PushToLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_PushToLayer& InRequest);
    
    UFUNCTION(BlueprintCallable,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        meta = (AutoCreateRefTerm = "InDelegate"),
        DisplayName="[Ck][WidgetLayerHandler] Request Push Instanced Widget To Layer")
    static FCk_Handle_WidgetLayerHandler
    Request_PushToLayer_Instanced(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_PushToLayer_Instanced& InRequest);

	UFUNCTION(BlueprintCallable,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        DisplayName="[Ck][WidgetLayerHandler] Request Pop From Layer")
	static FCk_Handle_WidgetLayerHandler
	Request_PopFromLayer(
		UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
		const FCk_Request_WidgetLayerHandler_PopFromLayer& InRequest);

	UFUNCTION(BlueprintCallable,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        DisplayName="[Ck][WidgetLayerHandler] Request Clear Layer")
	static FCk_Handle_WidgetLayerHandler
	Request_ClearLayer(
		UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
		const FCk_Request_WidgetLayerHandler_ClearLayer& InRequest);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Bind To OnPushToLayer")
    static FCk_Handle_WidgetLayerHandler
    BindTo_OnPushToLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer& InDelegate);
    
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Unbind From OnPushToLayer")
    static FCk_Handle_WidgetLayerHandler
    UnbindFrom_OnPushToLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Bind To OnPushToLayer_Instanced")
    static FCk_Handle_WidgetLayerHandler
    BindTo_OnPushToLayer_Instanced(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced& InDelegate);
    
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Unbind From OnPushToLayer_Instanced")
    static FCk_Handle_WidgetLayerHandler
    UnbindFrom_OnPushToLayer_Instanced(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Bind To OnPopFromLayer")
    static FCk_Handle_WidgetLayerHandler
    BindTo_OnPopFromLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnPopFromLayer& InDelegate);
    
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Unbind From OnPopFromLayer")
    static FCk_Handle_WidgetLayerHandler
    UnbindFrom_OnPopFromLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnPopFromLayer& InDelegate);
    
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Bind To OnClearLayer")
    static FCk_Handle_WidgetLayerHandler
    BindTo_OnClearLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnClearLayer& InDelegate);
    
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WidgetLayerHandler",
        DisplayName = "[Ck][WidgetLayerHandler] Unbind From OnClearLayer")
    static FCk_Handle_WidgetLayerHandler
    UnbindFrom_OnClearLayer(
        UPARAM(ref) FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnClearLayer& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
