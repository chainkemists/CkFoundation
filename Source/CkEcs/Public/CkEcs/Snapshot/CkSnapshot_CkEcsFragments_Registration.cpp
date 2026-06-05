#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Registration site for CkEcs-owned infra fragments. The handle-remap SerializeSnapshot bodies live here (out-of-line)
// because they deref the full ck::FSnapshotContext — which now lives in CkEcs alongside the archive. EntityScript's body
// lives in CkEntityScript_Fragment.cpp; Net_Params serializes inline in CkNet_Fragment.h (it is CKECS_API → an
// out-of-line body would be C4273). GameplayLabel registers in CkLabel.
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
// Registration.

using FSnap_EntityScript_Current = ck::FFragment_EntityScript_Current;
using FSnap_LifetimeOwner        = ck::FFragment_LifetimeOwner;
using FSnap_ContextOwner         = ck::FFragment_ContextOwner;
using FSnap_LifetimeDependents   = ck::FFragment_LifetimeDependents;
using FSnap_Net_Params           = ck::FFragment_Net_Params;

CK_REGISTER_SNAPSHOTABLE(FSnap_EntityScript_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_LifetimeOwner);
CK_REGISTER_SNAPSHOTABLE(FSnap_ContextOwner);
CK_REGISTER_SNAPSHOTABLE(FSnap_LifetimeDependents);
CK_REGISTER_SNAPSHOTABLE(FSnap_Net_Params);

// --------------------------------------------------------------------------------------------------------------------
