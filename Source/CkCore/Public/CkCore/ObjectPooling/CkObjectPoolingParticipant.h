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
 * Property-based pooling opt-in — the pooling sibling of FCk_Handle_ContextReceiver, reflection-scanned
 * the same way (works from C++/BP/AS; a UInterface would be unreachable from AngelScript). Gives the
 * instance payload-free OnAcquiredFromPool / OnReleasedToPool hooks. The recycle reset-to-archetype
 * sweep SKIPS this property, so binds survive recycling; binds are idempotent per (object, function)
 * because Construct re-runs per acquire. Full contract: ObjectPooling/README.md. Mutation goes through
 * UCk_Utils_ObjectPoolingParticipant_UE.
 */
USTRUCT(BlueprintType)
struct CKCORE_API FCk_Handle_ObjectPoolingParticipant
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_ObjectPoolingParticipant);

    friend class UCk_Utils_ObjectPoolingParticipant_UE;

private:
    FCk_Delegate_ObjectPoolingParticipant_OnAcquired_MC _OnAcquiredFromPool;
    FCk_Delegate_ObjectPoolingParticipant_OnReleased_MC _OnReleasedToPool;

    // idempotency ledgers — re-bind of an already-bound (object, function) is a no-op; the stored
    // handle lets Unbind remove exactly that bind (not every bind the object owns)
    TMap<TPair<FObjectKey, FName>, FDelegateHandle> _AcquiredBindHandles;
    TMap<TPair<FObjectKey, FName>, FDelegateHandle> _ReleasedBindHandles;
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE(
    FCk_Delegate_ObjectPoolingParticipant_OnAcquired);

DECLARE_DYNAMIC_DELEGATE(
    FCk_Delegate_ObjectPoolingParticipant_OnReleased);

// --------------------------------------------------------------------------------------------------------------------
