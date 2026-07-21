#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPool/ObjectPool/CkObjectPool_Data.h"

#include <StructUtils/InstancedStruct.h>
#include <Templates/SubclassOf.h>

#include "CkObjectPool_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Per-class bookkeeping. Both arrays are UPROPERTY so pooled objects are GC-rooted (fragments and raw
// containers are NOT GC-visible — see the CkIskmRenderer pool precedent). A slot that GC nulls out
// (externally destroyed actor) is swept lazily and counts as a "steal"
USTRUCT()
struct CKPOOL_API FCk_ObjectPool_PoolState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPool_PoolState);

    friend class UCk_ObjectPool_Subsystem_UE;

private:
    UPROPERTY(Transient)
    FCk_ObjectPool_ParamsData _Params;

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

    // registry entity mirroring this pool into ck::FFragment_RecordOfObjectPools (enumeration/tooling only —
    // the synchronous pool state above never moves to ECS)
    FCk_Handle _PoolEntity;

public:
    CK_PROPERTY_GET(_Params);
    CK_PROPERTY_GET(_PoolEntity);
};

// --------------------------------------------------------------------------------------------------------------------

// Synchronous pooling for arbitrary UObjects/AActors (the EntityPool sibling for non-entity things).
// Tickable ONLY for the amortized prewarm budget. Acquire/Release are immediate — NewObject/SpawnActor
// are synchronous and there is no ECS state to defer through (precedent: CkIskmRenderer's SKMC pool)
UCLASS()
class CKPOOL_API UCk_ObjectPool_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectPool_Subsystem_UE);

    friend class UCk_Utils_ObjectPool_UE;

public:
    auto Tick(float InDeltaTime) -> void override;

private:
    auto
    DoGetOrCreate_Pool(
        const FCk_ObjectPool_ParamsData& InParams) -> FCk_ObjectPool_PoolState*;

    auto
    DoAcquire(
        const TSubclassOf<UObject>& InObjectClass,
        const FTransform& InTransform,
        bool InHasTransform,
        const FInstancedStruct& InPerUseParams) -> UObject*;

    auto
    DoRelease(
        UObject* InObject) -> void;

    auto
    DoDestroyPool(
        const TSubclassOf<UObject>& InObjectClass) -> void;

    auto
    DoGet_Stats(
        const TSubclassOf<UObject>& InObjectClass) -> FCk_ObjectPool_Stats;

    auto
    DoGet_IsPooledObject(
        const UObject* InObject) -> bool;

private:
    auto
    DoSpawn_Instance(
        FCk_ObjectPool_PoolState& InPool) -> UObject*;

    auto
    DoDestroy_Instance(
        UObject* InObject) -> void;

    // remove GC-nulled slots (externally destroyed instances — "steal" semantics) and reconcile the
    // live count. Called lazily from acquire/release/stats, not per-tick
    auto
    DoSweep_NullSlots(
        FCk_ObjectPool_PoolState& InPool) -> void;

    static auto
    DoFreeze(
        UObject* InObject) -> void;

    static auto
    DoThaw(
        UObject* InObject,
        const FTransform& InTransform,
        bool InHasTransform) -> void;

private:
    UPROPERTY(Transient)
    TMap<TSubclassOf<UObject>, FCk_ObjectPool_PoolState> _Pools;
};

// --------------------------------------------------------------------------------------------------------------------
