#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Fragment.h"

#include "CkLabel/CkLabel_Fragment.h"

#include "CkSnapshot/Context/CkSnapshot_Context.h"
#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Registration site for CkEcs/CkLabel infra fragments. The SerializeSnapshot METHODS for handle-bearing fragments
// live HERE (out-of-line) — not in CkEcs — because their bodies deref the full ck::FSnapshotContext, which lives in
// CkSnapshot. CkEcs only forward-declares it, so CkEcs never takes a dependency edge on CkSnapshot (which would be a
// cycle, since CkSnapshot depends on CkEcs). Value-only fragments (Net_Params, GameplayLabel) serialize inline in
// their own headers; only their registration lives here.
//
// ck:: is hoisted to unqualified aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the type name.

// --------------------------------------------------------------------------------------------------------------------
// Out-of-line handle-remap bodies.

auto
    ck::FFragment_LifetimeOwner::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _Entity);
}

auto
    ck::FFragment_ContextOwner::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _Entity);
}

auto
    ck::FFragment_LifetimeDependents::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx)
    -> void
{
    auto Count = static_cast<int32>(_Entities.Num());
    InAr << Count;

    if (InAr.IsLoading())
    { _Entities.SetNum(Count); }

    for (auto& Dependent : _Entities)
    { InCtx.Snapshot_Handle(InAr, Dependent); }
}

// --------------------------------------------------------------------------------------------------------------------
// Out-of-line value body for GameplayLabel (no handle refs; InCtx unused). FFragment_GameplayLabel is NOT
// API-exported, so defining its body here is fine. (FFragment_Net_Params IS CKECS_API, so its body is inline
// in CkNet_Fragment.h to avoid inconsistent dll linkage.)

auto
    ck::FFragment_GameplayLabel::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    FGameplayTag::StaticStruct()->SerializeItem(InAr, &_Label, /*Defaults=*/nullptr);
}

// --------------------------------------------------------------------------------------------------------------------
// Registration.

using FSnap_EntityScript_Current   = ck::FFragment_EntityScript_Current;
using FSnap_LifetimeOwner          = ck::FFragment_LifetimeOwner;
using FSnap_ContextOwner           = ck::FFragment_ContextOwner;
using FSnap_LifetimeDependents     = ck::FFragment_LifetimeDependents;
using FSnap_Net_Params             = ck::FFragment_Net_Params;
using FSnap_GameplayLabel          = ck::FFragment_GameplayLabel;

using FSnap_Tag_HasAuthority       = ck::FTag_HasAuthority;
using FSnap_Tag_NetMode_IsClient   = ck::FTag_NetMode_IsClient;
using FSnap_Tag_NetMode_IsHost     = ck::FTag_NetMode_IsHost;

CK_REGISTER_SNAPSHOTABLE(FSnap_EntityScript_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_LifetimeOwner);
CK_REGISTER_SNAPSHOTABLE(FSnap_ContextOwner);
CK_REGISTER_SNAPSHOTABLE(FSnap_LifetimeDependents);
CK_REGISTER_SNAPSHOTABLE(FSnap_Net_Params);
CK_REGISTER_SNAPSHOTABLE(FSnap_GameplayLabel);

CK_REGISTER_SNAPSHOTABLE(FSnap_Tag_HasAuthority);
CK_REGISTER_SNAPSHOTABLE(FSnap_Tag_NetMode_IsClient);
CK_REGISTER_SNAPSHOTABLE(FSnap_Tag_NetMode_IsHost);

// --------------------------------------------------------------------------------------------------------------------
