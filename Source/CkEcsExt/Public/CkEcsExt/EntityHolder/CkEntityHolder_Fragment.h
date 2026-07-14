#pragma once

#include "CkEcs/Concepts/CkConcepts.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

// SerializeSnapshot routes the held entity handle through FSnapshotContext::Snapshot_Handle, so the complete
// FSnapshotContext type is required here (template body lives in the header).
#include "CkEcs/Snapshot/CkSnapshot_Context.h"
// Snapshot classification policy (RoundTrip vs Transient) + the IsSnapshotable marker base.
#include "CkEcs/Snapshot/CkSnapshot_Policy.h"

class FArchive;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // T_Policy classifies this holder for snapshot: ck::FSnapshotPolicy_RoundTrip or ck::FSnapshotPolicy_Transient.
    // INERT pending Option A (see docs/campaigns/saveload-v3-parity/PHASE_6.md): the classification feeds nothing;
    // CK_REGISTER_SNAPSHOTABLE and the capture audit were deleted with Model A. Both template params are still
    // REQUIRED — omitting the classification is a compile error — so use the CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP /
    // _TRANSIENT macros, which supply the policy.
    template <concepts::ValidHandleType T_HandleType, typename T_Policy>
    struct TFragment_EntityHolder : public detail::TSnapshotMarker<T_Policy>
    {
    public:
        CK_GENERATED_BODY(TFragment_EntityHolder<T_HandleType COMMA T_Policy>);

    public:
        template <typename>
        friend class TUtils_EntityHolder;

    public:
        using EntityType = T_HandleType;

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

    private:
        CK_PROPERTY_GET_NON_CONST(_Entity);

    public:
        CK_DEFINE_CONSTRUCTORS(TFragment_EntityHolder, _Entity);

    public:
        // Tier-C: holds a single entity handle, remapped through FSnapshotContext.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void
        {
            InCtx.Snapshot_Handle(InAr, _Entity);
        }
    };

}

// Policy-blind define is removed — every holder must classify its snapshot policy. Using it is now a hard error.
#define CK_DEFINE_ENTITY_HOLDER(_NameOfEntityHolder_, _HandleType_)                                          \
    static_assert(false, "CK_DEFINE_ENTITY_HOLDER is removed: choose CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP "      \
        "(authoritative state that must survive save/load — also add CK_REGISTER_SNAPSHOTABLE in the .cpp) "  \
        "or CK_DEFINE_ENTITY_HOLDER_TRANSIENT (state reconstructed on restore).")

// Explicit-policy defines. Pick _ROUNDTRIP or _TRANSIENT (both classifications are currently INERT — see the
// T_Policy note above; CK_REGISTER_SNAPSHOTABLE was deleted with Model A).
#define CK_DEFINE_ENTITY_HOLDER_WITH_POLICY(_NameOfEntityHolder_, _HandleType_, _Policy_)   \
struct _NameOfEntityHolder_ : public ck::TFragment_EntityHolder<_HandleType_, _Policy_>     \
{                                                                                           \
    using TFragment_EntityHolder::TFragment_EntityHolder;                                   \
}

#define CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP(_NameOfEntityHolder_, _HandleType_)                                 \
    CK_DEFINE_ENTITY_HOLDER_WITH_POLICY(_NameOfEntityHolder_, _HandleType_, ck::FSnapshotPolicy_RoundTrip)

#define CK_DEFINE_ENTITY_HOLDER_TRANSIENT(_NameOfEntityHolder_, _HandleType_) \
    CK_DEFINE_ENTITY_HOLDER_WITH_POLICY(_NameOfEntityHolder_, _HandleType_, ck::FSnapshotPolicy_Transient)

// --------------------------------------------------------------------------------------------------------------------
