#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::SnapshotRegistryType

#include "Containers/Array.h"
#include "Containers/UnrealString.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Describes every structurally-inconsistent stored handle: a dangling HARD ref (LifetimeOwner / ContextOwner),
    // or a live LifetimeDependent whose own LifetimeOwner names someone else. Unset/tombstone handles are skipped;
    // an EMPTY result means the restored entity graph's backbone is consistent. Rationale: CkSnapshot/CLAUDE.md.
    CKSNAPSHOT_API auto
    Verify_AllStoredHandlesResolve(
        ck::SnapshotRegistryType& InRegistry) -> TArray<FString>;
}
