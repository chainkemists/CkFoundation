#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot::runtime_spawn_policy
{
    constexpr auto k_NoOwnerSavedEntity = 0xFFFFFFFFu;

    // RuntimeSpawned non-bridged entities whose lifetime owner is not in the saved table are rebuilt under the new
    // world's transient root. That is allowed only when the entity's current EntityScript CDO explicitly opts in;
    // otherwise the fresh world's boot remains the producer and the saved row must be skipped to avoid a duplicate.
    inline auto
        CanRebuildRuntimeSpawnedWithOwnerPolicy(
            bool                InIsSnapshotRespawnable,
            uint32              InOwnerSavedId,
            const TSet<uint32>& InPersistedIds)
        -> bool
    {
        const auto OwnerIsNotPersisted =
            InOwnerSavedId != k_NoOwnerSavedEntity && NOT InPersistedIds.Contains(InOwnerSavedId);
        return NOT OwnerIsNotPersisted || InIsSnapshotRespawnable;
    }
}

// --------------------------------------------------------------------------------------------------------------------
