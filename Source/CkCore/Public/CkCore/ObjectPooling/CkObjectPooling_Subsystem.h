#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Params.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <Templates/SubclassOf.h>
#include <UObject/ObjectKey.h>

#include "CkObjectPooling_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// keyed by (class, archetype) — the CDO stands in when no archetype is supplied
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

// UPROPERTY storage so the pool's instances + archetype are GC-rooted; GC-nulled slots (externally
// destroyed = "steal") are swept lazily
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

    // reset source for every recycle
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

// Owns the lifetime of every instance it hands out (pinned in UPROPERTY storage) — so holders keep
// TWeakObjectPtr, never TStrongObjectPtr. Plain UObjects only (actors go through SpawnActor).
// Usually reached through UCk_Utils_Object_UE::Request_CreateNewObject. Full model: ObjectPooling/README.md
UCLASS()
class CKCORE_API UCk_ObjectPooling_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectPooling_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto Tick(float InDeltaTime) -> void override;

protected:
    // the editor ECS world (UCk_EditorEcsWorld_Subsystem_UE) vends EntityScripts/components through
    // the pooled path too — without this subsystem those instances would fall back to caller-owned
    // creates with only weak holders (no GC root) and editor GC would collect them mid-preview
    auto DoesSupportWorldType(const EWorldType::Type InWorldType) const -> bool override;

public:
    // DestroyOnRelease creates honor InOuter (a component may need an actor outer for GetOwner()-
    // dependent systems like the nav octree); Recycle-pool creates are world-outered (a recycled
    // instance outlives its first caller). Null only on Fail/at-capacity or invalid inputs
    auto
    AcquireFromPool(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams,
        UObject* InOuter) -> UObject*;

    // Recycle -> park; DestroyOnRelease -> unpin. A benign no-op (returns Failed) for objects never
    // handed out (CDOs, non-pooled creates) — safe to call unconditionally from any teardown path
    auto
    TryReleaseToPool(
        UObject* InObject) -> ECk_SucceededFailed;

public:
    // Read-only surface for tooling (debugger inspector)

    auto
    Get_PoolStats(
        const FCk_ObjectPooling_PoolKey& InKey) const -> FCk_ObjectPooling_PoolStats;

    auto
    ForEach_Pool(
        const TFunction<void(const FCk_ObjectPooling_PoolKey&, const FCk_ObjectPooling_PoolStats&)>& InFunc) const -> void;

    auto
    Get_NumPinnedUnique() const -> int32;

    // True when this subsystem handed out (and still tracks) InObject — pooled in-use OR pinned-unique
    auto
    Get_IsTrackedObject(
        const UObject* InObject) const -> bool;

    // GC-equivalence for cross-object subscriptions: a bind site (e.g. the ECS signal Bind funnel)
    // registers a disconnect closure keyed on the tracked object its delegate targets; release runs
    // and drops the object's hooks so no subscription outlives the logical life — the same contract
    // the timer/latent-action quiesce provides. Pre-pooling, GC gave this for free (a dead object's
    // delegates compare unequal to any live re-bind); a parked instance keeps its pointer identity,
    // so without this its bindings on longer-lived entities leak into the next vend (stale delivery
    // + duplicate-signature rejection of the re-bind). False (and no-op) for untracked objects
    auto
    TryRegisterReleaseQuiesceHook(
        const UObject* InBoundObject,
        TFunction<void()> InHook) -> bool;

public:
    // The recycle reset: copy every reflected property from the archetype, skipping
    // FCk_Handle_ObjectPoolingParticipant properties (bound delegates survive), then re-instance
    // instanced subobjects so the recycled object owns fresh copies instead of aliasing the
    // archetype's (a write through an aliased subobject would corrupt the CDO for every future
    // instance). Public so the contract is directly testable; acquire calls it on every recycle
    static auto
    Request_ResetToArchetype(
        UObject* InObject,
        const UObject* InArchetype) -> void;

private:
    auto
    DoGetOrCreate_Pool(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams) -> FCk_ObjectPooling_PoolState*;

    auto
    DoSpawn_Instance(
        FCk_ObjectPooling_PoolState& InPool) -> UObject*;

    // drop GC-nulled slots (steal) + reconcile the live count; lazy, not per-tick
    auto
    DoSweep_NullSlots(
        FCk_ObjectPooling_PoolState& InPool) -> void;

    // post-GC reconciliation for externally-destroyed (stolen) instances: prune dead reverse-map
    // entries, dead pinned-unique entries, and zombie pools whose class/archetype died
    auto
    DoSweep_StaleAfterGC() -> void;

    // released == dead as far as the world is concerned: clear the object's pending world timers +
    // latent actions so nothing fires against its ended associations (pre-pooling GC gave this for free)
    auto
    DoQuiesce_ReleasedObject(
        UObject* InObject) -> void;

private:
    UPROPERTY(Transient)
    TMap<FCk_ObjectPooling_PoolKey, FCk_ObjectPooling_PoolState> _Pools;

    // DestroyOnRelease instances pinned here until release
    UPROPERTY(Transient)
    TSet<TObjectPtr<UObject>> _PinnedUnique;

    // reverse lookup for release (instances pinned by the pool states, not this map)
    TMap<FObjectKey, FCk_ObjectPooling_PoolKey> _InstanceToPool;

    // release-quiesce hooks keyed by tracked object; run + cleared by DoQuiesce_ReleasedObject,
    // pruned for stolen (externally-destroyed) instances by DoSweep_StaleAfterGC
    TMap<FObjectKey, TArray<TFunction<void()>>> _ReleaseQuiesceHooks;

    // set by the post-GC callback, consumed on the next Tick (GC callbacks must stay cheap)
    bool _StaleSweepPending = false;

    FDelegateHandle _PostGarbageCollectHandle;
};

// --------------------------------------------------------------------------------------------------------------------
