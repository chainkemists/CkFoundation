#pragma once

#include "CkCore/Macros/CkMacros.h"

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
}

// --------------------------------------------------------------------------------------------------------------------

// Result of FHandler::Apply. NotReady means the feature the data targets is not composed on this
// entity yet — the entry stays pending and the dispatcher retries next tick.
enum class ECk_RepFragment_ApplyResult : uint8
{
    Applied,
    NotReady
};

// --------------------------------------------------------------------------------------------------------------------
// Handler Registry — generic callback dispatch by UScriptStruct* type

class CKECS_API FCk_ReplicatedFragmentHandlerRegistry
{
public:
    struct FHandler
    {
        // ---- New contract (deferred dispatch) ----
        // Called by FProcessor_ReplicatedFragments_Dispatch after OnConstructed-driven composition,
        // never inline during net receive. OldData is unset on the first application of this entry.
        // Return NotReady to retry next tick (composition not done yet) — never compose the feature
        // from inside Apply.
        TFunction<ECk_RepFragment_ApplyResult(FCk_Handle& Entity,
                        const FInstancedStruct& NewData,
                        const TOptional<FInstancedStruct>& OldData)> Apply;

        TFunction<void(FCk_Handle& Entity)> Remove;

        // ---- Legacy contract (inline dispatch during net receive / PostLink replay) ----
        // Migration in progress: a handler defines EITHER Apply/Remove OR the three below. Types
        // whose handler defines Apply are routed exclusively through the deferred dispatcher.
        TFunction<void(FCk_Handle& Entity,
                        const FInstancedStruct& NewData,
                        const FInstancedStruct& OldData)> OnChange;

        TFunction<void(FCk_Handle& Entity,
                        const FInstancedStruct& Data)> OnAdd;

        TFunction<void(FCk_Handle& Entity)> OnRemove;
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

    // Enforces the OnChange => OnAdd invariant. Initial replication always arrives as an Add, so a
    // handler that reacts to changes but not adds would silently drop the first replicated value.
    static auto
    DoValidateHandler(
        const UScriptStruct* InType,
        const FHandler& InHandler) -> void;

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

    // Client-side previous data for change detection (NOT replicated)
    FInstancedStruct _PreviousData;

    // ---- Client-local deferred-dispatch state (NOT replicated) ----
    // Set on receive/link, cleared by FProcessor_ReplicatedFragments_Dispatch once Apply succeeds.
    bool _PendingApply = false;

    // Accumulated while Apply keeps returning NotReady; past the timeout the entry is dropped LOUDLY.
    float _PendingForSeconds = 0.0f;

    // Last data successfully applied on this client — the Old side of the next Apply. Distinct from
    // _PreviousData (last RECEIVED): coalesced receives must diff against what was actually applied.
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
