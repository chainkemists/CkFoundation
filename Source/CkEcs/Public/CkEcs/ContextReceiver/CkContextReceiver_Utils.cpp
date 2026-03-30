#include "CkContextReceiver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    Request_InjectContext(
        FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Handle& InContextEntity)
    -> FCk_Handle_ContextReceiver
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContextEntity),
        TEXT("Cannot inject invalid context entity into ContextReceiver"))
    { return InContextReceiver; }

    if (InContextReceiver._ContextEntity == InContextEntity)
    { return InContextReceiver; }

    InContextReceiver._ContextEntity = InContextEntity;
    InContextReceiver._OnContextInjected.Broadcast(InContextReceiver, InContextEntity);

    return InContextReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    Request_ClearContext(
        FCk_Handle_ContextReceiver& InContextReceiver)
    -> FCk_Handle_ContextReceiver
{
    if (ck::Is_NOT_Valid(InContextReceiver._ContextEntity))
    { return InContextReceiver; }

    InContextReceiver._ContextEntity = FCk_Handle{};
    InContextReceiver._OnContextCleared.Broadcast(InContextReceiver);

    return InContextReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    Has_ValidContext(
        const FCk_Handle_ContextReceiver& InContextReceiver)
    -> bool
{
    return ck::IsValid(InContextReceiver.Get_ContextEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    Get_ContextEntity(
        const FCk_Handle_ContextReceiver& InContextReceiver)
    -> FCk_Handle
{
    return InContextReceiver.Get_ContextEntity();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    BindTo_OnContextInjected(
        FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextInjected& InDelegate)
    -> FCk_Handle_ContextReceiver
{
    InContextReceiver._OnContextInjected.AddUFunction(
        const_cast<UObject*>(InDelegate.GetUObject()),
        InDelegate.GetFunctionName());

    return InContextReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

// NOTE: RemoveAll removes ALL bindings from the given UObject, not just a specific function.
// This is acceptable since each object typically binds once per MC delegate.
auto
    UCk_Utils_ContextReceiver_UE::
    UnbindFrom_OnContextInjected(
        FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextInjected& InDelegate)
    -> FCk_Handle_ContextReceiver
{
    InContextReceiver._OnContextInjected.RemoveAll(InDelegate.GetUObject());

    return InContextReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    BindTo_OnContextCleared(
        FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextCleared& InDelegate)
    -> FCk_Handle_ContextReceiver
{
    InContextReceiver._OnContextCleared.AddUFunction(
        const_cast<UObject*>(InDelegate.GetUObject()),
        InDelegate.GetFunctionName());

    return InContextReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

// NOTE: RemoveAll removes ALL bindings from the given UObject, not just a specific function.
auto
    UCk_Utils_ContextReceiver_UE::
    UnbindFrom_OnContextCleared(
        FCk_Handle_ContextReceiver& InContextReceiver,
        const FCk_Delegate_ContextReceiver_OnContextCleared& InDelegate)
    -> FCk_Handle_ContextReceiver
{
    InContextReceiver._OnContextCleared.RemoveAll(InDelegate.GetUObject());

    return InContextReceiver;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    Request_UnbindAll(
        FCk_Handle_ContextReceiver& InContextReceiver,
        UObject* InBoundObject)
    -> void
{
    if (ck::Is_NOT_Valid(InBoundObject))
    { return; }

    InContextReceiver._OnContextInjected.RemoveAll(InBoundObject);
    InContextReceiver._OnContextCleared.RemoveAll(InBoundObject);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ContextReceiver_UE::
    TryInjectContextIntoObject(
        UObject* InObject,
        const FCk_Handle& InContextEntity)
    -> ECk_ContextReceiver_InjectResult
{
    if (ck::Is_NOT_Valid(InObject))
    { return ECk_ContextReceiver_InjectResult::Failed_InvalidObject; }

    if (ck::Is_NOT_Valid(InContextEntity))
    { return ECk_ContextReceiver_InjectResult::Failed_InvalidContext; }

    auto FoundAny = false;

    for (TFieldIterator<FStructProperty> PropIt(InObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
    {
        const auto* StructProp = *PropIt;

        if (StructProp->Struct != FCk_Handle_ContextReceiver::StaticStruct())
        { continue; }

        auto* ContextReceiverPtr = StructProp->ContainerPtrToValuePtr<FCk_Handle_ContextReceiver>(InObject);

        Request_InjectContext(*ContextReceiverPtr, InContextEntity);
        FoundAny = true;
    }

    if (NOT FoundAny)
    { return ECk_ContextReceiver_InjectResult::Failed_ContextReceiverPropertyNotFound; }

    return ECk_ContextReceiver_InjectResult::Success;
}

// --------------------------------------------------------------------------------------------------------------------
