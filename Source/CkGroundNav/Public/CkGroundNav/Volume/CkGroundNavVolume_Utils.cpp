#include "CkGroundNavVolume_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkGroundNav/CkGroundNav_Log.h"

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
    Get_Field(
        const FCk_Handle_GroundNavVolume& InVolume)
    -> ck::groundnav::FCk_GroundNav_FieldPtr
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_GroundNavVolume_BuiltField>())
    { return {}; }

    return InVolume.Get<ck::FFragment_GroundNavVolume_BuiltField>().Get_Field();
}

// --------------------------------------------------------------------------------------------------------------------
