#include "CkEntityTag_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
// The CK_REGISTER_SNAPSHOTABLE save/load closures call entt's get<T>(Archive&), so the Archive writer/reader
// types must be COMPLETE here (not just forward-declared as in the registry header).
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EntityTag_Root, TEXT("EntityTag"));

// --------------------------------------------------------------------------------------------------------------------
// Snapshot registration. ck:: types hoisted to file-scope aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the
// type name (the `::` cannot be pasted). BOTH layers must round-trip together: FFragment_EntityTag_Current (Tier C,
// default storage) is the source of truth for Has/Get_AllTags; FCk_Fragment_EntityTag_ParamsData (Tier A) lives in
// the PER-TAG NAMED storages (keyed by GetTypeHash(_Tag)) that drive ForEach_Entity — the capture loop iterates the
// registry's storages and captures each named storage by its id. Registering only one would desync the two views.

using FSnap_EntityTag_Current = ck::FFragment_EntityTag_Current;
CK_REGISTER_SNAPSHOTABLE(FSnap_EntityTag_Current);
CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_EntityTag_ParamsData); // Tier A USTRUCT (no `::` → no alias hoist needed)

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_EntityTag_Current::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    // No handle fields → InCtx unused. Serialize both counted-tag arrays by hand (non-USTRUCT struct, so no
    // SerializeItem path); FName / FGameplayTag / int32 all have FArchive operator<<.
    auto NumTags = _Tags.Num();
    InAr << NumTags;
    if (InAr.IsLoading())
    { _Tags.SetNum(NumTags); }
    for (auto& Entry : _Tags)
    {
        InAr << Entry._Name;
        InAr << Entry._Count;
    }

    auto NumGameplayTags = _GameplayTagCounts.Num();
    InAr << NumGameplayTags;
    if (InAr.IsLoading())
    { _GameplayTagCounts.SetNum(NumGameplayTags); }
    for (auto& Entry : _GameplayTagCounts)
    {
        InAr << Entry._Tag;
        InAr << Entry._Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------
