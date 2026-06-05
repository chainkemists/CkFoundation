#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::SnapshotRegistryType + entt::basic_continuous_loader (snapshot.hpp)

class FArchive;

// --------------------------------------------------------------------------------------------------------------------
// Generic, type-erased ECS-tag snapshot driver. Lives in CkEcs (where the archive lives) so every tag-defining module
// is covered without a per-tag closure. The tag section is a self-describing blob appended AFTER the fragment passes.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Writes the tag section to InByteWriter at the current stream position: [int32 NumTagTypes], then per present tag
    // [uint32 hash][int32 count][uint32 enttId * count]. The caller records InByteWriter.Tell() into the header's
    // _TagSectionByteOffset BEFORE calling (CkEcs cannot see the header struct).
    CKECS_API auto Capture_Tags(
        ck::SnapshotRegistryType& InRegistry,
        FArchive& InByteWriter) -> void;

    // Seeks to InTagSectionByteOffset, reads the tag section, remaps each saved entity id via InLoader, and pushes
    // presence into each tag's (materialized) type-erased storage. Unknown tag hashes are consumed + skipped.
    // MUST run after the entities + fragment passes (so map() resolves) and BEFORE Loader.orphans() (so tag-only
    // entities are not culled).
    CKECS_API auto Restore_Tags(
        ck::SnapshotRegistryType& InRegistry,
        entt::basic_continuous_loader<ck::SnapshotRegistryType>& InLoader,
        FArchive& InByteReader,
        int64 InTagSectionByteOffset) -> void;

    // Type-safe clear<T> of the lifecycle/just-created markers. Always correct: these markers must never persist into
    // a loaded world. Runs immediately after Restore_Tags.
    CKECS_API auto Strip_LifecycleTags(
        ck::SnapshotRegistryType& InRegistry) -> void;
}
