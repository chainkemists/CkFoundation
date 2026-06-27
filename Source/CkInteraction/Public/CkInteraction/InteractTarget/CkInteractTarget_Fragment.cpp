#include "CkInteractTarget_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
// The CK_REGISTER_SNAPSHOTABLE save/load closures call entt's get<T>(Archive&), so the Archive writer/reader types
// must be COMPLETE here (not just forward-declared as in the registry header).
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"
#include "Serialization/StructuredArchive.h"

#include "UObject/Class.h" // UScriptStruct::SerializeItem

// --------------------------------------------------------------------------------------------------------------------
// Snapshot registration. ck:: types hoisted to file-scope aliases (CK_REGISTER_SNAPSHOTABLE token-pastes the name;
// `::` can't be pasted). Registering Params + Current makes a restored host an InteractTarget — fixing the
// UBb_SmCondition_InteractedWith ensure (As_InteractTarget() on restore). The RecordOfInteractTargets parent->child
// link stays TRANSIENT (a separate, higher-blast change) — the SM condition reads its OWN host, not via the record.

using FSnap_InteractTarget_Params  = ck::FFragment_InteractTarget_Params;
using FSnap_InteractTarget_Current = ck::FFragment_InteractTarget_Current;

CK_REGISTER_SNAPSHOTABLE(FSnap_InteractTarget_Params);
CK_REGISTER_SNAPSHOTABLE(FSnap_InteractTarget_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_InteractTarget_Params::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    // Serialize the inner config USTRUCT by reflection (the channel is getter-only, no manual setter). SerializeItem
    // skips the native (non-UPROPERTY) delegate; the dynamic delegate + FMemberReference round-trip but are re-derived
    // at compose time, so a possibly-stale bound object is harmless — Get_InteractionChannel (the ensure path) is intact.
    FStructuredArchiveFromArchive Adapter{InAr};
    FCk_Fragment_InteractTarget_ParamsData::StaticStruct()->SerializeItem(Adapter.GetSlot(), &_Params, nullptr);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_InteractTarget_Current::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    // Persist ONLY _Enabled; the entt signal-connection map is runtime-only (excluded). No handle fields → InCtx unused.
    auto Enabled = static_cast<uint8>(_Enabled);
    InAr << Enabled;
    if (InAr.IsLoading())
    { _Enabled = static_cast<ECk_EnableDisable>(Enabled); }
}

// --------------------------------------------------------------------------------------------------------------------