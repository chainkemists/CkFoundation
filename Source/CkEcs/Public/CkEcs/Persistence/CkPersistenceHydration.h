#pragma once

// Load-path hydration bookkeeping: the save/load path fills FFragment_PendingHydration and
// FProcessor_Hydration_Dispatch drains it through the registered HydrationApply. Transport-neutral (no Net/ dep).

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Tag/CkTag_HydrationQuarantine.h" // FTag_Hydration_Quarantine / FCtx_HydrationQuarantine

#include <InstancedStruct.h>
#include "UObject/Object.h"
#include "UObject/StrongObjectPtr.h"

#include "CkPersistenceHydration.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// GC-traced carrier for deferred hydration payloads: FInstancedStruct only traces its script type and nested
// UObject refs inside a reflected graph, which an ECS fragment is not (the fragment below pins this holder).
UCLASS()
class CKECS_API UCk_PendingHydrationPayloads_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PendingHydrationPayloads_UE);

public:
    auto Add(FInstancedStruct InEntry) -> void;
    auto Get_Entries() -> TArray<FInstancedStruct>&;
    auto Get_Entries() const -> const TArray<FInstancedStruct>&;

private:
    UPROPERTY()
    TArray<FInstancedStruct> _Entries;
};

// --------------------------------------------------------------------------------------------------------------------

// Carries WHICH type was hydrated, never the data — the same shape, and the same reason, as
// FCk_DynamicFragment_RepNotifyInfo: a payload struct copied into a delegate frame goes stale, so the handler
// reads the value off the entity.
USTRUCT(BlueprintType)
struct CKECS_API FCk_Hydration_TypeInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Ck|Hydration")
    TObjectPtr<UScriptStruct> HydratedType = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------

// Fires once per entity, after every mapped entity's payloads have applied. Bind it through
// UCk_Utils_Snapshot_UE::Promise_OnHydrated rather than the raw signal — the promise is what guarantees a fire.
DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Hydration_OnHydrated,
    FCk_Handle, InHandle);

// Fires once per hydrated TYPE per entity, after every value in that entity's payload set is committed.
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Hydration_OnTypeHydrated,
    FCk_Handle, InHandle,
    FCk_Hydration_TypeInfo, InInfo);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The load-path counterpart to net-received container entries; drained by FProcessor_Hydration_Dispatch.
    CK_DEFINE_ECS_TAG(FTag_Hydration_PendingApply);

    // --------------------------------------------------------------------------------------------------------------------

    // The load-path twin of the net path's OnConstructed -> OnReplicationComplete pair, and the reason both live
    // here rather than in CkSnapshot: the edge belongs to whoever knows the payload set is complete, and that is
    // the framework, not each handler. Handlers keep emitting their own semantic edges (an attribute's
    // OnValueChanged, an inventory's OnItemsChanged) because those carry MEANING; these carry TIMING.
    //
    // Per ENTITY, broadcast at the global quarantine lift — so a subscriber sees a set that is entirely hydrated,
    // its own entity and every sibling alike. Exactly once per entity per load: the lift broadcasts only for the
    // entities whose quarantine tag it actually removed, and the two bounded escapes release through the same
    // path, so a forced entity still gets its edge (with its loss already recorded in the load report).
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        Hydration_OnHydrated,
        FCk_Delegate_Hydration_OnHydrated,
        FCk_Handle);

    // Per TYPE, broadcast by the hydration handler that committed it once every value in that entity's payload
    // set is committed. This is the load-path half of what used to be broadcast as DynamicFragment_OnRepNotify:
    // a load is not replication, and naming it as if it were is what let consumers bind one intent and receive
    // the other. The net path keeps OnRepNotify.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        Hydration_OnTypeHydrated,
        FCk_Delegate_Hydration_OnTypeHydrated,
        FCk_Handle,
        FCk_Hydration_TypeInfo);

    // Per-load tally of what the dispatcher actually DID with each payload entry, keyed into entt::registry::ctx()
    // like FCtx_HydrationQuarantine. It lives here, written by CkEcs, because the apply outcome is only knowable
    // where the apply happens; CkSnapshot owns the lifetime (zeroed once per load) and reads it once at the fold.
    //
    // Counted in ENTRIES, not entities — the load report partitions the save's payload rows, and one entity can
    // carry many. The read is deliberately once-and-final: the dispatcher is not load-gated, so it keeps draining
    // after a load goes Idle, and those post-fold outcomes are logged rather than retro-counted into a report
    // that has already been handed to consumers.
    // One payload that did not apply, named where the fact exists. A count tells a consumer that its world came
    // back incomplete; only a name tells it WHICH part.
    struct CKECS_API FHydration_LossRecord
    {
        FString _PayloadType;
        FString _OwnerIdentity;
        // "no-handler" | "rejected" | "timed-out" | "destroyed-with-entries"
        FString _Reason;
    };

    // A pathological load must not grow this without bound; past the cap the counts still tell the whole story.
    constexpr auto MaxRecordedHydrationLosses = 64;

    struct CKECS_API FCtx_HydrationOutcomes
    {
        int32 _Applied              = 0;
        int32 _Rejected             = 0;
        // The handler-less path: the save recorded state this build cannot apply. Its own bucket rather than the
        // timeout's, because nothing waited.
        int32 _DroppedNoHandler     = 0;
        int32 _DroppedTimeout       = 0;
        // Entries that died with their entity mid-load. Counted where destruction begins, because after that the
        // fragment holding them is gone and nothing downstream can tell they ever existed.
        int32 _DestroyedWithEntries = 0;

        // The losses above, named — capped at MaxRecordedHydrationLosses.
        TArray<FHydration_LossRecord> _Losses;

        auto Record_Loss(FString InPayloadType, FString InOwnerIdentity, FString InReason) -> void
        {
            if (_Losses.Num() >= MaxRecordedHydrationLosses)
            { return; }

            _Losses.Emplace(FHydration_LossRecord{MoveTemp(InPayloadType), MoveTemp(InOwnerIdentity), MoveTemp(InReason)});
        }
    };

    // Payloads to apply via the registered HydrationApply, sourced from a save load. Never persisted itself.
    struct CKECS_API FFragment_PendingHydration
    {
        CK_GENERATED_BODY(FFragment_PendingHydration);

    private:
        TStrongObjectPtr<UCk_PendingHydrationPayloads_UE> _Payloads;

    public:
        float _PendingForSeconds = 0.0f;

    public:
        auto Enqueue(UObject* InOuter, FInstancedStruct InEntry) -> void;
        auto Get_Entries() -> TArray<FInstancedStruct>&;
        auto Get_Entries() const -> const TArray<FInstancedStruct>&;
    };
}

// --------------------------------------------------------------------------------------------------------------------
