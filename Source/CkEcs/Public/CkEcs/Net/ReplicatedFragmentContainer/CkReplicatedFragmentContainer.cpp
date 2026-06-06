#include "CkReplicatedFragmentContainer.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Handler Registry

TMap<const UScriptStruct*, FCk_ReplicatedFragmentHandlerRegistry::FHandler>
    FCk_ReplicatedFragmentHandlerRegistry::_Handlers;

TArray<FCk_ReplicatedFragmentHandlerRegistry::FLazyEntry>
    FCk_ReplicatedFragmentHandlerRegistry::_PendingHandlers;

TOptional<FCk_ReplicatedFragmentHandlerRegistry::FHandler>
    FCk_ReplicatedFragmentHandlerRegistry::_Fallback;

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    Register(
        const UScriptStruct* InType,
        FHandler InHandler)
    -> void
{
    DoValidateHandler(InType, InHandler);
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
        if (const auto* Type = Entry.TypeResolver())
        {
            DoValidateHandler(Type, Entry.Handler);
            _Handlers.Add(Type, MoveTemp(Entry.Handler));
        }
    }
}

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    RegisterFallback(
        FHandler InHandler)
    -> void
{
    DoValidateHandler(nullptr, InHandler);
    _Fallback = MoveTemp(InHandler);
}

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    DoValidateHandler(
        const UScriptStruct* InType,
        const FHandler& InHandler)
    -> void
{
    const auto TypeName = ck::IsValid(InType)
        ? InType->GetName()
        : FString{TEXT("<dynamic fallback>")};

    const auto HandlesChange = static_cast<bool>(InHandler.OnChange);
    const auto HandlesAdd    = static_cast<bool>(InHandler.OnAdd);

    // Invariant: reacting to changes requires reacting to the initial Add. A value cannot change on
    // a client before it has first been added, so OnChange without OnAdd can only ever drop the
    // first replicated value. (The reverse — OnAdd without OnChange — is fine for write-once data.)
    const auto OnChangeRequiresOnAdd = HandlesAdd || NOT HandlesChange;

    CK_ENSURE_IF_NOT(OnChangeRequiresOnAdd,
        TEXT("Replicated fragment handler for [{}] registers OnChange but no OnAdd. Initial "
             "replication of a fragment always arrives as an Add (PostReplicatedAdd dispatches to "
             "OnAdd, never OnChange), so the first replicated value would be silently dropped on "
             "clients and the entity would keep its default. Add an OnAdd handler that applies the "
             "initial value (typically snapping it, mirroring OnChange)."),
        TypeName)
    { }
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

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    Resolve(
        const UScriptStruct* InType)
    -> const FHandler*
{
    if (const auto* Handler = Find(InType))
    { return Handler; }

    return _Fallback.IsSet() ? &_Fallback.GetValue() : nullptr;
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

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
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

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
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

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
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
