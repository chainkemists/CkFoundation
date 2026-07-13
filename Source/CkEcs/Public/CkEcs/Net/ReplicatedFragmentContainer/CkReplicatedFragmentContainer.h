#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h" // ECk_AddedOrNot (SeedContainer result)

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"

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

    // Set on an entity that has local (save-load) hydration payloads queued for Apply — the load-path
    // counterpart to net-received container entries. Drained by FProcessor_Hydration_Dispatch. DORMANT in
    // Phase 1: nothing enqueues here yet; Phase 3B's load path fills the queue below.
    CK_DEFINE_ECS_TAG(FTag_Hydration_PendingApply);

    // Local hydration queue: payloads to apply on this entity via the SAME handler Apply the net dispatcher
    // uses, but sourced from a save load rather than the wire. NOT snapshotable (transient bookkeeping — never
    // registered). Dormant in Phase 1.
    struct FFragment_PendingHydration
    {
        CK_GENERATED_BODY(FFragment_PendingHydration);

    public:
        TArray<FInstancedStruct> _Entries;
        float _PendingForSeconds = 0.0f;
    };
}

// --------------------------------------------------------------------------------------------------------------------

// RAII scope that FProcessor_Hydration_Dispatch holds around each save-load Apply call, so a save-transport handler's
// Apply can distinguish a LOAD-PATH hydration apply (authority-side — must re-drive local state, e.g. the SM redrive)
// from an ordinary net receive (client-side). Game-thread only: the dispatchers run serially on the game thread, so a
// plain static depth counter is race-free (no thread-local needed). Additive + inert until a handler queries
// Get_IsActive() — the net Apply path never enters this scope, so net behavior is byte-identical.
class CKECS_API FCk_HydrationApplyScope
{
public:
    FCk_HydrationApplyScope();
    ~FCk_HydrationApplyScope();

    FCk_HydrationApplyScope(const FCk_HydrationApplyScope&) = delete;
    auto operator=(const FCk_HydrationApplyScope&) -> FCk_HydrationApplyScope& = delete;

    static auto
    Get_IsActive() -> bool;

private:
    static int32 _Depth;
};

// --------------------------------------------------------------------------------------------------------------------

// Result of FHandler::Apply. NotReady means the feature the data targets is not composed on this
// entity yet — the entry stays pending and the dispatcher retries next tick.
enum class ECk_RepFragment_ApplyResult : uint8
{
    Applied,
    NotReady
};

// --------------------------------------------------------------------------------------------------------------------

// Which persistence pipelines consult a handler. Net-only is the default (Phase-1 behavior-neutral); features flip
// Save on when their payload becomes save-file surface (Phase 3+). Bit flags so a handler can serve both.
enum class ECk_PersistenceTransport : uint8
{
    Net        = 1 << 0,
    Save       = 1 << 1,
    NetAndSave = Net | Save
};

// --------------------------------------------------------------------------------------------------------------------
// Handler Registry — generic callback dispatch by UScriptStruct* type

class CKECS_API FCk_ReplicatedFragmentHandlerRegistry
{
public:
    struct FHandler
    {
        // Called by FProcessor_ReplicatedFragments_Dispatch after OnConstructed-driven composition,
        // never inline during net receive. OldData is unset on the first application of this entry
        // and otherwise holds the last APPLIED data (coalesced receives diff against it). Return
        // NotReady to retry next tick (composition not done yet) — never compose the feature from
        // inside Apply.
        TFunction<ECk_RepFragment_ApplyResult(FCk_Handle& Entity,
                        const FInstancedStruct& NewData,
                        const TOptional<FInstancedStruct>& OldData)> Apply;

        // Optional — dispatched (deferred) when the entry is removed by replication.
        TFunction<void(FCk_Handle& Entity)> Remove;

        // Authority-side counterpart to Apply: emit this feature's current hydration payload for the
        // entity, or unset when the feature is absent on it. Used by the restore re-drive (Phase 1),
        // the fidelity oracle (Tier-2), and the save path (Phase 3A). READ-ONLY by contract — never
        // mutate the entity from Produce. A SET-but-empty payload is meaningful (it seeds an empty
        // container, e.g. AnimPlan); only an UNSET result means "feature absent, do not seed".
        TFunction<TOptional<FInstancedStruct>(FCk_Handle& Entity)> Produce;

        // Which pipelines consult this handler. Net-only default keeps Phase 1 behavior-neutral;
        // features flip Save on when their payload becomes save-file surface (Phase 3+).
        ECk_PersistenceTransport Transport = ECk_PersistenceTransport::Net;
    };

    using FTypeResolver = TFunction<UScriptStruct*()>;

    /** Immediate registration — use only when the UScriptStruct* is known to be ready. */
    static auto
    Register(
        const UScriptStruct* InType,
        FHandler InHandler) -> void;

    /** Lazy registration — type is resolved on first Find(). Safe to call during static init. */
    static auto
    RegisterLazy(
        FTypeResolver InTypeResolver,
        FHandler InHandler) -> void;

    /**
     * Typed lazy registration. Resolves the payload type lazily via T_RepData::StaticStruct(), then forwards to
     * RegisterLazy. Body lives in CkReplicatedFragmentContainer.inl.h so it instantiates where T_RepData is complete.
     */
    template <typename T_RepData>
    static auto
    RegisterLazyTyped(
        FHandler InHandler) -> void;

    /**
     * Resolves pending registrations, then returns the payload types of every handler that has a Produce AND whose
     * Transport opts into Save — the subset the v3 save capture writes payloads for (Phase 3A). A Net-only Produce
     * handler is save-invisible; a NetAndSave one participates in both the wire and the save file.
     */
    static auto
    Get_SaveHandlerTypes() -> TArray<const UScriptStruct*>;

    /**
     * Register a single catch-all handler consulted by Resolve() when no per-type handler matches.
     * Used by runtime-typed features (e.g. dynamic fragments) whose payload UScriptStruct is not
     * known at compile time, so per-type registration is impossible. The fallback only ever sees
     * types that nobody registered explicitly — registered types always win via Find().
     */
    static auto
    RegisterFallback(
        FHandler InHandler) -> void;

    static auto
    Find(
        const UScriptStruct* InType) -> const FHandler*;

    /** Find() the per-type handler, else the registered fallback (or nullptr if neither exists). */
    static auto
    Resolve(
        const UScriptStruct* InType) -> const FHandler*;

private:
    static auto
    ResolvePending() -> void;

    struct FLazyEntry
    {
        FTypeResolver TypeResolver;
        FHandler Handler;
    };

    static TMap<const UScriptStruct*, FHandler> _Handlers;
    static TArray<FLazyEntry> _PendingHandlers;
    static TOptional<FHandler> _Fallback;
};

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
