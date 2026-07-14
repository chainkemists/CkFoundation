#pragma once

#include "CkEcs/Concepts/CkConcepts.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

// SerializeSnapshot routes the held entity handle through FSnapshotContext::Snapshot_Handle, so the complete
// FSnapshotContext type is required here (template body lives in the header).
#include "CkEcs/Snapshot/CkSnapshot_Context.h"

class FArchive;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Holds a single entity handle on the owning entity, remapped through FSnapshotContext on save/load. The former
    // T_Policy snapshot-classification parameter (RoundTrip/Transient) was removed with the Model-A purge (Option A,
    // saveload-v3-ergonomics Phase 2) — it fed nothing; the IsSnapshotable marker had no consumer left.
    template <concepts::ValidHandleType T_HandleType>
    struct TFragment_EntityHolder
    {
    public:
        CK_GENERATED_BODY(TFragment_EntityHolder<T_HandleType>);

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

// The single canonical define (the former policy-parameterized macro split is collapsed — the snapshot
// policy was inert, deleted with Model A). _ROUNDTRIP / _TRANSIENT are retained as thin aliases so existing call
// sites are untouched (Option A, saveload-v3-ergonomics Phase 2); they carry no meaning now.
#define CK_DEFINE_ENTITY_HOLDER(_NameOfEntityHolder_, _HandleType_)              \
struct _NameOfEntityHolder_ : public ck::TFragment_EntityHolder<_HandleType_>    \
{                                                                                \
    using TFragment_EntityHolder::TFragment_EntityHolder;                        \
}

#define CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP(_NameOfEntityHolder_, _HandleType_) \
    CK_DEFINE_ENTITY_HOLDER(_NameOfEntityHolder_, _HandleType_)

#define CK_DEFINE_ENTITY_HOLDER_TRANSIENT(_NameOfEntityHolder_, _HandleType_) \
    CK_DEFINE_ENTITY_HOLDER(_NameOfEntityHolder_, _HandleType_)

// --------------------------------------------------------------------------------------------------------------------
