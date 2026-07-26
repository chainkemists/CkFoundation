#pragma once

// Load-path hydration bookkeeping: the save/load path fills FFragment_PendingHydration and
// FProcessor_Hydration_Dispatch drains it through the registered HydrationApply. Transport-neutral (no Net/ dep).

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

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

namespace ck
{
    // The load-path counterpart to net-received container entries; drained by FProcessor_Hydration_Dispatch.
    CK_DEFINE_ECS_TAG(FTag_Hydration_PendingApply);

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
