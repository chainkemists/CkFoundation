#include "CkReplicatedFragmentContainer.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

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

        const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->NetRemove)
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

    // Pure bookkeeping: the dispatcher applies after composition, and PostLink re-marks pre-link receives
    for (const auto& Index : InAddedIndices)
    {
        auto& Entry = _Items[Index];

        const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->NetApply)
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

        const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->NetApply)
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
