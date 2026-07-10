#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkPool/EntityPool/CkEntityPool_Fragment.h"
#include "CkPool/EntityPool/CkEntityPool_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkEntityPool_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityPool_Subsystem_UE;
class UCk_EntityScript_UE;

namespace ck
{
    class FProcessor_EntityPool_Prewarm;
    class FProcessor_EntityPool_HandleRequests;
    class FProcessor_EntityPool_HandleDestroyedPooledEntity;

    struct RecordOfEntityPools_Utils : public TUtils_RecordOfEntities<FFragment_RecordOfEntityPools> {};
}

// --------------------------------------------------------------------------------------------------------------------

/**
 * Object pooling for EntityScript-spawned entities.
 *
 * Contract summary (details in CkPool/Claude.md):
 * - Acquire is deferred: Request_Acquire returns a pending handle; bind Promise_OnAcquired IMMEDIATELY
 *   (same frame) — the promise is fulfilled by the pool's HandleRequests processor.
 * - Per-use data rides the OnAcquiredFromPool signal payload, NEVER Construct — instances Construct ONCE
 *   (at prewarm/grow) with the pool's ConstructionSpawnParams. On every acquire, per-use params are ALSO
 *   injected into matching script properties (spawn-param semantics) before the hooks fire; BeginPlay is
 *   NOT re-run — OnAcquiredFromPool is the per-use begin.
 * - Quiescence is the pooled script's job: on OnReleasedToPool deactivate visuals/audio, cancel per-use
 *   timers, unbind per-use signal bindings. The pool never touches feature state.
 * - Hook surfaces (both fire; pick one per script): the entity signals below (BindTo_OnAcquiredFromPool /
 *   BindTo_OnReleasedToPool), or an FCk_Pool_PoolableReceiver property on the EntityScript — auto-detected
 *   by reflection, bind in Construct. The receiver also carries the _CanBePooled release-time veto
 *   (false = the pool destroys the instance instead of parking it).
 * - The pool retains lifetime ownership across acquires. Consumers Release; destroying an in-use pooled
 *   entity instead is legal "steal" semantics (the pool forgets it).
 * - v1 supports DoesNotReplicate EntityScripts only (enforced at pool creation).
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_EntityPool"))
class CKPOOL_API UCk_Utils_EntityPool_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityPool_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EntityPool);

public:
    friend class UCk_EntityPool_Subsystem_UE;
    friend class ck::FProcessor_EntityPool_Prewarm;
    friend class ck::FProcessor_EntityPool_HandleRequests;
    friend class ck::FProcessor_EntityPool_HandleDestroyedPooledEntity;

public:
    // Explicitly create (or fetch) a pool with full configuration — prewarm, capacity, exhaustion policy,
    // optional pool name. Call at load time to pay warm-up cost early
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Request Create Pool",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle_EntityPool
    Request_CreatePool(
        const UObject* InWorldContextObject,
        const FCk_Fragment_EntityPool_ParamsData& InParams);

    // Destroys the pool AND everything it owns: dormant instances, parked acquires (fulfilled with Failed),
    // and — because pooled instances are lifetime children of the pool — any instances still in use
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Request Destroy Pool",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_DestroyPool(
        const UObject* InWorldContextObject,
        UPARAM(ref) FCk_Handle_EntityPool& InPool);

public:
    // The 90% case: acquire from the class's default pool, auto-creating it (no prewarm, unbounded, Grow)
    // on first use. Bind Promise_OnAcquired on the returned handle IMMEDIATELY (same frame)
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Request Acquire",
              meta = (WorldContext = "InWorldContextObject", AutoCreateRefTerm = "InPerUseParams"))
    static FCk_Handle_PendingEntityPoolAcquire
    Request_Acquire(
        const UObject* InWorldContextObject,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        const FInstancedStruct& InPerUseParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Request Acquire (On Pool)",
              meta = (AutoCreateRefTerm = "InPerUseParams"))
    static FCk_Handle_PendingEntityPoolAcquire
    Request_Acquire_OnPool(
        UPARAM(ref) FCk_Handle_EntityPool& InPool,
        const FInstancedStruct& InPerUseParams);

    // One-shot acquire that supplies the pool's FULL configuration for the lazy-create case (prewarm,
    // capacity, archetype, name, ...). The params are consulted ONLY if THIS call creates the pool —
    // an existing pool (matched by name, else class) wins outright, same rule as Project Settings
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Request Acquire (With Pool Params)",
              meta = (WorldContext = "InWorldContextObject", AutoCreateRefTerm = "InPerUseParams"))
    static FCk_Handle_PendingEntityPoolAcquire
    Request_Acquire_WithPoolParams(
        const UObject* InWorldContextObject,
        const FCk_Fragment_EntityPool_ParamsData& InPoolParams,
        const FInstancedStruct& InPerUseParams);

    // Return a pooled entity to its owning pool. The pool broadcasts OnReleasedToPool on the entity
    // (quiescence hook), then either re-vends it to a parked acquire or parks it dormant
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Request Release To Pool")
    static FCk_Handle
    Request_ReleaseToPool(
        UPARAM(ref) FCk_Handle& InPooledEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Try Get Pool (By Class)",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle_EntityPool
    TryGet_Pool_ByClass(
        const UObject* InWorldContextObject,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Try Get Pool (By Name)",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle_EntityPool
    TryGet_Pool_ByName(
        const UObject* InWorldContextObject,
        UPARAM(meta = (Categories = "EntityPool")) FGameplayTag InPoolName);

    // Every live EntityPool in the world — pools are Record entries on the world's transient entity
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Get All Pools",
              meta = (WorldContext = "InWorldContextObject"))
    static TArray<FCk_Handle_EntityPool>
    Get_AllPools(
        const UObject* InWorldContextObject);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityPool",
        DisplayName="[Ck][EntityPool] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EntityPool
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityPool",
        DisplayName="[Ck][EntityPool] Handle -> EntityPool Handle",
        meta = (CompactNodeTitle = "<AsEntityPool>", BlueprintAutocast))
    static FCk_Handle_EntityPool
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid EntityPool Handle",
        Category = "Ck|Utils|EntityPool",
        meta = (CompactNodeTitle = "INVALID_EntityPoolHandle", Keywords = "make"))
    static FCk_Handle_EntityPool
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Get Stats")
    static FCk_EntityPool_Stats
    Get_Stats(
        const FCk_Handle_EntityPool& InPool);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Get Is Pooled Entity")
    static bool
    Get_IsPooledEntity(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Get Is Dormant")
    static bool
    Get_IsDormant(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Try Get Owning Pool")
    static FCk_Handle_EntityPool
    TryGet_OwningPool(
        const FCk_Handle& InHandle);

    // Increments on every acquire. Cache it next to a stored pooled-entity handle and compare on read —
    // pooled entities keep the same EnTT id+version across uses, so ck::IsValid alone cannot detect recycling
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityPool",
              DisplayName="[Ck][EntityPool] Get Use Generation")
    static int32
    Get_UseGeneration(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Bind To OnAcquiredFromPool")
    static FCk_Handle
    BindTo_OnAcquiredFromPool(
        UPARAM(ref) FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityAcquired& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Unbind From OnAcquiredFromPool")
    static FCk_Handle
    UnbindFrom_OnAcquiredFromPool(
        UPARAM(ref) FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityAcquired& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Bind To OnReleasedToPool")
    static FCk_Handle
    BindTo_OnReleasedToPool(
        UPARAM(ref) FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityReleased& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Unbind From OnReleasedToPool")
    static FCk_Handle
    UnbindFrom_OnReleasedToPool(
        UPARAM(ref) FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityReleased& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Bind To OnExhausted")
    static FCk_Handle_EntityPool
    BindTo_OnExhausted(
        UPARAM(ref) FCk_Handle_EntityPool& InPool,
        const FCk_Delegate_EntityPool_Exhausted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Unbind From OnExhausted")
    static FCk_Handle_EntityPool
    UnbindFrom_OnExhausted(
        UPARAM(ref) FCk_Handle_EntityPool& InPool,
        const FCk_Delegate_EntityPool_Exhausted& InDelegate);

private:
    static auto
    DoGet_Subsystem(
        const UObject* InWorldContextObject) -> UCk_EntityPool_Subsystem_UE*;

    static auto
    DoCreate_PoolEntity(
        UCk_EntityPool_Subsystem_UE* InSubsystem,
        const FCk_Fragment_EntityPool_ParamsData& InParams) -> FCk_Handle_EntityPool;

    static auto
    DoRequest_HandleConstructed(
        FCk_Handle_EntityPool& InPool,
        FCk_Handle InConstructedEntity) -> void;

    static auto
    DoRequest_HandleDestroyed(
        FCk_Handle_EntityPool InPool,
        FCk_Handle InDestroyedEntity,
        bool InWasDormant,
        bool InWasInUse) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PendingEntityPoolAcquire"))
class CKPOOL_API UCk_Utils_PendingEntityPoolAcquire_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PendingEntityPoolAcquire_UE);

public:
    // Fire-once, sticky: fires immediately if the acquire was already fulfilled earlier THIS frame.
    // Bind in the same frame as Request_Acquire — the fulfillment ticket does not outlive its fulfillment
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityPool",
              DisplayName = "[Ck][EntityPool] Promise/Future OnAcquired",
              meta = (Keywords = "bind, wait, when, finish"))
    static void
    Promise_OnAcquired(
        UPARAM(ref) FCk_Handle_PendingEntityPoolAcquire& InPendingAcquire,
        const FCk_Delegate_EntityPool_Acquired& InDelegate);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][EntityPool] Get Is Valid (Pending Acquire)",
              Category = "Ck|Utils|EntityPool",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid(
        const FCk_Handle_PendingEntityPoolAcquire& InPendingAcquire);
};

// --------------------------------------------------------------------------------------------------------------------
