#include "CkObjectPoolingParticipant_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_object_pooling_participant_utils
{
    template <typename T_Func>
    auto
        ForEach_Participant(
            const UObject* InObject,
            T_Func InFunc)
        -> bool
    {
        auto FoundAny = false;

        for (TFieldIterator<FStructProperty> PropIt(InObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            const auto* StructProp = *PropIt;

            if (StructProp->Struct != FCk_Handle_ObjectPoolingParticipant::StaticStruct())
            { continue; }

            InFunc(*StructProp->ContainerPtrToValuePtr<FCk_Handle_ObjectPoolingParticipant>(const_cast<UObject*>(InObject)));
            FoundAny = true;
        }

        return FoundAny;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    BindTo_OnAcquiredFromPool(
        FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnAcquired& InDelegate)
    -> FCk_Handle_ObjectPoolingParticipant
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDelegate.GetUObject()),
        TEXT("Cannot Bind To OnAcquiredFromPool — the supplied delegate is unbound"))
    { return InParticipant; }

    const auto BindKey = TPair<FObjectKey, FName>{InDelegate.GetUObject(), InDelegate.GetFunctionName()};

    if (InParticipant._AcquiredBindKeys.Contains(BindKey))
    { return InParticipant; }

    InParticipant._AcquiredBindKeys.Add(BindKey);
    InParticipant._OnAcquiredFromPool.AddUFunction(
        const_cast<UObject*>(InDelegate.GetUObject()),
        InDelegate.GetFunctionName());

    return InParticipant;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    UnbindFrom_OnAcquiredFromPool(
        FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnAcquired& InDelegate)
    -> FCk_Handle_ObjectPoolingParticipant
{
    const auto* BoundObject = InDelegate.GetUObject();

    InParticipant._OnAcquiredFromPool.RemoveAll(BoundObject);

    for (auto It = InParticipant._AcquiredBindKeys.CreateIterator(); It; ++It)
    {
        if (It->Key == FObjectKey{BoundObject})
        { It.RemoveCurrent(); }
    }

    return InParticipant;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    BindTo_OnReleasedToPool(
        FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnReleased& InDelegate)
    -> FCk_Handle_ObjectPoolingParticipant
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDelegate.GetUObject()),
        TEXT("Cannot Bind To OnReleasedToPool — the supplied delegate is unbound"))
    { return InParticipant; }

    const auto BindKey = TPair<FObjectKey, FName>{InDelegate.GetUObject(), InDelegate.GetFunctionName()};

    if (InParticipant._ReleasedBindKeys.Contains(BindKey))
    { return InParticipant; }

    InParticipant._ReleasedBindKeys.Add(BindKey);
    InParticipant._OnReleasedToPool.AddUFunction(
        const_cast<UObject*>(InDelegate.GetUObject()),
        InDelegate.GetFunctionName());

    return InParticipant;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    UnbindFrom_OnReleasedToPool(
        FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnReleased& InDelegate)
    -> FCk_Handle_ObjectPoolingParticipant
{
    const auto* BoundObject = InDelegate.GetUObject();

    InParticipant._OnReleasedToPool.RemoveAll(BoundObject);

    for (auto It = InParticipant._ReleasedBindKeys.CreateIterator(); It; ++It)
    {
        if (It->Key == FObjectKey{BoundObject})
        { It.RemoveCurrent(); }
    }

    return InParticipant;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    Request_UnbindAll(
        FCk_Handle_ObjectPoolingParticipant& InParticipant,
        UObject* InBoundObject)
    -> void
{
    if (ck::Is_NOT_Valid(InBoundObject))
    { return; }

    InParticipant._OnAcquiredFromPool.RemoveAll(InBoundObject);
    InParticipant._OnReleasedToPool.RemoveAll(InBoundObject);

    for (auto It = InParticipant._AcquiredBindKeys.CreateIterator(); It; ++It)
    {
        if (It->Key == FObjectKey{InBoundObject})
        { It.RemoveCurrent(); }
    }

    for (auto It = InParticipant._ReleasedBindKeys.CreateIterator(); It; ++It)
    {
        if (It->Key == FObjectKey{InBoundObject})
        { It.RemoveCurrent(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    Broadcast_AcquiredFromPool_OnObject(
        UObject* InObject)
    -> void
{
    if (ck::Is_NOT_Valid(InObject))
    { return; }

    ck_object_pooling_participant_utils::ForEach_Participant(InObject,
    [&](FCk_Handle_ObjectPoolingParticipant& InParticipant)
    {
        InParticipant._OnAcquiredFromPool.Broadcast();
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    Broadcast_ReleasedToPool_OnObject(
        UObject* InObject)
    -> void
{
    if (ck::Is_NOT_Valid(InObject))
    { return; }

    ck_object_pooling_participant_utils::ForEach_Participant(InObject,
    [&](FCk_Handle_ObjectPoolingParticipant& InParticipant)
    {
        InParticipant._OnReleasedToPool.Broadcast();
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectPoolingParticipant_UE::
    Has_Participant(
        const UObject* InObject)
    -> bool
{
    if (ck::Is_NOT_Valid(InObject))
    { return false; }

    constexpr auto NoOp = [](const FCk_Handle_ObjectPoolingParticipant&) {};
    return ck_object_pooling_participant_utils::ForEach_Participant(InObject, NoOp);
}

// --------------------------------------------------------------------------------------------------------------------
