#pragma once

#include "CkCore/Enums/CkEnums.h"          // ECk_MinMaxCurrent
#include "CkEcs/Handle/CkHandle.h"          // FCk_Handle
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h" // ECk_Persistence_ApplyResult

#include "CkAttribute/CkAttribute_Fragment.h"      // ck::FTag_IsRefillAttribute
#include "CkAttribute/CkAttribute_Fragment_Data.h" // ECk_Attribute_RefillState

#include <InstancedStruct.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared save-only Produce / HydrationApply for the attribute-refill RUN-STATE (Running/Paused), used by the Float and
// Integer refill registrars. This is a SAVE-ONLY handler (Produce + HydrationApply, no net Apply) — the run-state is
// NOT a replicated container value; each world sets its own via the entity script's Construct StartingState + runtime
// Request_Pause/Resume. It therefore has no Replicate pass to re-arm on load (see CkSnapshot/Claude.md §4, the
// unreplicated case, mirrored on the Timer parity handler).
//
// The refill VALUE (fill rate) already round-trips via the attribute VALUE handler keyed on the same refill child
// entity (ck::attribute_restore::Produce/HydrationApply). Only the run-state is missing from a plain rebuild — Construct
// resurrects it to the StartingState param, discarding a runtime Pause/Resume — so this handler captures JUST the
// run-state on a distinct save-only payload keyed on the SAME refill child entity.
//
// Keyed PER-ATTRIBUTE-ENTITY: Produce/HydrationApply fire on the refill CHILD entity, which carries both the attribute
// kind's Current component AND ck::FTag_IsRefillAttribute. The Current-component check scopes the handler to its own
// attribute kind (a Float refill entity has TFragment_FloatAttribute<Current>, an Integer one does not), so the two
// per-kind instantiations never cross-fire on the same entity.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::attribute_refill_restore
{
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RefillHandle, typename T_RefillUtils, typename T_SaveData>
    auto
        Produce(
            FCk_Handle& InEntity) -> TOptional<FInstancedStruct>
    {
        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;

        // UNSET unless this is a refill child entity of THIS attribute kind — a plain (non-refill) attribute of the
        // same kind, or a refill entity of the OTHER kind, both fall through to "feature absent".
        if (NOT InEntity.Has<Current>() || NOT InEntity.Has<ck::FTag_IsRefillAttribute>())
        { return {}; }

        auto RefillHandle = T_RefillUtils::Cast(InEntity);

        auto Data = T_SaveData{};
        Data.Set_State(T_RefillUtils::Get_RefillState(RefillHandle)); // READ-ONLY tag read
        return FInstancedStruct::Make(MoveTemp(Data));
    }

    // Authority-side load apply, dispatched by FProcessor_Hydration_Dispatch. NotReady until Construct has re-composed
    // the refill child (FTag_IsRefillAttribute present) — that guard PRECEDES the only mutation, and Request_Pause/Resume
    // are idempotent (Try_Remove / AddOrGet of the run tag), so a NotReady-retry can never stack state. Re-drive is via
    // the public Request_* surface only (no friend-gated fragment writes). Nothing to re-arm: the run-state is unreplicated.
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RefillHandle, typename T_RefillUtils, typename T_SaveData>
    auto
        HydrationApply(
            FCk_Handle& InEntity,
            const FInstancedStruct& InNew) -> ECk_Persistence_ApplyResult
    {
        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;

        if (NOT InEntity.Has<Current>() || NOT InEntity.Has<ck::FTag_IsRefillAttribute>())
        { return ECk_Persistence_ApplyResult::NotReady; }

        auto RefillHandle = T_RefillUtils::Cast(InEntity);

        const auto SavedState = InNew.Get<T_SaveData>().Get_State();
        if (SavedState == ECk_Attribute_RefillState::Paused)
        { T_RefillUtils::Request_Pause(RefillHandle); }
        else
        { T_RefillUtils::Request_Resume(RefillHandle); }

        return ECk_Persistence_ApplyResult::Applied;
    }
}
