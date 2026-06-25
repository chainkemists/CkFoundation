#pragma once

// --------------------------------------------------------------------------------------------------------------------
// Snapshot classification policy for entity-handle-bearing fragment families (EntityHolder, RecordOfEntities).
//
// Every such family is structurally serializable (it knows how to remap its held handles through FSnapshotContext),
// but whether a given instantiation SHOULD be captured is a per-feature decision:
//
//   RoundTrip  — authoritative state nothing reconstructs on restore (e.g. SceneNode parent/child links). The
//                family carries the IsSnapshotable opt-in marker, so it is captured AND must be registered via
//                CK_REGISTER_SNAPSHOTABLE in a .cpp (a forgotten registration is caught by the capture-time audit).
//   Transient  — derived/runtime state reconstructed on restore (e.g. the StateMachine graph, in-flight requests,
//                aggro/sensor/interaction runtime). The family does NOT carry the marker, is never captured, and
//                CK_REGISTER_SNAPSHOTABLE on it is a COMPILE ERROR ("no serialization path") — the policy and the
//                registration cannot contradict each other.
//
// The policy is a REQUIRED template parameter on the base templates (no default in the end state): adding a holder
// or record without classifying it is a compile error. That is the whole point — it makes the SceneNode bug class
// (a silently-unclassified, silently-dropped snapshotable fragment) impossible.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FSnapshotPolicy_RoundTrip {};
    struct FSnapshotPolicy_Transient {};

    namespace detail
    {
        // Injects the tier-agnostic IsSnapshotable opt-in marker ONLY for the RoundTrip policy. A holder/record
        // base template inherits TSnapshotMarker<T_Policy>; the marker is an empty base (EBO — no layout cost) and
        // its IsSnapshotable typedef is visible on the derived fragment through the inheritance chain, satisfying
        // ck::concepts::FragmentIsSnapshotable. Transient has no specialization here, so it carries no marker.
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
