#include "CkVoxelNavVolume_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_VoxelNavVolume_ParamsData& InParams)
    -> FCk_Handle_VoxelNavVolume
{
    const auto OwnerIsValid = ck::IsValid(InOwner);

    CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("Invalid owner Handle [{}] supplied to UCk_Utils_VoxelNavVolume_UE::Add"), InOwner)
    {}

    if (NOT OwnerIsValid)
    { return {}; }

    auto NewVolumeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_VoxelNavVolume>(InOwner);

    NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_Params>(InParams);
    NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_BuildState>();
    NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_RepairState>();

    auto& BuiltOctree = NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_BuiltOctree>();

    // The entity id IS the volume's stable identity: a bake outlives whatever authored it, and cross-volume
    // paths need an id that never dereferences an actor. Id 0 is the registry's transient root, so it is
    // never a volume and stays free as the never-a-volume sentinel.
    BuiltOctree._VolumeId = ck::voxelnav::FVolumeId
    {
        static_cast<uint32>(NewVolumeEntity.Get_Entity().Get_ID())
    };

    NewVolumeEntity.Add<ck::FTag_VoxelNavVolume_NeedsSetup>();

    ck::voxelnav::Verbose(TEXT("VoxelNav Volume added to [{}] -> [{}]"), InOwner, NewVolumeEntity);

    return NewVolumeEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_VoxelNavVolume_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Request_Build(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_Build& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoxelNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_VoxelNavVolume_Requests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid VoxelNav Volume Handle [{}] supplied to Request_Build"), InVolume)
    {}

    if (NOT VolumeIsValid)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_VoxelNavVolume_Requests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Request_CancelBuild(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_CancelBuild& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoxelNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_VoxelNavVolume_Requests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid VoxelNav Volume Handle [{}] supplied to Request_CancelBuild"), InVolume)
    {}

    if (NOT VolumeIsValid)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_VoxelNavVolume_Requests>()._Requests.Emplace(InRequest);

    return InVolume;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Request_MarkDirty(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Request_VoxelNavVolume_MarkDirty& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoxelNavVolume
{
    CK_CALLSTACK_RECORD(ck::FFragment_VoxelNavVolume_Requests, InVolume);

    const auto VolumeIsValid = ck::IsValid(InVolume);

    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid VoxelNav Volume Handle [{}] supplied to Request_MarkDirty"), InVolume)
    {}

    if (NOT VolumeIsValid)
    {
        InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InVolume;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InVolume.AddOrGet<ck::FFragment_VoxelNavVolume_Requests>()._Requests.Emplace(InRequest);

    return InVolume;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_IsBuilt(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> bool
{
    if (ck::Is_NOT_Valid(InVolume))
    { return false; }

    const auto& Octree = InVolume.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>().Get_Octree();

    return Octree.IsValid() && Octree->Get_IsValid();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_BuildEpoch(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> int32
{
    if (ck::Is_NOT_Valid(InVolume))
    { return 0; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>().Get_Epoch();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_BuildStage(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> ECk_VoxelNav_BuildStage
{
    if (ck::Is_NOT_Valid(InVolume))
    { return ECk_VoxelNav_BuildStage::NotStarted; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_BuildState>().Get_Build().Get_Stage();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_BuildProgress(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> float
{
    if (ck::Is_NOT_Valid(InVolume))
    { return 0.0f; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_BuildState>().Get_Build().Get_Progress01();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_BuildStats(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> FCk_VoxelNav_BuildStats
{
    if (ck::Is_NOT_Valid(InVolume))
    { return {}; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_BuildState>().Get_Build().Get_Stats();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_NumLayers(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> int32
{
    if (NOT Get_IsBuilt(InVolume))
    { return 0; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>().Get_Octree()->Get_LayerCount();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_RepairStage(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> ECk_VoxelNav_RepairStage
{
    if (ck::Is_NOT_Valid(InVolume))
    { return ECk_VoxelNav_RepairStage::NotStarted; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_RepairState>().Get_Repair().Get_Stage();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_RepairStats(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> FCk_VoxelNav_RepairStats
{
    if (ck::Is_NOT_Valid(InVolume))
    { return {}; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_RepairState>().Get_Repair().Get_Stats();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_PendingDirtyBounds(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> FBox
{
    if (ck::Is_NOT_Valid(InVolume))
    { return FBox{ForceInit}; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_RepairState>().Get_PendingDirtyBounds();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_IsPointFree(
        const FCk_Handle_VoxelNavVolume& InVolume,
        FVector InLocation)
    -> bool
{
    if (NOT Get_IsBuilt(InVolume))
    { return false; }

    const auto& Octree = InVolume.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>().Get_Octree();

    return ck::voxelnav::Get_IsPositionFree(*Octree, InLocation);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    BindTo_OnBuildComplete(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnBuildComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VoxelNavVolume
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoxelNavVolumeBuildComplete, InVolume, InDelegate,
        InBindingPolicy, InPostFireBehavior);

    return InVolume;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    UnbindFrom_OnBuildComplete(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnBuildComplete& InDelegate)
    -> FCk_Handle_VoxelNavVolume
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoxelNavVolumeBuildComplete, InVolume, InDelegate);

    return InVolume;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    BindTo_OnRepairComplete(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnRepairComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VoxelNavVolume
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoxelNavVolumeRepairComplete, InVolume, InDelegate,
        InBindingPolicy, InPostFireBehavior);

    return InVolume;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    UnbindFrom_OnRepairComplete(
        FCk_Handle_VoxelNavVolume& InVolume,
        const FCk_Delegate_VoxelNavVolume_OnRepairComplete& InDelegate)
    -> FCk_Handle_VoxelNavVolume
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoxelNavVolumeRepairComplete, InVolume, InDelegate);

    return InVolume;
}

// --------------------------------------------------------------------------------------------------------------------
