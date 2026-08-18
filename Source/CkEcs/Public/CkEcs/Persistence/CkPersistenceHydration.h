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
