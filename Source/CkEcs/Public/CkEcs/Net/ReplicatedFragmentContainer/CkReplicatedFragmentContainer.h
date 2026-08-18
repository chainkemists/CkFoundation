#pragma once

// Net WIRE plumbing only: the FastArray, its entry type, and the net-side pending tag. Transport-neutral
// persistence lives in CkEcs/Persistence/, and Net -> Persistence is the only allowed direction.

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h" // ECk_AddedOrNot

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h" // FastArray callbacks Resolve handlers

#include <InstancedStruct.h>
#include <Net/Serialization/FastArraySerializer.h>
#include <Misc/Optional.h>

#include "CkReplicatedFragmentContainer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Fragment_EntityReplicationDriver_Rep;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // On the entity while its driver holds unapplied entries or removals; FProcessor_ReplicatedFragments_Dispatch drains it
    CK_DEFINE_ECS_TAG(FTag_RepFragments_PendingApply);
}

// --------------------------------------------------------------------------------------------------------------------
// Entry — one per DataType on the owning replication driver

USTRUCT()
struct CKECS_API FCk_ReplicatedFragmentEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ReplicatedFragmentEntry);

public:
    UPROPERTY()
    FInstancedStruct Data;

    // ---- Client-local deferred-dispatch state (NOT replicated) ----
    // Set on receive/link, cleared by FProcessor_ReplicatedFragments_Dispatch once Apply succeeds.
    bool _PendingApply = false;

    // WALL-clock stamp (FPlatformTime::Seconds) of the first NotReady; 0.0 while nothing is pending. Past the
    // timeout the entry is dropped LOUDLY. Wall time, not game time: a snapshot load freezes game time for its
    // whole duration, and a watchdog that stops with the world it is watching never fires.
    double _PendingSinceRealTimeSeconds = 0.0;

    // The Old side of the next Apply: coalesced receives diff against what was APPLIED, not last received
    UPROPERTY(Transient, NotReplicated)
    FInstancedStruct _LastAppliedData;
    bool _WasEverApplied = false;
};

// --------------------------------------------------------------------------------------------------------------------
// Array — FFastArraySerializer wrapping entries

USTRUCT()
struct CKECS_API FCk_ReplicatedFragmentArray : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ReplicatedFragmentArray);

public:
    auto
    PreReplicatedRemove(
        const TArrayView<int32> InRemovedIndices,
        int32 InFinalSize) -> void;

    auto
    PostReplicatedAdd(
        const TArrayView<int32> InAddedIndices,
        int32 InFinalSize) -> void;

    auto
    PostReplicatedChange(
        const TArrayView<int32> InChangedIndices,
        int32 InFinalSize) -> void;

    auto
    NetDeltaSerialize(
        FNetDeltaSerializeInfo& InDeltaParams) -> bool;

public:
    UPROPERTY()
    TArray<FCk_ReplicatedFragmentEntry> _Items;

    UPROPERTY(NotReplicated)
    TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep> _OwningDriver = nullptr;

    // Client-local last data of replication-removed entries: removal is never dispatched inline during
    // net receive, so FProcessor_ReplicatedFragments_Dispatch resolves Remove by stored type and drains this.
    UPROPERTY(NotReplicated)
    TArray<FInstancedStruct> _PendingRemovals;
};

template<>
struct TStructOpsTypeTraits<FCk_ReplicatedFragmentArray>
    : public TStructOpsTypeTraitsBase2<FCk_ReplicatedFragmentArray>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};

// --------------------------------------------------------------------------------------------------------------------
// TFragment_ContainerEntryRef — entity-side reference to a replication driver entry

namespace ck
{
    template<typename TDataStruct>
    struct TFragment_ContainerEntryRef
    {
        CK_GENERATED_BODY(TFragment_ContainerEntryRef<TDataStruct>);

        TWeakObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep> _Driver;
        int32 _EntryIndex = INDEX_NONE;

        CK_PROPERTY_GET(_Driver);
        CK_PROPERTY_GET(_EntryIndex);
    };
}

// --------------------------------------------------------------------------------------------------------------------
