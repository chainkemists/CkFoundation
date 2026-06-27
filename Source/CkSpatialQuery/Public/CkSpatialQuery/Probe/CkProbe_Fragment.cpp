#include "CkProbe_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
// The CK_REGISTER_SNAPSHOTABLE save/load closures call entt's get<T>(Archive&), so the Archive writer/reader types
// must be COMPLETE here (not just forward-declared as in the registry header).
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------
// Snapshot registration for the persistent line-trace settings. ck:: alias hoisted (CK_REGISTER_SNAPSHOTABLE
// token-pastes the name; `::` can't be pasted). Makes a restored interaction/perception line-trace keep its config
// and re-home its _StartPos transform handle, instead of being dropped on load.

using FSnap_ProbeTrace_RayCast = ck::FFragment_ProbeTrace_RayCast;
CK_REGISTER_SNAPSHOTABLE(FSnap_ProbeTrace_RayCast);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Probe_RayCastPersistent_Settings::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    // Plain config fields serialized directly (member access; FVector + FGameplayTagContainer have FArchive operator<<;
    // enums via uint8). The _StartPos Transform handle is re-homed by the snapshot context (its raw entity id is
    // session-specific). The per-tick overlap TSet is NOT here — it's transient and re-populates on the next cast.
    InAr << _DirectionAndLength;

    // FGameplayTagContainer has no FArchive operator<< (its Serialize takes a structured slot). Round-trip its tags
    // by hand via FGameplayTag's FArchive operator<<.
    if (InAr.IsLoading())
    {
        _Filter.Reset();
        auto NumFilterTags = int32{0};
        InAr << NumFilterTags;
        for (auto Index = 0; Index < NumFilterTags; ++Index)
        {
            auto Tag = FGameplayTag{};
            InAr << Tag;
            _Filter.AddTag(Tag);
        }
    }
    else
    {
        const auto& FilterTags = _Filter.GetGameplayTagArray();
        auto NumFilterTags = FilterTags.Num();
        InAr << NumFilterTags;
        for (const auto& Tag : FilterTags)
        {
            auto TagToWrite = Tag; // operator<< takes a non-const ref
            InAr << TagToWrite;
        }
    }

    auto BackFaceTriangles = static_cast<uint8>(_BackFaceModeTriangles);
    auto BackFaceConvex    = static_cast<uint8>(_BackFaceModeConvex);
    auto TracePolicy       = static_cast<uint8>(_TracePolicy);
    InAr << BackFaceTriangles;
    InAr << BackFaceConvex;
    InAr << TracePolicy;
    if (InAr.IsLoading())
    {
        _BackFaceModeTriangles = static_cast<ECk_BackFaceMode>(BackFaceTriangles);
        _BackFaceModeConvex    = static_cast<ECk_BackFaceMode>(BackFaceConvex);
        _TracePolicy           = static_cast<ECk_ProbeTrace_Policy>(TracePolicy);
    }

    InCtx.Snapshot_Handle(InAr, _StartPos);
}

// --------------------------------------------------------------------------------------------------------------------
