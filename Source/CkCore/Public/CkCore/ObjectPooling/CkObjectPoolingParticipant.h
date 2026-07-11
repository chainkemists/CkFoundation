#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <UObject/ObjectKey.h>

#include "CkObjectPoolingParticipant.generated.h"

class UCk_Utils_ObjectPoolingParticipant_UE;
class UCk_ObjectPooling_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE(
    FCk_Delegate_ObjectPoolingParticipant_OnAcquired_MC);

DECLARE_MULTICAST_DELEGATE(
    FCk_Delegate_ObjectPoolingParticipant_OnReleased_MC);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Property-based pooling opt-in — the pooling sibling of FCk_Handle_ContextReceiver.
 *
 * ANY UObject-derived class (EntityScripts, plain objects) that declares a property of this type is
 * automatically opted in: the pooling subsystem reflection-scans the instance for
 * FCk_Handle_ObjectPoolingParticipant properties (same detection as FCk_Handle_ContextReceiver) and
 * fires the hooks on every acquire/release. AngelScript classes cannot implement UInterfaces
 * (Hazelight fork limitation) — this property route is the single opt-in surface for all three
 * environments.
 *
 * - OnAcquiredFromPool: per-use (re)initialization signal. Fires on EVERY vend (fresh instances
 *   simply have no binds yet), BEFORE the caller's init runs. Carries no payload — per-use data
 *   flows through the caller (synchronous acquire) or, for EntityScripts, through the normal
 *   spawn-params injection + Construct.
 * - OnReleasedToPool: quiescence — reset per-use state. Fires when the owner releases the instance,
 *   before it is stored (Recycle) or unpinned (DestroyOnRelease). Not fired when _CanBePooled vetoed
 *   the release into a destroy.
 * - _CanBePooled: per-instance veto read at release time. False = the instance is destroyed instead
 *   of stored (flip back to true before the next release to re-enable pooling).
 *
 * Recycle contract: the reset-to-archetype sweep SKIPS properties of this type, so delegates bound
 * on the instance survive recycling. Because Construct/BeginPlay re-run on a recycled EntityScript,
 * the Bind utils are idempotent per (object, function) — re-binding is a no-op, never a double-fire.
 *
 * All mutation goes through UCk_Utils_ObjectPoolingParticipant_UE.
 */
USTRUCT(BlueprintType)
struct CKCORE_API FCk_Handle_ObjectPoolingParticipant
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_ObjectPoolingParticipant);

    friend class UCk_Utils_ObjectPoolingParticipant_UE;

private:
    UPROPERTY()
    bool _CanBePooled = true;

    FCk_Delegate_ObjectPoolingParticipant_OnAcquired_MC _OnAcquiredFromPool;
    FCk_Delegate_ObjectPoolingParticipant_OnReleased_MC _OnReleasedToPool;

    // Idempotency ledgers: Construct re-runs on every acquire of a recycled instance while the
    // delegates survive recycling (the reset sweep skips this property) — a re-bind of an already
    // bound (object, function) pair must be a no-op
    TSet<TPair<FObjectKey, FName>> _AcquiredBindKeys;
    TSet<TPair<FObjectKey, FName>> _ReleasedBindKeys;

public:
    CK_PROPERTY(_CanBePooled);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE(
    FCk_Delegate_ObjectPoolingParticipant_OnAcquired);

DECLARE_DYNAMIC_DELEGATE(
    FCk_Delegate_ObjectPoolingParticipant_OnReleased);

// --------------------------------------------------------------------------------------------------------------------
