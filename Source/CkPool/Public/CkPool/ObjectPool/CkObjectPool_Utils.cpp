#include "CkObjectPool_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkPool/CkPool_Log.h"
#include "CkPool/ObjectPool/CkObjectPool_Subsystem.h"
#include "CkPool/Settings/CkPool_Settings.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPool_UE::
    Request_CreatePool(
        const UObject* InWorldContextObject,
        const FCk_ObjectPool_ParamsData& InParams)
    -> void
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("ObjectPool subsystem is invalid when creating a Pool"))
    { return; }

    PoolSubsystem->DoGetOrCreate_Pool(InParams);
}

auto
    UCk_Utils_ObjectPool_UE::
    Request_DestroyPool(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InObjectClass)
    -> void
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("ObjectPool subsystem is invalid when destroying the Pool of [{}]"),
        InObjectClass)
    { return; }

    PoolSubsystem->DoDestroyPool(InObjectClass);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPool_UE::
    Acquire(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InObjectClass,
        const FInstancedStruct& InPerUseParams)
    -> UObject*
{
    constexpr auto HasTransform = false;
    return DoAcquire_Common(InWorldContextObject, InObjectClass, FTransform::Identity, HasTransform, InPerUseParams);
}

auto
    UCk_Utils_ObjectPool_UE::
    Acquire_Actor(
        const UObject* InWorldContextObject,
        TSubclassOf<AActor> InActorClass,
        const FTransform& InTransform,
        const FInstancedStruct& InPerUseParams)
    -> AActor*
{
    constexpr auto HasTransform = true;
    return Cast<AActor>(DoAcquire_Common(InWorldContextObject, InActorClass, InTransform, HasTransform, InPerUseParams));
}

auto
    UCk_Utils_ObjectPool_UE::
    Release(
        UObject* InObject)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Cannot Release an INVALID object to its ObjectPool"))
    { return; }

    auto PoolSubsystem = DoGet_Subsystem(InObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("ObjectPool subsystem is invalid when Releasing [{}]"), InObject)
    { return; }

    PoolSubsystem->DoRelease(InObject);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPool_UE::
    Get_Stats(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InObjectClass)
    -> FCk_ObjectPool_Stats
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(PoolSubsystem))
    { return {}; }

    return PoolSubsystem->DoGet_Stats(InObjectClass);
}

auto
    UCk_Utils_ObjectPool_UE::
    Get_IsPooledObject(
        const UObject* InObject)
    -> bool
{
    if (ck::Is_NOT_Valid(InObject, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    auto PoolSubsystem = DoGet_Subsystem(InObject);

    if (ck::Is_NOT_Valid(PoolSubsystem))
    { return false; }

    return PoolSubsystem->DoGet_IsPooledObject(InObject);
}

auto
    UCk_Utils_ObjectPool_UE::
    Get_AllPools(
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(InWorldContextObject->GetWorld()),
        TEXT("Invalid WorldContextObject when enumerating ObjectPools"))
    { return {}; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorldContextObject->GetWorld());

    if (NOT ck::RecordOfObjectPools_Utils::Has(TransientEntity))
    { return {}; }

    return ck::RecordOfObjectPools_Utils::Get_ValidEntries(TransientEntity);
}

auto
    UCk_Utils_ObjectPool_UE::
    Get_PoolObjectClass(
        const FCk_Handle& InPoolEntity)
    -> TSubclassOf<UObject>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPoolEntity), TEXT("Invalid ObjectPool registry entity handle"))
    { return {}; }

    CK_ENSURE_IF_NOT(InPoolEntity.Has<ck::FFragment_ObjectPool_PoolInfo>(),
        TEXT("Entity [{}] is not an ObjectPool registry entity"), InPoolEntity)
    { return {}; }

    return InPoolEntity.Get<ck::FFragment_ObjectPool_PoolInfo>().Get_ObjectClass();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPool_UE::
    DoGet_Subsystem(
        const UObject* InWorldContextObject)
    -> UCk_ObjectPool_Subsystem_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("WorldContextObject is invalid when resolving the ObjectPool subsystem"))
    { return {}; }

    auto World = InWorldContextObject->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("World is invalid when resolving the ObjectPool subsystem"))
    { return {}; }

    return World->GetSubsystem<UCk_ObjectPool_Subsystem_UE>();
}

auto
    UCk_Utils_ObjectPool_UE::
    DoMake_AutoCreateParams(
        const TSubclassOf<UObject>& InObjectClass)
    -> FCk_ObjectPool_ParamsData
{
    auto Params = FCk_ObjectPool_ParamsData{InObjectClass};

    if (const auto SettingsEntry = UCk_Pool_ProjectSettings_UE::TryGet_ObjectPoolEntry(InObjectClass);
        SettingsEntry.IsSet())
    {
        Params.Set_PrewarmCount(SettingsEntry->Get_PrewarmCount())
              .Set_PrewarmBudgetPerTick(SettingsEntry->Get_PrewarmBudgetPerTick())
              .Set_CapacityPolicy(SettingsEntry->Get_CapacityPolicy())
              .Set_MaxSize(SettingsEntry->Get_MaxSize())
              .Set_ExhaustionPolicy(SettingsEntry->Get_ExhaustionPolicy());
    }

    return Params;
}

auto
    UCk_Utils_ObjectPool_UE::
    DoAcquire_Common(
        const UObject* InWorldContextObject,
        const TSubclassOf<UObject>& InObjectClass,
        const FTransform& InTransform,
        bool InHasTransform,
        const FInstancedStruct& InPerUseParams)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObjectClass), TEXT("Cannot Acquire from an ObjectPool with an INVALID class"))
    { return {}; }

    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("ObjectPool subsystem is invalid when acquiring [{}]"), InObjectClass)
    { return {}; }

    // auto-create with settings-or-default config; explicit Request_CreatePool earlier wins (GetOrCreate)
    PoolSubsystem->DoGetOrCreate_Pool(DoMake_AutoCreateParams(InObjectClass));

    return PoolSubsystem->DoAcquire(InObjectClass, InTransform, InHasTransform, InPerUseParams);
}

// --------------------------------------------------------------------------------------------------------------------
