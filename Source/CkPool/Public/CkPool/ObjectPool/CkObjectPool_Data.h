#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPool/CkPool_Common.h"

#include <Templates/SubclassOf.h>

#include "CkObjectPool_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_ObjectPool_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPool_ParamsData);

private:
    // AActor subclasses are spawned into the world (hidden/frozen while pooled); anything else is
    // NewObject'd with the subsystem as outer. Replicated actor classes are rejected at pool creation
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UObject> _ObjectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _PrewarmCount = 0;

    // Instances created per subsystem tick while prewarming — amortizes warm-up cost across frames
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _PrewarmBudgetPerTick = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Pool_CapacityPolicy _CapacityPolicy = ECk_Pool_CapacityPolicy::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1", EditCondition = "_CapacityPolicy == ECk_Pool_CapacityPolicy::Bounded"))
    int32 _MaxSize = 32;

    // ObjectPool acquire is SYNCHRONOUS — there is no parking. Grow spawns when empty (returns null at
    // Bounded capacity); Fail returns null whenever the free list is empty
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Pool_ExhaustionPolicy _ExhaustionPolicy = ECk_Pool_ExhaustionPolicy::Grow;

    // How many instances a growth event provisions: 1 = demand-exact (default). Higher values queue
    // (GrowBatchCount - 1) EXTRA instances through the amortized prewarm tick so subsequent misses
    // become hits without a synchronous spawn. Always clamped to Bounded capacity
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1", EditCondition = "_ExhaustionPolicy == ECk_Pool_ExhaustionPolicy::Grow"))
    int32 _GrowBatchCount = 1;

public:
    CK_PROPERTY_GET(_ObjectClass);
    CK_PROPERTY(_PrewarmCount);
    CK_PROPERTY(_PrewarmBudgetPerTick);
    CK_PROPERTY(_CapacityPolicy);
    CK_PROPERTY(_MaxSize);
    CK_PROPERTY(_ExhaustionPolicy);
    CK_PROPERTY(_GrowBatchCount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ObjectPool_ParamsData, _ObjectClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_ObjectPool_Stats
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPool_Stats);

private:
    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumFree = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumInUse = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumLiveInstances = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumPrewarmRemaining = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _HighWaterMark = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumHits = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumMisses = 0;

public:
    CK_PROPERTY(_NumFree);
    CK_PROPERTY(_NumInUse);
    CK_PROPERTY(_NumLiveInstances);
    CK_PROPERTY(_NumPrewarmRemaining);
    CK_PROPERTY(_HighWaterMark);
    CK_PROPERTY(_NumHits);
    CK_PROPERTY(_NumMisses);
};

// --------------------------------------------------------------------------------------------------------------------
