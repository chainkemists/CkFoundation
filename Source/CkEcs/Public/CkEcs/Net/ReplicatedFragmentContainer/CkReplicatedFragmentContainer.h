#pragma once

// Net WIRE plumbing for replicated fragment containers — the FastArray + entry types + the net-side pending tag.
// The transport-neutral persistence machinery (registry, hydration) moved to CkEcs/Persistence/ (saveload-v3-
// ergonomics Phase 5); this header #includes the registry because its FastArray callbacks Resolve handlers.
// Net -> Persistence is the only allowed direction.

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h" // ECk_AddedOrNot

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h" // FastArray callbacks Resolve handlers (split Phase 5)

#include <InstancedStruct.h>
#include <Net/Serialization/FastArraySerializer.h>
#include <Misc/Optional.h>

#include "CkReplicatedFragmentContainer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Fragment_EntityReplicationDriver_Rep;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Set on the associated entity whenever its replication driver holds container entries (or
    // removals) that have not been applied yet. Drained by FProcessor_ReplicatedFragments_Dispatch.
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

    // Accumulated while Apply keeps returning NotReady; past the timeout the entry is dropped LOUDLY.
    float _PendingForSeconds = 0.0f;

    // Last data successfully applied on this client — the Old side of the next Apply. Coalesced
    // receives diff against what was actually applied, not the last received snapshot.
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

    // Client-local: last data of entries removed by replication whose handler speaks the new
    // contract. FProcessor_ReplicatedFragments_Dispatch resolves Remove by the stored type and
    // drains this — removal is never dispatched inline during net receive.
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
