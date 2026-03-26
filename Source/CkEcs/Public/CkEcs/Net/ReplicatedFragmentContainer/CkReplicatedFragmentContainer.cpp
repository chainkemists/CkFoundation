#include "CkReplicatedFragmentContainer.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Handler Registry

TMap<const UScriptStruct*, FCk_ReplicatedFragmentHandlerRegistry::FHandler>
    FCk_ReplicatedFragmentHandlerRegistry::_Handlers;

TArray<FCk_ReplicatedFragmentHandlerRegistry::FLazyEntry>
    FCk_ReplicatedFragmentHandlerRegistry::_PendingHandlers;

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    Register(
        const UScriptStruct* InType,
        FHandler InHandler)
    -> void
{
    _Handlers.Add(InType, MoveTemp(InHandler));
}

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    RegisterLazy(
        FTypeResolver InTypeResolver,
        FHandler InHandler)
    -> void
{
    _PendingHandlers.Add({MoveTemp(InTypeResolver), MoveTemp(InHandler)});
}

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    ResolvePending()
    -> void
{
    auto Pending = MoveTemp(_PendingHandlers);
    _PendingHandlers.Empty();

    for (auto& Entry : Pending)
    {
        if (auto* Type = Entry.TypeResolver())
        {
            _Handlers.Add(Type, MoveTemp(Entry.Handler));
        }
    }
}

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    Find(
        const UScriptStruct* InType)
    -> const FHandler*
{
    if (_PendingHandlers.Num() > 0)
    {
        ResolvePending();
    }

    return _Handlers.Find(InType);
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_ReplicatedFragmentArray

auto
    FCk_ReplicatedFragmentArray::
    PreReplicatedRemove(
        const TArrayView<int32> InRemovedIndices,
        int32 InFinalSize)
    -> void
{
    if (ck::Is_NOT_Valid(_OwningDriver))
    { return; }

    auto Entity = _OwningDriver->Get_AssociatedEntity();
    if (ck::Is_NOT_Valid(Entity))
    { return; }

    for (const auto& Index : InRemovedIndices)
    {
        auto& Entry = _Items[Index];

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Find(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || !Handler->OnRemove)
        { continue; }

        Handler->OnRemove(Entity);
    }
}

auto
    FCk_ReplicatedFragmentArray::
    PostReplicatedAdd(
        const TArrayView<int32> InAddedIndices,
        int32 InFinalSize)
    -> void
{
    if (ck::Is_NOT_Valid(_OwningDriver))
    { return; }

    auto Entity = _OwningDriver->Get_AssociatedEntity();
    if (ck::Is_NOT_Valid(Entity))
    { return; }

    // During initial replication, PostReplicatedAdd fires before construction scripts
    // have run — child entities don't exist yet so handlers would silently no-op.
    // PostLink on the driver will replay OnAdd handlers after construction completes.
    const auto IsConstructionComplete = Entity.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    for (const auto& Index : InAddedIndices)
    {
        auto& Entry = _Items[Index];
        Entry._PreviousData = Entry.Data;

        if (NOT IsConstructionComplete)
        { continue; }

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Find(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || !Handler->OnAdd)
        { continue; }

        Handler->OnAdd(Entity, Entry.Data);
    }
}

auto
    FCk_ReplicatedFragmentArray::
    PostReplicatedChange(
        const TArrayView<int32> InChangedIndices,
        int32 InFinalSize)
    -> void
{
    if (ck::Is_NOT_Valid(_OwningDriver))
    { return; }

    auto Entity = _OwningDriver->Get_AssociatedEntity();
    if (ck::Is_NOT_Valid(Entity))
    { return; }

    for (const auto& Index : InChangedIndices)
    {
        auto& Entry = _Items[Index];

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Find(Entry.Data.GetScriptStruct());
        if (Handler != nullptr && Handler->OnChange)
        {
            Handler->OnChange(Entity, Entry.Data, Entry._PreviousData);
        }

        Entry._PreviousData = Entry.Data;
    }
}

auto
    FCk_ReplicatedFragmentArray::
    NetDeltaSerialize(
        FNetDeltaSerializeInfo& InDeltaParams)
    -> bool
{
    return FastArrayDeltaSerialize<FCk_ReplicatedFragmentEntry, FCk_ReplicatedFragmentArray>(_Items, InDeltaParams, *this);
}

// --------------------------------------------------------------------------------------------------------------------
