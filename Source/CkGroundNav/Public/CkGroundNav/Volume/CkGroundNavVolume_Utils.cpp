#include "CkGroundNavVolume_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Query/CkGroundNav_Query_BuildStatus.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_volume_utils
{
    // EMPTINESS, never presence. The request utils compose a queue fragment lazily and the drains
    // reset the array in place without removing the fragment, so a volume that was ever asked for
    // anything carries the fragment for the rest of its life - and a presence test would read every
    // such volume as permanently unsettled.
    template <typename T_RequestsFragment>
    auto Get_HasPendingRequests(
        const FCk_Handle_GroundNavVolume& InVolume) -> bool
    {
        return InVolume.Has<T_RequestsFragment>() &&
               NOT InVolume.Get<T_RequestsFragment>().Get_Requests().IsEmpty();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavVolume_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_GroundNavVolume_ParamsData& InParams)
    -> FCk_Handle_GroundNavVolume
{
    const auto OwnerIsValid = ck::IsValid(InOwner);

    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Invalid owner Handle [{}] supplied to UCk_Utils_GroundNavVolume_UE::Add"), InOwner)
    { return {}; }

    auto NewVolumeEntity =
        UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_GroundNavVolume>(InOwner);

    NewVolumeEntity.Add<ck::FFragment_GroundNavVolume_Params>(InParams);
    NewVolumeEntity.Add<ck::FFragment_GroundNavVolume_BuildState>();
    NewVolumeEntity.Add<ck::FFragment_GroundNavVolume_RepairState>();
    NewVolumeEntity.Add<ck::FFragment_GroundNavVolume_BuiltField>();
    NewVolumeEntity.Add<ck::FTag_GroundNavVolume_NeedsSetup>();

    ck::groundnav::Verbose(TEXT("GroundNav Volume added to [{}] -> [{}]"), InOwner, NewVolumeEntity);

    return NewVolumeEntity;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_GroundNavVolume_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavVolume_UE::
    Request_Build(
        FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Build& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavVolume_Requests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid GroundNav Volume Handle [{}] supplied to Request_Build"), InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_Requests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Request_Repair(
        FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Repair& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavVolume_RepairRequests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid GroundNav Volume Handle [{}] supplied to Request_Repair"), InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    const auto& DirtyBounds = InRequest.Get_DirtyBounds();

    const auto DirtyBoundsIsABox =
        DirtyBounds.IsValid != 0 &&
        NOT DirtyBounds.Min.ContainsNaN() &&
        NOT DirtyBounds.Max.ContainsNaN() &&
        DirtyBounds.Min.X < DirtyBounds.Max.X &&
        DirtyBounds.Min.Y < DirtyBounds.Max.Y &&
        DirtyBounds.Min.Z < DirtyBounds.Max.Z;

    CK_ENSURE_IF_NOT(DirtyBoundsIsABox,
        TEXT("Degenerate dirty bounds [{}] supplied to Request_Repair on GroundNav Volume [{}]"),
        DirtyBounds, InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_RepairRequests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Request_AreaMarkup(
        FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_AreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavVolume_MarkupRequests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid GroundNav Volume Handle [{}] supplied to Request_AreaMarkup"), InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_Markup>();
    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_MarkupRequests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Request_ReleaseAreaMarkup(
        FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_ReleaseAreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavVolume_MarkupRequests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid GroundNav Volume Handle [{}] supplied to Request_ReleaseAreaMarkup"), InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_Markup>();
    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_MarkupRequests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Request_Link(
        FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Link& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavVolume_LinkRequests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid GroundNav Volume Handle [{}] supplied to Request_Link"), InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_Links>();
    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_LinkRequests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Request_ReleaseLink(
        FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_ReleaseLink& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_GroundNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_GroundNavVolume_LinkRequests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid GroundNav Volume Handle [{}] supplied to Request_ReleaseLink"), InVolume)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_Links>();
    InVolume.AddOrGet<ck::FFragment_GroundNavVolume_LinkRequests>()._Requests.Emplace(InRequest);

    return InVolume;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_IsBuilt(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> bool
{
    return ck::IsValid(InVolume) && InVolume.Has<ck::FTag_GroundNavVolume_Built>();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_IsBuilding(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> bool
{
    if (ck::Is_NOT_Valid(InVolume))
    { return false; }

    // Armed counts as building. A caller polling this between the request landing and the first slice
    // would otherwise see a gap where the volume is neither building nor built, and conclude wrongly.
    return InVolume.Has<ck::FTag_GroundNavVolume_BuildInProgress>() ||
           InVolume.Has<ck::FTag_GroundNavVolume_NeedsBuild>();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_BuildEpoch(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> int64
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_BuiltField>())
    { return 0; }

    return InVolume.Get<ck::FFragment_GroundNavVolume_BuiltField>().Get_Epoch()._Value;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_TileCount(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> int32
{
    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return 0; }

    return Field->Get_TileCount();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_BuiltTileCount(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> int32
{
    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return 0; }

    return Field->Get_BuiltTileCount();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_SeamPortalCount(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> int32
{
    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return 0; }

    return Field->Get_SeamPortalCount();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_WalkableCellCount(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> int32
{
    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return 0; }

    auto Count = 0;

    for (const auto& Tile : Field->_Tiles)
    { Count += Tile.Get_WalkableCellCount(); }

    return Count;
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_UnresolvedLinkCount(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> int32
{
    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return 0; }

    return Field->Get_UnresolvedLinkCount();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_LinkRecords(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> TArray<FCk_GroundNav_LinkRecord>
{
    return ck::algo::Transform<TArray<FCk_GroundNav_LinkRecord>>(Get_LinkEntries(InVolume),
        [](const ck::FCk_GroundNav_LinkEntry& InEntry) -> FCk_GroundNav_LinkRecord
        {
            return InEntry.Get_Record();
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_RegionStatusAt(
        const FCk_Handle_GroundNavVolume& InVolume,
        const FVector& InLocation)
    -> ECk_GroundNav_RegionStatus
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_Params>())
    { return ECk_GroundNav_RegionStatus::OutsideField; }

    return ck::groundnav::Get_RegionStatusAt_ForVolume(
        Get_Field(InVolume),
        InVolume.Get<ck::FFragment_GroundNavVolume_Params>().Get_VolumeBounds(),
        Get_IsBuilding(InVolume),
        InLocation);
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_RegionStatusWithin(
        const FCk_Handle_GroundNavVolume& InVolume,
        const FBox& InBounds)
    -> ECk_GroundNav_RegionStatus
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_Params>())
    { return ECk_GroundNav_RegionStatus::OutsideField; }

    return ck::groundnav::Get_RegionStatusWithin_ForVolume(
        Get_Field(InVolume),
        InVolume.Get<ck::FFragment_GroundNavVolume_Params>().Get_VolumeBounds(),
        Get_IsBuilding(InVolume),
        InBounds);
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_SurfaceBounds(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> FBox
{
    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return FBox{ForceInit}; }

    return ck::groundnav::Get_SurfaceBounds(*Field);
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_ProviderHealth(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> ECk_NavSurface_ProviderHealth
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_BuildState>())
    { return ECk_NavSurface_ProviderHealth::NoData; }

    return ck::groundnav::Get_ProviderHealth(
        Get_Field(InVolume),
        InVolume.Get<ck::FFragment_GroundNavVolume_BuildState>().Get_Build()._Status,
        Get_IsBuilding(InVolume));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_Field(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> ck::groundnav::FCk_GroundNav_FieldPtr
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_BuiltField>())
    { return {}; }

    return InVolume.Get<ck::FFragment_GroundNavVolume_BuiltField>().Get_Field();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_MarkupRecords(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> TConstArrayView<ck::FCk_GroundNav_MarkupEntry>
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_Markup>())
    { return {}; }

    return InVolume.Get<ck::FFragment_GroundNavVolume_Markup>().Get_Entries();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_LinkEntries(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> TConstArrayView<ck::FCk_GroundNav_LinkEntry>
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_Links>())
    { return {}; }

    return InVolume.Get<ck::FFragment_GroundNavVolume_Links>().Get_Entries();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_PendingDirtyBounds(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> FBox
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_RepairState>())
    { return FBox{ForceInit}; }

    return InVolume.Get<ck::FFragment_GroundNavVolume_RepairState>().Get_PendingDirtyBounds();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_IsRepairInProgress(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> bool
{
    return ck::IsValid(InVolume) && InVolume.Has<ck::FTag_GroundNavVolume_RepairInProgress>();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_IsSettled(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> bool
{
    using namespace ck_groundnav_volume_utils;

    const auto Field = Get_Field(InVolume);

    if (ck::Is_NOT_Valid(Field))
    { return false; }

    // Get_IsBuilding is the NeedsBuild/BuildInProgress pair; the other three markers are the repair,
    // the cost derive and the link derive - the remaining stages that still owe this volume a publish.
    const auto AStageStillOwesAPublish =
        Get_IsBuilding(InVolume) ||
        Get_IsRepairInProgress(InVolume) ||
        InVolume.Has<ck::FTag_GroundNavVolume_NeedsRepair>() ||
        InVolume.Has<ck::FTag_GroundNavVolume_MarkupCostDirty>() ||
        InVolume.Has<ck::FTag_GroundNavVolume_LinksDirty>();

    if (AStageStillOwesAPublish)
    { return false; }

    // A queued request has not reached a stage yet, so no marker names it. Settling on the markers
    // alone would report the tick between an enqueue and its drain as settled.
    return NOT Get_HasPendingRequests<ck::FFragment_GroundNavVolume_Requests>(InVolume) &&
           NOT Get_HasPendingRequests<ck::FFragment_GroundNavVolume_RepairRequests>(InVolume) &&
           NOT Get_HasPendingRequests<ck::FFragment_GroundNavVolume_MarkupRequests>(InVolume) &&
           NOT Get_HasPendingRequests<ck::FFragment_GroundNavVolume_LinkRequests>(InVolume);
}

auto
    UCk_Utils_GroundNavVolume_UE::
    TryGet_MarkupRecord(
        const FCk_Handle_GroundNavVolume& InVolume,
        int32 InRecordId)
    -> TOptional<FCk_GroundNav_MarkupRecord>
{
    const auto Entries = Get_MarkupRecords(InVolume);

    const auto Index = ck::algo::FindIndex(Entries,
        [&](const ck::FCk_GroundNav_MarkupEntry& InEntry) -> bool
        {
            return InEntry.Get_Record().Get_Id() == InRecordId;
        });

    if (NOT Entries.IsValidIndex(Index))
    { return {}; }

    return Entries[Index].Get_Record();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    TryGet_LinkRecord(
        const FCk_Handle_GroundNavVolume& InVolume,
        int32 InRecordId)
    -> TOptional<FCk_GroundNav_LinkRecord>
{
    const auto Entries = Get_LinkEntries(InVolume);

    const auto Index = ck::algo::FindIndex(Entries,
        [&](const ck::FCk_GroundNav_LinkEntry& InEntry) -> bool
        {
            return InEntry.Get_Record().Get_Id() == InRecordId;
        });

    if (NOT Entries.IsValidIndex(Index))
    { return {}; }

    return Entries[Index].Get_Record();
}

auto
    UCk_Utils_GroundNavVolume_UE::
    Get_IsLinkLiveOnField(
        const ck::groundnav::FCk_GroundNav_Field& InField,
        const FCk_GroundNav_LinkRecord&           InRecord)
    -> bool
{
    const auto ResolvedIndex = ck::algo::FindIndex(InField._ResolvedLinks,
        [&](const ck::groundnav::FCk_GroundNav_ResolvedLink& InResolved) -> bool
        {
            return InResolved._Id == InRecord.Get_Id();
        });

    // A record the published field never resolved - because it was authored after that publish, or
    // released before it - has no entry to be live through.
    if (NOT InField._ResolvedLinks.IsValidIndex(ResolvedIndex))
    { return false; }

    // The clause with no markup analogue, and the whole point: a markup that reaches nothing is
    // admitted and simply decides nothing, where a link that did not resolve is not there at all.
    if (NOT InField._ResolvedLinks[ResolvedIndex].Get_IsResolved())
    { return false; }

    // Live means in effect, as for a markup: a disabled record the field has processed is a record
    // that decides nothing, and a fixture that switched one off waits on the volume being settled.
    if (InRecord.Get_Enable() == ECk_EnableDisable::Disable)
    { return false; }

    const auto* StartTile = InField.Get_TileAt(InRecord.Get_Start());
    const auto* EndTile = InField.Get_TileAt(InRecord.Get_End());

    if (StartTile == nullptr || EndTile == nullptr)
    { return false; }

    // BOTH ends, because a link is only as live as its laggard, and an agent handed one whose far end
    // has not republished would be routed onto ground that does not carry it yet.
    return StartTile->Get_IsBuilt() && EndTile->Get_IsBuilt() &&
           StartTile->_Epoch._Value > InRecord.Get_RequestedAtEpoch() &&
           EndTile->_Epoch._Value > InRecord.Get_RequestedAtEpoch();
}

bool
    UCk_Utils_GroundNavVolume_UE::
    Get_IsLinkLive(
        const FCk_Handle& InLinkEntity)
{
    if (ck::Is_NOT_Valid(InLinkEntity) || NOT InLinkEntity.Has<ck::FFragment_GroundNav_LinkRef>())
    { return false; }

    const auto& LinkRef = InLinkEntity.Get<ck::FFragment_GroundNav_LinkRef>();

    auto VolumeEntity = LinkRef.Get_VolumeEntity();

    auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

    if (ck::Is_NOT_Valid(Volume))
    { return false; }

    const auto Record = TryGet_LinkRecord(Volume, LinkRef.Get_RecordId());

    if (NOT Record.IsSet())
    { return false; }

    const auto Field = Get_Field(Volume);

    if (NOT Field.IsValid())
    { return false; }

    return Get_IsLinkLiveOnField(*Field, *Record);
}

// --------------------------------------------------------------------------------------------------------------------
