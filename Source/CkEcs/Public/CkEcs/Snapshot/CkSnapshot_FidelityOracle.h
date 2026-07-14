#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"          // ck::SnapshotRegistryType
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"  // CK_WITH_FIDELITY_ORACLE define

// --------------------------------------------------------------------------------------------------------------------
// Fidelity oracle — Tier 1 (structural diff). Test-only: everything compiles only under CK_WITH_FIDELITY_ORACLE
// (spec §5). Captures a structural signature per entity from raw entt-registry storage iteration (needs no
// serialize path), then multiset-diffs two captures. Identity across a rebuild is BY SIGNATURE, not entity id.
//
// Lives in CkEcs so it can see the registry storage types. It therefore CANNOT reach CkLabel
// (UCk_Utils_GameplayLabel_UE) — that module depends on CkEcs, so the reverse edge is impossible. _LabelPath is
// consequently left empty here; a CkEcs-visible label hook (or a higher-tier capture step) is Phase 3B's job when
// label-keyed identity actually matters. Tier 1 only needs stable signatures, which the fragment/tag set provides.
// --------------------------------------------------------------------------------------------------------------------

#if CK_WITH_FIDELITY_ORACLE

namespace ck::snapshot_oracle
{
    // One entity's structural signature. Fields are pre-sorted so ToString() is stable.
    struct CKECS_API FCk_Oracle_EntitySignature
    {
        FString _LabelPath;                 // owner-chain of GameplayLabels, "/"-joined; empty (see file header)
        FString _ScriptClassPath;           // EntityScript class path if any, else empty
        TArray<FString> _FragmentTypeNames; // sorted; registered types use their clean display name (tags lumped here)
        TArray<FString> _TagTypeNames;      // sorted; left empty in Tier 1 (tags live in _FragmentTypeNames)

        auto ToString() const -> FString;   // stable, for diff lines
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FCk_Oracle_StructuralImage
    {
        TArray<FCk_Oracle_EntitySignature> _Entities; // sorted by ToString()
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Capture one structural image from a raw entt registry.
    //
    // InRegisteredEnttHashes (optional): when non-null, only storages whose entt type-hash is in the set contribute
    // to the signature. Model A round-trips ONLY registered-snapshotable fragments, so a baseline diff against it
    // must be taken through this filter (an unfiltered diff would legitimately show the unregistered types Model A
    // drops). Default (nullptr) = unfiltered: every storage contributes.
    CKECS_API auto Capture_Structural(
        ck::SnapshotRegistryType& InRegistry,
        const TSet<uint32>* InRegisteredEnttHashes = nullptr) -> FCk_Oracle_StructuralImage;

    // Multiset diff of signatures. Each returned line: "+ <sig>" (after-only) or "- <sig>" (before-only).
    // InAllowlist: ToString() prefixes expected to differ (Phase 3B uses this for N1 annotations); matched lines
    // go to OutAnnotated instead of the returned failure list.
    CKECS_API auto Diff_Structural(
        const FCk_Oracle_StructuralImage& InBefore,
        const FCk_Oracle_StructuralImage& InAfter,
        const TArray<FString>& InAllowlist,
        TArray<FString>& OutAnnotated) -> TArray<FString>;
}

#endif // CK_WITH_FIDELITY_ORACLE
