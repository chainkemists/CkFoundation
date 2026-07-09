#pragma once

#include <UObject/Interface.h>

#include "CkObjectPool_Poolable.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, Blueprintable)
class CKPOOL_API UCk_ObjectPool_Poolable : public UInterface
{
    GENERATED_BODY()
};

// Optional per-object pooling hooks — mirrors the engine's IMassActorPoolableInterface contract.
// The pool always does the generic freeze/thaw (actors: hidden + collision/tick off on release; visible +
// CDO-default collision/tick on acquire). Deep per-use reset — physics velocities, AI state, bound
// delegates, per-use timers — is the implementer's job in PrepareForPool/PrepareForUse.
//
// C++/BP only: AngelScript cannot implement UInterfaces. AS classes opt in by declaring an
// FCk_Pool_PoolableReceiver property instead (CkPool/Poolable/CkPoolableReceiver.h) — same hooks,
// same veto, detected by reflection. Both mechanisms may coexist; the interface fires first
class CKPOOL_API ICk_ObjectPool_Poolable
{
    GENERATED_BODY()

public:
    // Called when the object is handed out — per-use (re)initialization. Fires AFTER the generic thaw
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|ObjectPool")
    void PrepareForUse();

    // Called when the object is returned — reset per-use state. Fires BEFORE the generic freeze
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|ObjectPool")
    void PrepareForPool();

    // Per-instance veto at release time: return false to have the pool DESTROY the instance instead of
    // storing it (e.g. the object is in a state too expensive or unsafe to reset)
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|ObjectPool")
    bool Get_CanBePooled();

public:
    virtual auto PrepareForUse_Implementation() -> void {}
    virtual auto PrepareForPool_Implementation() -> void {}
    virtual auto Get_CanBePooled_Implementation() -> bool { return true; }
};

// --------------------------------------------------------------------------------------------------------------------
