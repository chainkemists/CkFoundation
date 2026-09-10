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
    static FCk_Fragment_UnrealComponent_ParamsData
    Make_Params(
        TSubclassOf<UActorComponent> InComponentClass,
        ECk_UnrealComponent_TickPolicy InTickPolicy = ECk_UnrealComponent_TickPolicy::DoNotTick,
        FName InDebugName = NAME_None);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Make Params (Archetype)",
              meta = (NativeMakeFunc))
    static FCk_Fragment_UnrealComponent_ParamsData
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
        const FCk_Fragment_UnrealComponent_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Remove",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Remove(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Bakes the hosted PRIMITIVE component's geometry into the Jolt static world (ExplicitActor
    // semantics — the bake filter does not apply; the caller declared it static-in-intent). Call
    // AFTER the component is configured: an ISM baked before its instances are added bakes nothing.
    // Calling again REPLACES the previous bodies (re-bake after repopulating instances). Removal is
    // automatic at teardown, or explicit via Request_RemoveFromJoltStaticWorld.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Bake Into Jolt Static World",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_UnrealComponent
    Request_BakeIntoJoltStaticWorld(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Removes the bodies added by Request_BakeIntoJoltStaticWorld. Idempotent — removing an
    // unbaked component succeeds (the intent "no baked bodies" holds).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Remove From Jolt Static World",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_UnrealComponent
    Request_RemoveFromJoltStaticWorld(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Stops the per-tick push of the owning entity's world transform onto this component — use when
    // it is about to be Unreal-physics-driven instead. Request_EnableTransformPush restores the
    // normal ownership contract when the external owner releases the component.
    // The disabled tag is per-COMPONENT; the owner's push memory (LastPushedTransform) keeps tracking
    // the owner while disabled, so re-enabling never needs to re-seed it — the enable path's
    // synchronization is what puts the returning component back in step with that memory.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Disable Transform Push",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_UnrealComponent
    Request_DisableTransformPush(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Restores transform pushing and synchronously snaps the scene component to its owning entity's
    // current authoritative transform before succeeding. This is safe to call immediately before a
    // spatial consumer (for example audio playback); it does not wait for PostTransform. Components
    // that are pending setup, non-scene, non-movable, invalid, baked into the Jolt static world, or
    // whose owner carries no push memory (LastPushedTransform) are rejected without removing the
    // disabled tag — the last of those would otherwise succeed into a component that is synchronized
    // once here and then never pushed to again.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Request Enable Transform Push",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_UnrealComponent
    Request_EnableTransformPush(
        UPARAM(ref) FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Returns whether Request_EnableTransformPush can synchronously take transform ownership now.
    // Use this to preflight every component in a coupled set before enabling any of them; immediate
    // calls on the same game-thread stack cannot interleave with the processor that changes setup,
    // ownership, mobility, or the hosted component.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UnrealComponent",
              DisplayName = "[Ck][UnrealComponent] Get Can Enable Transform Push")
    static bool
    Get_CanEnableTransformPush(
        const FCk_Handle_UnrealComponent& InUnrealComponent);

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
