#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkPool/CkPool_Common.h"

#include <GameplayTagContainer.h>
#include <StructUtils/InstancedStruct.h>
#include <Templates/SubclassOf.h>

#include "CkEntityPool_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPOOL_API FCk_Handle_EntityPool : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityPool); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityPool);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_Fragment_EntityPool_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EntityPool_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _EntityScriptClass;

    // Optional. Named pools allow multiple differently-configured pools of the same EntityScript class.
    // A pool with no name is the class's DEFAULT pool (the one Request_Acquire auto-creates/targets)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "EntityPool"))
    FGameplayTag _PoolName;

    // Passed ONCE to each instance's Construct at creation (prewarm/grow). Per-acquire data does NOT go here —
    // it rides the OnAcquiredFromPool signal payload (see FCk_Delegate_EntityPool_EntityAcquired)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FInstancedStruct _ConstructionSpawnParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _PrewarmCount = 0;

    // Instances spawned per tick while prewarming — amortizes warm-up cost across frames so a large pool
    // does not hitch the frame it is created on
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _PrewarmBudgetPerTick = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Pool_CapacityPolicy _CapacityPolicy = ECk_Pool_CapacityPolicy::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1", EditCondition = "_CapacityPolicy == ECk_Pool_CapacityPolicy::Bounded"))
    int32 _MaxSize = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Pool_ExhaustionPolicy _ExhaustionPolicy = ECk_Pool_ExhaustionPolicy::Grow;

public:
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY(_PoolName);
    CK_PROPERTY(_ConstructionSpawnParams);
    CK_PROPERTY(_PrewarmCount);
    CK_PROPERTY(_PrewarmBudgetPerTick);
    CK_PROPERTY(_CapacityPolicy);
    CK_PROPERTY(_MaxSize);
    CK_PROPERTY(_ExhaustionPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_EntityPool_ParamsData, _EntityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_EntityPool_AcquireResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityPool_AcquireResult);

private:
    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_EntityPool _Pool;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_SucceededFailed _Result = ECk_SucceededFailed::Failed;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _AcquiredEntity;

public:
    CK_PROPERTY_GET(_Pool);
    CK_PROPERTY_GET(_Result);
    CK_PROPERTY_GET(_AcquiredEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_EntityPool_AcquireResult, _Pool, _Result, _AcquiredEntity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_EntityPool_Stats
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityPool_Stats);

private:
    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumDormant = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumInUse = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumLiveInstances = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumSpawnsInFlight = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumPendingAcquires = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _HighWaterMark = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumHits = 0;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _NumMisses = 0;

public:
    CK_PROPERTY(_NumDormant);
    CK_PROPERTY(_NumInUse);
    CK_PROPERTY(_NumLiveInstances);
    CK_PROPERTY(_NumSpawnsInFlight);
    CK_PROPERTY(_NumPendingAcquires);
    CK_PROPERTY(_HighWaterMark);
    CK_PROPERTY(_NumHits);
    CK_PROPERTY(_NumMisses);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPOOL_API FCk_Handle_PendingEntityPoolAcquire
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_PendingEntityPoolAcquire);

private:
    UPROPERTY()
    FCk_Handle _TicketEntity;

public:
    CK_PROPERTY_GET(_TicketEntity);

    CK_DEFINE_CONSTRUCTORS(FCk_Handle_PendingEntityPoolAcquire, _TicketEntity);
};

CK_DECLARE_CUSTOM_IS_VALID(CKPOOL_API, FCk_Handle_PendingEntityPoolAcquire, IsValid_Policy_Default);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_EntityPool_Acquired,
    FCk_EntityPool_AcquireResult, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_EntityPool_EntityAcquired,
    FCk_Handle, InPooledEntity,
    FInstancedStruct, InPerUseParams);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_EntityPool_EntityReleased,
    FCk_Handle, InPooledEntity);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_EntityPool_Exhausted,
    FCk_Handle_EntityPool, InPool);

// --------------------------------------------------------------------------------------------------------------------
