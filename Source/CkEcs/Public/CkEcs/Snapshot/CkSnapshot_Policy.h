#pragma once

// --------------------------------------------------------------------------------------------------------------------
// Snapshot classification policy for entity-handle-bearing fragment families (EntityHolder, RecordOfEntities).
//
// INERT pending Option A (see docs/campaigns/saveload-v3-parity/PHASE_6.md): the classification feeds nothing;
// CK_REGISTER_SNAPSHOTABLE and the capture audit were deleted with Model A. The policy tag types and the marker
// below still compile — T_Policy remains a REQUIRED template parameter, so a holder/record must still name a
// classification (omitting it is a compile error) — but nothing consumes the resulting IsSnapshotable marker.
// Retained as-is until Phase 6 retires the macro surface.
//
//   RoundTrip  — was: authoritative state nothing reconstructs on restore (e.g. SceneNode parent/child links).
//   Transient  — was: derived/runtime state reconstructed on restore (StateMachine graph, in-flight requests, ...).
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FSnapshotPolicy_RoundTrip {};
    struct FSnapshotPolicy_Transient {};

    namespace detail
    {
        // Injects the IsSnapshotable opt-in marker ONLY for the RoundTrip policy (empty base, EBO — no layout cost).
        // INERT: its consumer, ck::concepts::FragmentIsSnapshotable, was deleted with Model A, so nothing reads the
        // typedef. Retained so T_Policy classification still compiles pending the Phase 6 macro retirement.
        template <typename T_Policy>
        struct TSnapshotMarker {};

        template <>
        struct TSnapshotMarker<FSnapshotPolicy_RoundTrip>
        {
            using IsSnapshotable = void;
        };
    }
}

// --------------------------------------------------------------------------------------------------------------------
