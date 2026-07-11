#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Params.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <StructUtils/InstancedStruct.h>
#include <Templates/SubclassOf.h>
#include <UObject/ObjectKey.h>

#include "CkObjectPooling_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Pools are keyed by (class, archetype): recycled instances reset to the archetype they were created
// against, so two archetypes of one class can never share a pool. The class CDO stands in when no
// archetype is supplied
USTRUCT()
struct CKCORE_API FCk_ObjectPooling_PoolKey
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPooling_PoolKey);

private:
    UPROPERTY(Transient)
    TObjectPtr<UClass> _Class;

    UPROPERTY(Transient)
    TObjectPtr<UObject> _Archetype;

public:
    CK_PROPERTY_GET(_Class);
    CK_PROPERTY_GET(_Archetype);

    CK_DEFINE_CONSTRUCTORS(FCk_ObjectPooling_PoolKey, _Class, _Archetype);

public:
    auto operator==(const FCk_ObjectPooling_PoolKey& InOther) const -> bool
    {
        return _Class == InOther._Class && _Archetype == InOther._Archetype;
    }

    friend auto GetTypeHash(const FCk_ObjectPooling_PoolKey& InKey) -> uint32
    {
        return HashCombineFast(GetTypeHash(InKey._Class), GetTypeHash(InKey._Archetype));
    }
};

// --------------------------------------------------------------------------------------------------------------------

// Per-pool bookkeeping. Both instance arrays and the archetype are UPROPERTY so everything the pool
// owns is GC-rooted (fragments and raw containers are NOT GC-visible). A slot that GC nulls out
// (externally destroyed instance) is swept lazily and counts as a "steal"
USTRUCT()
struct CKCORE_API FCk_ObjectPooling_PoolState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPooling_PoolState);

    friend class UCk_ObjectPooling_Subsystem_UE;

private:
    UPROPERTY(Transient)
    FCk_ObjectPooling_PoolParams _Params;

    UPROPERTY(Transient)
    TObjectPtr<UClass> _Class;

    // Pinned for the pool's lifetime — the reset source for every recycle
    UPROPERTY(Transient)
    TObjectPtr<UObject> _Archetype;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> _FreeObjects;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> _InUseObjects;

    UPROPERTY(Transient)
    int32 _NumLiveInstances = 0;

    UPROPERTY(Transient)
    int32 _NumPrewarmRemaining = 0;

    UPROPERTY(Transient)
    int32 _HighWaterMark = 0;

    UPROPERTY(Transient)
    int32 _NumHits = 0;

    UPROPERTY(Transient)
    int32 _NumMisses = 0;

public:
    CK_PROPERTY_GET(_Params);
    CK_PROPERTY_GET(_Class);
    CK_PROPERTY_GET(_Archetype);
};

// --------------------------------------------------------------------------------------------------------------------

// The object-pooling core of the framework, hoisted into UCk_Utils_Object_UE::Request_CreateNewObject —
// callers go through the Utils surface, never this subsystem directly. The subsystem OWNS the lifetime
// of every instance it vends (pinned in UPROPERTY storage): Recycle-policy instances park in a pool on
// release; DestroyOnRelease instances are pinned-unique and unpinned (GC-collected) on release. Holders
// therefore keep TWeakObjectPtr, never TStrongObjectPtr. Tickable ONLY for the amortized prewarm budget.
// Plain UObjects only — actors are excluded by design (SpawnActor, not NewObject, owns actor creation)
UCLASS()
class CKCORE_API UCk_ObjectPooling_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectPooling_Subsystem_UE);

    friend class UCk_Utils_Object_UE;

public:
    auto Tick(float InDeltaTime) -> void override;

public:
    // Read-only surface for tooling (debugger inspector) — mutations go through UCk_Utils_Object_UE

    auto
    Get_PoolStats(
        const FCk_ObjectPooling_PoolKey& InKey) const -> FCk_ObjectPooling_PoolStats;

    auto
    ForEach_Pool(
        const TFunction<void(const FCk_ObjectPooling_PoolKey&, const FCk_ObjectPooling_PoolStats&)>& InFunc) const -> void;

    auto
    Get_NumVendedUnique() const -> int32;

    auto
    Get_IsVendedObject(
        const UObject* InObject) const -> bool;

private:
    // Vend an instance: recycled from the pool, freshly created on miss (Grow), or pinned-unique
    // (DestroyOnRelease). Fresh creates are outered to the subsystem's world. Returns null only on
    // Fail/at-capacity exhaustion or invalid inputs
    auto
    DoRequest_Acquire(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams,
        const FInstancedStruct& InPerUseParams) -> UObject*;

    // Recycle-policy instances: veto-check -> OnReleasedToPool -> park in the free list (veto
    // destroys instead). DestroyOnRelease instances: OnReleasedToPool -> unpin (GC collects).
    // Objects the subsystem never vended fail loudly
    auto
    DoTryReleaseToPool(
        UObject* InObject) -> ECk_SucceededFailed;

private:
    auto
    DoGetOrCreate_Pool(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams) -> FCk_ObjectPooling_PoolState*;

    auto
    DoSpawn_Instance(
        FCk_ObjectPooling_PoolState& InPool) -> UObject*;

    // Reset every reflected property to the archetype's value, SKIPPING
    // FCk_Handle_ObjectPoolingParticipant properties so bound delegates survive recycling
    static auto
    DoReset_ToArchetype(
        UObject* InObject,
        const UObject* InArchetype) -> void;

    // Remove GC-nulled slots (externally destroyed instances — "steal" semantics) and reconcile the
    // live count. Called lazily from acquire/release/stats, not per-tick
    auto
    DoSweep_NullSlots(
        FCk_ObjectPooling_PoolState& InPool) -> void;

private:
    UPROPERTY(Transient)
    TMap<FCk_ObjectPooling_PoolKey, FCk_ObjectPooling_PoolState> _Pools;

    // DestroyOnRelease vends: pinned here for their lifetime, removed (= GC-eligible) on release
    UPROPERTY(Transient)
    TSet<TObjectPtr<UObject>> _VendedUnique;

    // Reverse lookup for release — instances pinned by the pool states, not by this map
    TMap<FObjectKey, FCk_ObjectPooling_PoolKey> _InstanceToPool;
};

// --------------------------------------------------------------------------------------------------------------------
