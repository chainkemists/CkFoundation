#pragma once

// --------------------------------------------------------------------------------------------------------------------
// Concepts that drive CkSnapshot's three-tier serialization dispatch.
//
// The CkSnapshot module (tier-2) consumes these concepts. They live here in CkEcs (tier-1) so a feature header
// that writes `using IsSnapshotable = void;` does not need to take a dependency edge on CkSnapshot.
//
// Three tiers:
//   - Tier A — USTRUCT with no entity-handle refs: marker only; framework dispatches to T::StaticStruct()->SerializeItem.
//   - Tier B — USTRUCT with entity-handle refs: marker + SerializeSnapshot method; handles routed via FSnapshotContext.
//   - Tier C — Non-USTRUCT (templated ck:: families): marker + SerializeSnapshot method.
//
// FSnapshotContext is forward-declared here; the full definition lives in CkEcs/Snapshot/CkSnapshot_Context.h.
// --------------------------------------------------------------------------------------------------------------------

#include <type_traits>

namespace ck
{
    class FSnapshotContext;
}

class FArchive;

namespace ck::concepts
{
    // Tier-agnostic opt-in marker. Any fragment that wants to be captured by a snapshot declares:
    //     using IsSnapshotable = void;
    template <typename T>
    concept FragmentIsSnapshotable = requires { typename T::IsSnapshotable; };

    // Tier B / C: fragment provides its own serialize body.
    // Concept body uses the C++20 `not` keyword (clearer than CkFoundation's NOT macro in template-meta contexts).
    template <typename T>
    concept FragmentHasCustomSnapshotSerialize =
        FragmentIsSnapshotable<T> &&
        requires (T& InFragment, FArchive& InAr, ck::FSnapshotContext& InCtx)
        {
            InFragment.SerializeSnapshot(InAr, InCtx);
        };

    // Tier A: USTRUCT fallback. T must have StaticStruct() (UHT-generated USTRUCT) and must NOT provide
    // its own SerializeSnapshot — that case is handled by FragmentHasCustomSnapshotSerialize first.
    template <typename T>
    concept FragmentIsUStructSnapshotable =
        FragmentIsSnapshotable<T> &&
        not FragmentHasCustomSnapshotSerialize<T> &&
        requires { T::StaticStruct(); };
}
