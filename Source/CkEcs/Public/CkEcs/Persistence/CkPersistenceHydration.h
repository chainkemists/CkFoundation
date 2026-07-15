#pragma once

// Load-path hydration bookkeeping — split out of Net/ReplicatedFragmentContainer/ (saveload-v3-ergonomics Phase 5).
// The save/load path fills FFragment_PendingHydration; FProcessor_Hydration_Dispatch drains it through the
// registered HydrationApply. Transport-neutral (no Net/ dependency).

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include <InstancedStruct.h>
#include "UObject/Object.h"
#include "UObject/StrongObjectPtr.h"

#include "CkPersistenceHydration.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// GC-traced carrier for deferred hydration payloads. FInstancedStruct traces its script type and nested UObject
// references only when it lives in a reflected graph; an ECS fragment is not such a graph. The fragment below pins
// this holder with TStrongObjectPtr for exactly as long as any payload can remain queued across frames.
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

namespace ck
{
    // Set on an entity that has local (save-load) hydration payloads queued for Apply — the load-path
    // counterpart to net-received container entries. Drained by FProcessor_Hydration_Dispatch. The
    // load path fills the queue below.
    CK_DEFINE_ECS_TAG(FTag_Hydration_PendingApply);

    // Local hydration queue: payloads to apply on this entity via the SAME handler HydrationApply the net path's
    // sibling uses, but sourced from a save load rather than the wire. Transient bookkeeping — not persisted.
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
