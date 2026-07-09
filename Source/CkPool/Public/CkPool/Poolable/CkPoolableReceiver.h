#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <StructUtils/InstancedStruct.h>

#include "CkPoolableReceiver.generated.h"

class UCk_Utils_PoolableReceiver_UE;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_PoolableReceiver_OnAcquired_MC,
    FInstancedStruct);

DECLARE_MULTICAST_DELEGATE(
    FCk_Delegate_PoolableReceiver_OnReleased_MC);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Property-based pooling opt-in — the AngelScript-reachable sibling of ICk_ObjectPool_Poolable.
 *
 * AngelScript classes cannot implement UInterfaces (Hazelight fork limitation), so the interface
 * route is C++/BP-only. Instead, ANY UObject-derived class (actors, plain objects, EntityScripts)
 * that declares a property of this type is automatically opted in: the pools reflection-scan the
 * instance for FCk_Pool_PoolableReceiver properties (same detection as FCk_Handle_ContextReceiver)
 * and fire the hooks on every acquire/release.
 *
 * - OnAcquiredFromPool: per-use (re)initialization. ObjectPool: fires AFTER the generic thaw and
 *   AFTER ICk_ObjectPool_Poolable::PrepareForUse. EntityPool: fires on the EntityScript instance
 *   when the pooled entity is delivered, after the OnAcquiredFromPool entity signal.
 * - OnReleasedToPool: quiescence — reset per-use state. ObjectPool: fires BEFORE the generic
 *   freeze, after PrepareForPool. EntityPool: fires after the OnReleasedToPool entity signal.
 * - _CanBePooled: per-instance veto read at release time. False = the pool DESTROYS the instance
 *   instead of storing it (mirrors ICk_ObjectPool_Poolable::Get_CanBePooled).
 *
 * Bind where the instance gets its one-time init: actor BeginPlay (pool-spawned actors BeginPlay
 * once, at hidden spawn), EntityScript Construct. All mutation goes through
 * UCk_Utils_PoolableReceiver_UE.
 */
USTRUCT(BlueprintType)
struct CKPOOL_API FCk_Pool_PoolableReceiver
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Pool_PoolableReceiver);

    friend class UCk_Utils_PoolableReceiver_UE;

private:
    UPROPERTY()
    bool _CanBePooled = true;

    FCk_Delegate_PoolableReceiver_OnAcquired_MC _OnAcquiredFromPool;
    FCk_Delegate_PoolableReceiver_OnReleased_MC _OnReleasedToPool;

public:
    CK_PROPERTY(_CanBePooled);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_PoolableReceiver_OnAcquired,
    FInstancedStruct, InPerUseParams);

DECLARE_DYNAMIC_DELEGATE(
    FCk_Delegate_PoolableReceiver_OnReleased);

// --------------------------------------------------------------------------------------------------------------------
