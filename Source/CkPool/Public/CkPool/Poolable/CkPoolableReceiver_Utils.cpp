#include "CkPoolableReceiver_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_poolable_receiver_utils
{
    template <typename T_Func>
    auto
        ForEach_PoolableReceiver(
            const UObject* InObject,
            T_Func InFunc)
        -> bool
    {
        auto FoundAny = false;

        for (TFieldIterator<FStructProperty> PropIt(InObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            const auto* StructProp = *PropIt;

            if (StructProp->Struct != FCk_Pool_PoolableReceiver::StaticStruct())
            { continue; }

            InFunc(*StructProp->ContainerPtrToValuePtr<FCk_Pool_PoolableReceiver>(const_cast<UObject*>(InObject)));
            FoundAny = true;
        }

        return FoundAny;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    BindTo_OnAcquiredFromPool(
        FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnAcquired& InDelegate)
    -> FCk_Pool_PoolableReceiver
{
    InPoolableReceiver._OnAcquiredFromPool.AddUFunction(
        const_cast<UObject*>(InDelegate.GetUObject()),
        InDelegate.GetFunctionName());

    return InPoolableReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

// NOTE: RemoveAll removes ALL bindings from the given UObject, not just a specific function.
// This is acceptable since each object typically binds once per MC delegate.
auto
    UCk_Utils_PoolableReceiver_UE::
    UnbindFrom_OnAcquiredFromPool(
        FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnAcquired& InDelegate)
    -> FCk_Pool_PoolableReceiver
{
    InPoolableReceiver._OnAcquiredFromPool.RemoveAll(InDelegate.GetUObject());

    return InPoolableReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    BindTo_OnReleasedToPool(
        FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnReleased& InDelegate)
    -> FCk_Pool_PoolableReceiver
{
    InPoolableReceiver._OnReleasedToPool.AddUFunction(
        const_cast<UObject*>(InDelegate.GetUObject()),
        InDelegate.GetFunctionName());

    return InPoolableReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

// NOTE: RemoveAll removes ALL bindings from the given UObject, not just a specific function.
auto
    UCk_Utils_PoolableReceiver_UE::
    UnbindFrom_OnReleasedToPool(
        FCk_Pool_PoolableReceiver& InPoolableReceiver,
        const FCk_Delegate_PoolableReceiver_OnReleased& InDelegate)
    -> FCk_Pool_PoolableReceiver
{
    InPoolableReceiver._OnReleasedToPool.RemoveAll(InDelegate.GetUObject());

    return InPoolableReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Request_UnbindAll(
        FCk_Pool_PoolableReceiver& InPoolableReceiver,
        UObject* InBoundObject)
    -> void
{
    if (ck::Is_NOT_Valid(InBoundObject))
    { return; }

    InPoolableReceiver._OnAcquiredFromPool.RemoveAll(InBoundObject);
    InPoolableReceiver._OnReleasedToPool.RemoveAll(InBoundObject);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Set_CanBePooled(
        FCk_Pool_PoolableReceiver& InPoolableReceiver,
        bool InCanBePooled)
    -> FCk_Pool_PoolableReceiver
{
    InPoolableReceiver._CanBePooled = InCanBePooled;

    return InPoolableReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Get_CanBePooled(
        const FCk_Pool_PoolableReceiver& InPoolableReceiver)
    -> bool
{
    return InPoolableReceiver.Get_CanBePooled();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Broadcast_AcquiredFromPool_OnObject(
        UObject* InObject,
        const FInstancedStruct& InPerUseParams)
    -> void
{
    if (ck::Is_NOT_Valid(InObject))
    { return; }

    ck_poolable_receiver_utils::ForEach_PoolableReceiver(InObject,
    [&](FCk_Pool_PoolableReceiver& InReceiver)
    {
        InReceiver._OnAcquiredFromPool.Broadcast(InPerUseParams);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Broadcast_ReleasedToPool_OnObject(
        UObject* InObject)
    -> void
{
    if (ck::Is_NOT_Valid(InObject))
    { return; }

    ck_poolable_receiver_utils::ForEach_PoolableReceiver(InObject,
    [&](FCk_Pool_PoolableReceiver& InReceiver)
    {
        InReceiver._OnReleasedToPool.Broadcast();
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Get_CanBePooled_OnObject(
        const UObject* InObject)
    -> bool
{
    if (ck::Is_NOT_Valid(InObject))
    { return true; }

    auto CanBePooled = true;

    ck_poolable_receiver_utils::ForEach_PoolableReceiver(InObject,
    [&](const FCk_Pool_PoolableReceiver& InReceiver)
    {
        CanBePooled = CanBePooled && InReceiver.Get_CanBePooled();
    });

    return CanBePooled;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoolableReceiver_UE::
    Has_PoolableReceiver(
        const UObject* InObject)
    -> bool
{
    if (ck::Is_NOT_Valid(InObject))
    { return false; }

    constexpr auto NoOp = [](const FCk_Pool_PoolableReceiver&) {};
    return ck_poolable_receiver_utils::ForEach_PoolableReceiver(InObject, NoOp);
}

// --------------------------------------------------------------------------------------------------------------------
