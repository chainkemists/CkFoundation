#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPool/ObjectPool/CkObjectPool_Data.h"
#include "CkPool/ObjectPool/CkObjectPool_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <StructUtils/InstancedStruct.h>
#include <Templates/SubclassOf.h>

#include "CkObjectPool_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UCk_ObjectPool_Subsystem_UE;

namespace ck
{
    struct RecordOfObjectPools_Utils : public TUtils_RecordOfEntities<FFragment_RecordOfObjectPools> {};
}

// --------------------------------------------------------------------------------------------------------------------

/**
 * Synchronous pooling for arbitrary UObjects and AActors — the non-entity sibling of UCk_Utils_EntityPool_UE.
 *
 * Contract summary (details in CkPool/Claude.md):
 * - Acquire returns immediately (pooled instance, or a fresh spawn under Grow policy; null on Fail policy /
 *   Bounded capacity). Pools are auto-created on first Acquire — configuration comes from the Pool project
 *   settings entry for the class, else built-in defaults; an explicit Request_CreatePool always wins.
 * - Actors are spawned hidden/frozen; acquire thaws them back to their CDO defaults (+ optional transform),
 *   release re-freezes (hidden, collision/tick off, timers cleared).
 * - Deep per-use reset is the implementer's job — via ICk_ObjectPool_Poolable (PrepareForUse / PrepareForPool /
 *   Get_CanBePooled) in C++/BP, or an FCk_Pool_PoolableReceiver property in ANY class including AngelScript
 *   (AS cannot implement UInterfaces). The pool only does the generic freeze/thaw.
 * - Optional per-use params (FInstancedStruct) ride the receiver's OnAcquiredFromPool hook — same contract
 *   as the EntityPool. The interface hooks stay parameterless (mirrors IMassActorPoolableInterface).
 * - Destroying an in-use instance externally is legal "steal" semantics (swept lazily); destroying a FREE
 *   (pool-owned) instance fires an ensure.
 * - v1 rejects replicated actor classes at pool creation.
 */
UCLASS(NotBlueprintable)
class CKPOOL_API UCk_Utils_ObjectPool_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ObjectPool_UE);

public:
    // Explicitly create (or fetch) a pool with full configuration. Call at load time to pay warm-up early.
    // Settings entries are NOT consulted on this path — these params win
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Request Create Pool",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_CreatePool(
        const UObject* InWorldContextObject,
        const FCk_ObjectPool_ParamsData& InParams);

    // Destroys the pool's free instances and stops tracking in-use ones (consumers finish with them normally)
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Request Destroy Pool",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_DestroyPool(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InObjectClass);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Acquire",
              meta = (WorldContext = "InWorldContextObject", DeterminesOutputType = "InObjectClass", AutoCreateRefTerm = "InPerUseParams"))
    static UObject*
    Acquire(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InObjectClass,
        const FInstancedStruct& InPerUseParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Acquire Actor",
              meta = (WorldContext = "InWorldContextObject", DeterminesOutputType = "InActorClass", AutoCreateRefTerm = "InPerUseParams"))
    static AActor*
    Acquire_Actor(
        const UObject* InWorldContextObject,
        TSubclassOf<AActor> InActorClass,
        const FTransform& InTransform,
        const FInstancedStruct& InPerUseParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Release")
    static void
    Release(
        UObject* InObject);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Get Stats",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_ObjectPool_Stats
    Get_Stats(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InObjectClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Get Is Pooled Object")
    static bool
    Get_IsPooledObject(
        const UObject* InObject);

    // Every live ObjectPool's registry entity — pools mirror into a Record on the world's transient
    // entity (enumeration/tooling only; the synchronous pool state stays on the subsystem)
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Get All Pools",
              meta = (WorldContext = "InWorldContextObject"))
    static TArray<FCk_Handle>
    Get_AllPools(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectPool",
              DisplayName="[Ck][ObjectPool] Get Pool Object Class")
    static TSubclassOf<UObject>
    Get_PoolObjectClass(
        const FCk_Handle& InPoolEntity);

private:
    static auto
    DoGet_Subsystem(
        const UObject* InWorldContextObject) -> UCk_ObjectPool_Subsystem_UE*;

    static auto
    DoMake_AutoCreateParams(
        const TSubclassOf<UObject>& InObjectClass) -> FCk_ObjectPool_ParamsData;

    static auto
    DoAcquire_Common(
        const UObject* InWorldContextObject,
        const TSubclassOf<UObject>& InObjectClass,
        const FTransform& InTransform,
        bool InHasTransform,
        const FInstancedStruct& InPerUseParams) -> UObject*;
};

// --------------------------------------------------------------------------------------------------------------------
