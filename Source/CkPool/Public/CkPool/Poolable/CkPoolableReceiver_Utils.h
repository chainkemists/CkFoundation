#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPool/Poolable/CkPoolableReceiver.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkPoolableReceiver_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Pool_PoolableReceiver"))
class CKPOOL_API UCk_Utils_PoolableReceiver_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PoolableReceiver_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Bind To OnAcquiredFromPool")
    static FCk_Pool_PoolableReceiver
    BindTo_OnAcquiredFromPool(
        UPARAM(ref) FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnAcquired& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Unbind From OnAcquiredFromPool")
    static FCk_Pool_PoolableReceiver
    UnbindFrom_OnAcquiredFromPool(
        UPARAM(ref) FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnAcquired& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Bind To OnReleasedToPool")
    static FCk_Pool_PoolableReceiver
    BindTo_OnReleasedToPool(
        UPARAM(ref) FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnReleased& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Unbind From OnReleasedToPool")
    static FCk_Pool_PoolableReceiver
    UnbindFrom_OnReleasedToPool(
        UPARAM(ref) FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnReleased& InDelegate);

    // ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Request Unbind All")
    static void
    Request_UnbindAll(
        UPARAM(ref) FCk_Pool_PoolableReceiver& InPoolableReceiver,
        UObject* InBoundObject);

    // ----

    // Per-instance veto read by the owning pool at Release time. False = the pool destroys the
    // instance instead of storing it. Flip it back to true before the next Release to re-enable pooling
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Set Can Be Pooled")
    static FCk_Pool_PoolableReceiver
    Set_CanBePooled(
        UPARAM(ref) FCk_Pool_PoolableReceiver& InPoolableReceiver,
        bool InCanBePooled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poolable",
              DisplayName = "[Ck][Poolable] Get Can Be Pooled")
    static bool
    Get_CanBePooled(
        const FCk_Pool_PoolableReceiver& InPoolableReceiver);

public:
    // Pool-side plumbing (not UFUNCTIONs): reflection-scan InObject for FCk_Pool_PoolableReceiver
    // properties (IncludeSuper) and fire the hook on each. Objects without a receiver no-op

    static auto
    Broadcast_AcquiredFromPool_OnObject(
        UObject* InObject,
        const FInstancedStruct& InPerUseParams) -> void;

    static auto
    Broadcast_ReleasedToPool_OnObject(
        UObject* InObject) -> void;

    // True when the object has NO receiver (opt-in only gates instances that declared one).
    // With multiple receivers, ANY _CanBePooled == false vetoes
    static auto
    Get_CanBePooled_OnObject(
        const UObject* InObject) -> bool;

    static auto
    Has_PoolableReceiver(
        const UObject* InObject) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
