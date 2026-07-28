#pragma once

#include "CkCore/Enums/CkEnums.h"          // ECk_MinMaxCurrent
#include "CkEcs/Handle/CkHandle.h"          // FCk_Handle
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h" // ECk_Persistence_ApplyResult

#include "CkAttribute/CkAttribute_Fragment.h"      // ck::FTag_IsRefillAttribute
#include "CkAttribute/CkAttribute_Fragment_Data.h" // ECk_Attribute_RefillState

#include <InstancedStruct.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared save-only refill RUN-STATE (Running/Paused) handlers, keyed on the refill CHILD entity. See CkAttribute/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::attribute_refill_restore
{
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RefillHandle, typename T_RefillUtils, typename T_SaveData>
    auto
        Produce(
            FCk_Handle& InEntity) -> TOptional<FInstancedStruct>
    {
        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;

        // UNSET ("feature absent") unless this is a refill child entity of THIS attribute kind.
        if (NOT InEntity.Has<Current>() || NOT InEntity.Has<ck::FTag_IsRefillAttribute>())
        { return {}; }

        auto RefillHandle = T_RefillUtils::Cast(InEntity);

        auto Data = T_SaveData{};
        Data.Set_State(T_RefillUtils::Get_RefillState(RefillHandle)); // READ-ONLY tag read
        return FInstancedStruct::Make(MoveTemp(Data));
    }

    // Authority-side: the NotReady guard PRECEDES the only mutation and Pause/Resume are idempotent, so a retry cannot stack.
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
        { T_RefillUtils::Request_Pause(RefillHandle, {}); }
        else
        { T_RefillUtils::Request_Resume(RefillHandle, {}); }

        return ECk_Persistence_ApplyResult::Applied;
    }
}
