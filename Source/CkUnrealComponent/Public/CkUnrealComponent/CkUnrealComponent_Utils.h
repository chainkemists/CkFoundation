#pragma once

#include "CkUnrealComponent/CkUnrealComponent_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "Components/ActorComponent.h"

#include "CkUnrealComponent_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUNREALCOMPONENT_API UCk_Utils_UnrealComponent_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UnrealComponent_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_UnrealComponent);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|UnrealComponent",
        DisplayName = "[Ck][UnrealComponent] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_UnrealComponent
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|UnrealComponent",
        DisplayName = "[Ck][UnrealComponent] Handle -> UnrealComponent Handle",
        meta = (CompactNodeTitle = "<AsUnrealComponent>", BlueprintAutocast))
    static FCk_Handle_UnrealComponent
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid UnrealComponent Handle",
        Category = "Ck|Utils|UnrealComponent",
        meta = (CompactNodeTitle = "INVALID_UnrealComponentHandle", Keywords = "make"))
    static FCk_Handle_UnrealComponent
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Make Params (Class)",
              meta = (NativeMakeFunc))
    static FCk_UnrealComponent_Spec
    Make_Params(
        TSubclassOf<UActorComponent> InComponentClass,
        ECk_UnrealComponent_TickPolicy InTickPolicy = ECk_UnrealComponent_TickPolicy::DoNotTick,
        FName InDebugName = NAME_None);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Make Params (Archetype)",
              meta = (NativeMakeFunc))
    static FCk_UnrealComponent_Spec
    Make_Params_FromArchetype(
        UActorComponent* InComponentArchetype,
        ECk_UnrealComponent_TickPolicy InTickPolicy = ECk_UnrealComponent_TickPolicy::DoNotTick,
        FName InDebugName = NAME_None);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Add")
    static FCk_Handle_UnrealComponent
    Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_UnrealComponent_Spec& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Remove",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Remove(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Stops the per-tick push of the owning entity's world transform onto this component — use when
    // it is about to be Unreal-physics-driven instead. ONE-WAY: there is no re-enable, and the
    // component is expected to be short-lived after opting out.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Disable Transform Push",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_UnrealComponent
    Request_DisableTransformPush(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get Component")
    static UActorComponent*
    Get_Component(
        const FCk_Handle_UnrealComponent& InUnrealComponent);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get Owning Entity")
    static FCk_Handle
    Get_OwningEntity(
        const FCk_Handle_UnrealComponent& InUnrealComponent);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Try Get Owning Handle From Component")
    static FCk_Handle_UnrealComponent
    TryGet_OwningHandle_FromComponent(
        UActorComponent* InComponent);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get All Handles")
    static TArray<FCk_Handle_UnrealComponent>
    Get_AllHandles(
        const FCk_Handle& InOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get All Components")
    static TArray<UActorComponent*>
    Get_AllComponents(
        const FCk_Handle& InOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Try Get Handle By Type",
              meta = (DeterminesOutputType = "InComponentClass"))
    static FCk_Handle_UnrealComponent
    TryGet_HandleByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get Handles By Type",
              meta = (DeterminesOutputType = "InComponentClass"))
    static TArray<FCk_Handle_UnrealComponent>
    Get_HandlesByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Try Get Component By Type",
              meta = (DeterminesOutputType = "InComponentClass"))
    static UActorComponent*
    TryGet_ComponentByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get Components By Type",
              meta = (DeterminesOutputType = "InComponentClass"))
    static TArray<UActorComponent*>
    Get_ComponentsByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Bind To OnAdded")
    static FCk_Handle_UnrealComponent
    BindTo_OnAdded(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnAdded& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Bind To OnRemoved")
    static FCk_Handle_UnrealComponent
    BindTo_OnRemoved(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Unbind From OnAdded")
    static FCk_Handle_UnrealComponent
    UnbindFrom_OnAdded(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Unbind From OnRemoved")
    static FCk_Handle_UnrealComponent
    UnbindFrom_OnRemoved(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnRemoved& InDelegate);

public:
    static auto
    DoRegisterBridge(
        UActorComponent* InComponent,
        FCk_Handle_UnrealComponent InHandle) -> void;

    static auto
    DoUnregisterBridge(
        UActorComponent* InComponent) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
