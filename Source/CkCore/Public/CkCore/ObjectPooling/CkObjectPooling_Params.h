#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkObjectPooling_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ObjectPooling_RecyclePolicy : uint8
{
    // Released instances are stored and re-issued on the next acquire of the same class+archetype.
    // On re-issue, all reflected properties are reset to the creation archetype (participant
    // properties are skipped so their bound delegates survive)
    Recycle,

    // The subsystem pins the instance for its lifetime but never recycles it — release unpins and
    // lets GC collect. This is the "force create new every time" mode: callers still get subsystem
    // ownership (safe to hold TWeakObjectPtr), just no pooling
    DestroyOnRelease
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ObjectPooling_RecyclePolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ObjectPooling_CapacityPolicy : uint8
{
    Unbounded,
    Bounded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ObjectPooling_CapacityPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ObjectPooling_ExhaustionPolicy : uint8
{
    // When the pool has no free instance: create a new one (subject to the CapacityPolicy — at
    // Bounded capacity the acquire returns null)
    Grow,

    // When the pool has no free instance: return null immediately. The pool never creates
    // instances on demand
    Fail
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ObjectPooling_ExhaustionPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_ObjectPooling_PoolParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPooling_PoolParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ObjectPooling_RecyclePolicy _RecyclePolicy = ECk_ObjectPooling_RecyclePolicy::Recycle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0",
                  EditCondition = "_RecyclePolicy == ECk_ObjectPooling_RecyclePolicy::Recycle"))
    int32 _PrewarmCount = 0;

    // Instances created per subsystem tick while prewarming — amortizes warm-up cost across frames
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1",
                  EditCondition = "_RecyclePolicy == ECk_ObjectPooling_RecyclePolicy::Recycle"))
    int32 _PrewarmBudgetPerTick = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition = "_RecyclePolicy == ECk_ObjectPooling_RecyclePolicy::Recycle"))
    ECk_ObjectPooling_CapacityPolicy _CapacityPolicy = ECk_ObjectPooling_CapacityPolicy::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1",
                  EditCondition = "_CapacityPolicy == ECk_ObjectPooling_CapacityPolicy::Bounded"))
    int32 _MaxSize = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition = "_RecyclePolicy == ECk_ObjectPooling_RecyclePolicy::Recycle"))
    ECk_ObjectPooling_ExhaustionPolicy _ExhaustionPolicy = ECk_ObjectPooling_ExhaustionPolicy::Grow;

    // How many instances a growth event provisions: 1 = demand-exact (default). Higher values TOP UP
    // the queued extras to (GrowBatchCount - 1) through the amortized prewarm tick so subsequent misses
    // become hits without a synchronous spawn (a burst of misses provisions ONE batch, not one per miss).
    // Always clamped to Bounded capacity
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1",
                  EditCondition = "_ExhaustionPolicy == ECk_ObjectPooling_ExhaustionPolicy::Grow"))
    int32 _GrowBatchCount = 1;

public:
    CK_PROPERTY(_RecyclePolicy);
    CK_PROPERTY(_PrewarmCount);
    CK_PROPERTY(_PrewarmBudgetPerTick);
    CK_PROPERTY(_CapacityPolicy);
    CK_PROPERTY(_MaxSize);
    CK_PROPERTY(_ExhaustionPolicy);
    CK_PROPERTY(_GrowBatchCount);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_ObjectPooling_PoolStats
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPooling_PoolStats);

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
