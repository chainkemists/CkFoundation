#include "CkReplicatedFragmentContainer.h"

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
    _Fallback = MoveTemp(InHandler);
}

auto
    FCk_ReplicatedFragmentHandlerRegistry::
    Get_ReDriveHandlerTypes()
    -> TArray<const UScriptStruct*>
{
    if (_PendingHandlers.Num() > 0)
    { ResolvePending(); }

    auto Types = TArray<const UScriptStruct*>{};
    for (const auto& Pair : _Handlers)
    {
        // Participation rule: only handlers with BOTH Produce and SeedContainer are re-driven; Produce-only
        // handlers are capture/oracle-only (Phase 3A) and must never be re-seeded.
        if (Pair.Value.Produce && Pair.Value.SeedContainer)
        { Types.Add(Pair.Key); }
    }
    return Types;
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

static auto
    MarkEntryPendingApply(
        FCk_ReplicatedFragmentEntry& InEntry,
        FCk_Handle& InEntity)
    -> void
{
    InEntry._PendingApply = true;
    InEntry._PendingForSeconds = 0.0f;
    InEntity.AddOrGet<ck::FTag_RepFragments_PendingApply>();
}

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
        if (Handler == nullptr || NOT Handler->Remove)
        { continue; }

        // Removal dispatches from the deferred dispatcher, never inline during net receive.
        _PendingRemovals.Emplace(Entry.Data);
        Entity.AddOrGet<ck::FTag_RepFragments_PendingApply>();
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

    // Pure bookkeeping — the deferred dispatcher applies after composition. Pre-link receives
    // are also covered: PostLink re-marks every entry pending.
    for (const auto& Index : InAddedIndices)
    {
        auto& Entry = _Items[Index];

        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->Apply)
        { continue; }

        MarkEntryPendingApply(Entry, Entity);
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
        if (Handler == nullptr || NOT Handler->Apply)
        { continue; }

        MarkEntryPendingApply(Entry, Entity);
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
