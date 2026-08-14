#include "CkVoxelNavVolume_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkVoxelNav/Chunk/CkVoxelNav_Chunk_Search.h"
#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"
#include "CkVoxelNav/Settings/CkVoxelNav_ProjectSettings.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_volume_utils
{
    auto
        Get_DebugStateFingerprint(
            ECk_VoxelNav_BuildStage InBuildStage,
            ECk_VoxelNav_RepairStage InRepairStage,
            const FBox& InDirtyBounds)
        -> uint64
    {
        auto Hash = HashCombineFast(GetTypeHash(static_cast<uint8>(InBuildStage)),
            GetTypeHash(static_cast<uint8>(InRepairStage)));

        if (InDirtyBounds.IsValid != 0)
        {
            Hash = HashCombineFast(Hash, GetTypeHash(InDirtyBounds.Min));
            Hash = HashCombineFast(Hash, GetTypeHash(InDirtyBounds.Max));
        }

        return static_cast<uint64>(Hash);
    }

    auto
        Get_DebugStatus(
            bool InIsBuilt,
            ECk_VoxelNav_BuildStage InBuildStage,
            ECk_VoxelNav_RepairStage InRepairStage)
        -> ck::voxelnav::EDebugSnapshotStatus
    {
        using namespace ck::voxelnav;

        if (InBuildStage == ECk_VoxelNav_BuildStage::Failed ||
            InRepairStage == ECk_VoxelNav_RepairStage::Failed)
        { return EDebugSnapshotStatus::Failed; }

        const auto BuildIsWorking =
            InBuildStage != ECk_VoxelNav_BuildStage::NotStarted &&
            InBuildStage != ECk_VoxelNav_BuildStage::Complete;

        const auto RepairIsWorking =
            InRepairStage != ECk_VoxelNav_RepairStage::NotStarted &&
            InRepairStage != ECk_VoxelNav_RepairStage::Complete &&
            InRepairStage != ECk_VoxelNav_RepairStage::Failed;

        if (BuildIsWorking || RepairIsWorking || NOT InIsBuilt)
        { return EDebugSnapshotStatus::Building; }

        return EDebugSnapshotStatus::Current;
    }

    auto
        Get_DebugStatusMessage(
            const ck::voxelnav::FDebugSnapshot& InSnapshot)
        -> FString
    {
        using namespace ck::voxelnav;

        if (InSnapshot._Status == EDebugSnapshotStatus::Failed && InSnapshot._IsBuilt)
        { return TEXT("Latest operation failed; showing the last published navigation epoch."); }

        if (InSnapshot._Status == EDebugSnapshotStatus::Failed)
        { return TEXT("VoxelNav has no published navigation because its latest operation failed."); }

        if (InSnapshot._Status == EDebugSnapshotStatus::Building && InSnapshot._IsBuilt)
        { return TEXT("VoxelNav is rebuilding or repairing; showing the last published navigation epoch."); }

        if (InSnapshot._Status == EDebugSnapshotStatus::Building)
        { return TEXT("VoxelNav is building and has not published navigation yet."); }

        return TEXT("Live VoxelNav navigation is current.");
    }

    auto
        Make_PartitionParams(
            const FCk_Fragment_VoxelNavVolume_ParamsData& InParams)
        -> ck::voxelnav::FChunkPartitionParams
    {
        auto PartitionParams = ck::voxelnav::FChunkPartitionParams{};

        PartitionParams._VolumeBounds = InParams.Get_VolumeBounds();
        PartitionParams._MaxChunkSizeUu = InParams.Get_MaxChunkSizeOverride() == ECk_EnableDisable::Enable
            ? InParams.Get_MaxChunkSizeUuOverride()
            : UCk_Utils_VoxelNav_Settings_UE::Get_MaxChunkSizeUu();
        PartitionParams._MaxChunksPerAxis = UCk_Utils_VoxelNav_Settings_UE::Get_MaxChunksPerAxis();

        return PartitionParams;
    }

    auto
        Make_ChunkParams(
            const FCk_Fragment_VoxelNavVolume_ParamsData& InParams,
            const FBox& InChunkBounds)
        -> FCk_Fragment_VoxelNavVolume_ParamsData
    {
        auto ChunkParams = FCk_Fragment_VoxelNavVolume_ParamsData{InChunkBounds, InParams.Get_FinestCellSizeUu()};

        ChunkParams.Set_ClearanceUu(InParams.Get_ClearanceUu());
        ChunkParams.Set_AutoBuildOnSetup(InParams.Get_AutoBuildOnSetup());
        ChunkParams.Set_BuildBudgetOverride(InParams.Get_BuildBudgetOverride());
        ChunkParams.Set_MaxOccupancyProbesPerTickOverride(InParams.Get_MaxOccupancyProbesPerTickOverride());

        // A chunk is a leaf of the partition by construction, not by arithmetic: leaving partitioning armed
        // would make a chunk's leaf-ness depend on the max chunk size dividing the volume evenly.
        ChunkParams.Set_ChunkPartitioning(ECk_EnableDisable::Disable);

        return ChunkParams;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_VoxelNavVolume_ParamsData& InParams)
    -> FCk_Handle_VoxelNavVolume
{
    using namespace ck_voxelnav_volume_utils;

    const auto OwnerIsValid = ck::IsValid(InOwner);

    CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("Invalid owner Handle [{}] supplied to UCk_Utils_VoxelNavVolume_UE::Add"), InOwner)
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

    if (InParams.Get_ChunkPartitioning() == ECk_EnableDisable::Disable)
    { return NewVolumeEntity; }

    const auto Partition = ck::voxelnav::Get_ChunkPartition(Make_PartitionParams(InParams));

    // A volume that fits inside one chunk stays exactly what it was - one entity, one bake, one octree.
    // Wrapping it in a one-chunk hierarchy would change every query's route for no gain at all.
    if (NOT Partition.Get_IsPartitioned())
    { return NewVolumeEntity; }

    auto& ChunkRecord = NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_Chunks>();
    NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_ChunkAdjacency>();
    NewVolumeEntity.Add<ck::FTag_VoxelNavVolume_Partitioned>();

    ChunkRecord._Divisions = Partition.Get_Divisions();
    ChunkRecord._Chunks.Reserve(Partition.Get_ChunkCount());

    const auto ParentVolumeId = BuiltOctree.Get_VolumeId();

    for (auto ChunkIdx = 0; ChunkIdx < Partition.Get_ChunkCount(); ++ChunkIdx)
    {
        auto ChunkEntity = Add(NewVolumeEntity, Make_ChunkParams(InParams, Partition.Get_ChunkBounds(ChunkIdx)));

        ChunkEntity.Add<ck::FFragment_VoxelNavVolume_ChunkIdentity>()._ChunkId =
            ck::voxelnav::FChunkId{ParentVolumeId, ChunkIdx};

        ChunkRecord._Chunks.Emplace(ChunkEntity);
    }

    ck::voxelnav::Verbose(TEXT("VoxelNav Volume [{}] partitioned into [{}] chunks ([{}] divisions)"),
        NewVolumeEntity, ChunkRecord._Chunks.Num(), Partition.Get_Divisions());

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

    // A partitioned volume is built when every one of its chunks is: it holds no octree of its own, and
    // answering "built" while one chunk is still rasterizing would hand a caller a volume with a hole in it.
    if (Get_IsPartitioned(InVolume))
    {
        for (const auto& Chunk : InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks())
        {
            if (NOT Get_IsBuilt(Chunk))
            { return false; }
        }

        return true;
    }

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

    if (Get_IsPartitioned(InVolume))
    {
        const auto& Chunks = InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks();

        if (Chunks.IsEmpty())
        { return 0.0f; }

        auto Total = 0.0f;

        for (const auto& Chunk : Chunks)
        { Total += Get_BuildProgress(Chunk); }

        return Total / Chunks.Num();
    }

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

    // A partitioned volume's bake cost is the sum of its chunks'. Summed rather than left per-chunk because
    // the probe counts are what a test asserts against, and a partitioned volume's total is as
    // deterministic for a given scene as an unpartitioned one's.
    if (Get_IsPartitioned(InVolume))
    {
        auto Stats = FCk_VoxelNav_BuildStats{};

        for (const auto& Chunk : InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks())
        {
            const auto ChunkStats = Get_BuildStats(Chunk);

            Stats.Set_OccupancyProbes(Stats.Get_OccupancyProbes() + ChunkStats.Get_OccupancyProbes());
            Stats.Set_Slices(Stats.Get_Slices() + ChunkStats.Get_Slices());
            Stats.Set_BroadphaseBodyCount(Stats.Get_BroadphaseBodyCount() + ChunkStats.Get_BroadphaseBodyCount());
            Stats.Set_BlockedLayer1Cells(Stats.Get_BlockedLayer1Cells() + ChunkStats.Get_BlockedLayer1Cells());
            Stats.Set_LeafNodeCount(Stats.Get_LeafNodeCount() + ChunkStats.Get_LeafNodeCount());
            Stats.Set_OccludedLeafCount(Stats.Get_OccludedLeafCount() + ChunkStats.Get_OccludedLeafCount());
            Stats.Set_TotalNodeCount(Stats.Get_TotalNodeCount() + ChunkStats.Get_TotalNodeCount());
            Stats.Set_BuildSeconds(Stats.Get_BuildSeconds() + ChunkStats.Get_BuildSeconds());
        }

        return Stats;
    }

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

    // The DEEPEST chunk, not a sum and not an average: layer count is a resolution, and a volume's
    // resolution is the finest its chunks reached.
    if (Get_IsPartitioned(InVolume))
    {
        auto MaxLayers = 0;

        for (const auto& Chunk : InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks())
        { MaxLayers = FMath::Max(MaxLayers, Get_NumLayers(Chunk)); }

        return MaxLayers;
    }

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

    // ANY chunk vouching for the point is enough. Chunks abut exactly, but each one's octree addresses a
    // power-of-two cube larger than its own box, so a point near a shared face is inside more than one
    // chunk's bake and the chunk that OWNS it is the one whose answer counts.
    if (Get_IsPartitioned(InVolume))
    {
        const auto Chunks = Get_ChunkSearchInputs(InVolume);
        const auto ChunkIndex = ck::voxelnav::TryGet_ChunkIndexContainingPoint(Chunks, InLocation);

        if (ChunkIndex == ck::voxelnav::FChunkId::InvalidIndex)
        { return false; }

        return ck::voxelnav::Get_IsPositionFree(*Chunks[ChunkIndex]._Octree, InLocation);
    }

    const auto& Octree = InVolume.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>().Get_Octree();

    return ck::voxelnav::Get_IsPositionFree(*Octree, InLocation);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_IsPartitioned(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> bool
{
    if (ck::Is_NOT_Valid(InVolume))
    { return false; }

    return InVolume.Has<ck::FFragment_VoxelNavVolume_Chunks>();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_ChunkCount(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> int32
{
    if (NOT Get_IsPartitioned(InVolume))
    { return 0; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks().Num();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_Chunk(
        const FCk_Handle_VoxelNavVolume& InVolume,
        int32 InChunkIndex)
    -> FCk_Handle_VoxelNavVolume
{
    if (NOT Get_IsPartitioned(InVolume))
    { return {}; }

    const auto& Chunks = InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks();

    const auto IndexIsInRange = Chunks.IsValidIndex(InChunkIndex);

    CK_ENSURE_IF_NOT(IndexIsInRange,
        TEXT("VoxelNav Volume [{}] has [{}] chunks, so chunk index [{}] names none of them"),
        InVolume, Chunks.Num(), InChunkIndex)
    { return {}; }

    return Chunks[InChunkIndex];
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_ChunkIndex(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> int32
{
    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_VoxelNavVolume_ChunkIdentity>())
    { return ck::voxelnav::FChunkId::InvalidIndex; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_ChunkIdentity>().Get_ChunkId().Get_Index();
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_ChunkPortalCount(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> int32
{
    return Get_ChunkAdjacency(InVolume).Get_PortalCount();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    TryBuild_DebugSnapshot(
        const FCk_Handle_VoxelNavVolume& InVolume,
        const ck::voxelnav::FDebugSnapshotBuildParams& InParams,
        ck::voxelnav::FDebugSnapshot& OutSnapshot)
    -> bool
{
    using namespace ck_voxelnav_volume_utils;
    using namespace ck::voxelnav;

    auto Snapshot = FDebugSnapshot{};

    if (ck::Is_NOT_Valid(InVolume))
    {
        OutSnapshot = MoveTemp(Snapshot);
        return false;
    }

    const auto VolumeIsUsable =
        InVolume.Has<ck::FFragment_VoxelNavVolume_Params>() &&
        InVolume.Has<ck::FFragment_VoxelNavVolume_BuildState>() &&
        InVolume.Has<ck::FFragment_VoxelNavVolume_RepairState>() &&
        InVolume.Has<ck::FFragment_VoxelNavVolume_BuiltOctree>();

    CK_ENSURE_IF_NOT(VolumeIsUsable,
        TEXT("Cannot build a VoxelNav debug snapshot from incomplete Volume [{}]"), InVolume)
    {
        OutSnapshot = MoveTemp(Snapshot);
        return false;
    }

    const auto& BuiltOctree = InVolume.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>();
    const auto& RepairState = InVolume.Get<ck::FFragment_VoxelNavVolume_RepairState>();

    Snapshot._Source = EDebugSnapshotSource::LivePie;
    Snapshot._SourceIdentity = FString::Printf(TEXT("VoxelNavVolume:%u"), BuiltOctree.Get_VolumeId().Get_Value());
    Snapshot._SourceEpoch = BuiltOctree.Get_Epoch();
    Snapshot._AuthoredBounds = InVolume.Get<ck::FFragment_VoxelNavVolume_Params>().Get_VolumeBounds();
    Snapshot._BuildStage = Get_BuildStage(InVolume);
    Snapshot._RepairStage = Get_RepairStage(InVolume);
    Snapshot._BuildStats = Get_BuildStats(InVolume);
    Snapshot._RepairStats = Get_RepairStats(InVolume);
    Snapshot._BuildProgress = Get_BuildProgress(InVolume);
    Snapshot._IsBuilt = Get_IsBuilt(InVolume);
    Snapshot._IsPartitioned = Get_IsPartitioned(InVolume);
    Snapshot._Status = Get_DebugStatus(Snapshot._IsBuilt, Snapshot._BuildStage, Snapshot._RepairStage);

    const auto DirtyBounds = RepairState.Get_PendingDirtyBounds();
    Snapshot._SourceFingerprint = Get_DebugStateFingerprint(
        Snapshot._BuildStage, Snapshot._RepairStage, DirtyBounds);

    const auto RepairIsWorking =
        Snapshot._RepairStage != ECk_VoxelNav_RepairStage::NotStarted &&
        Snapshot._RepairStage != ECk_VoxelNav_RepairStage::Complete &&
        Snapshot._RepairStage != ECk_VoxelNav_RepairStage::Failed;

    if (RepairIsWorking)
    { Snapshot._ActiveDirtyBounds = DirtyBounds; }
    else
    { Snapshot._PendingDirtyBounds = DirtyBounds; }

    Snapshot._StatusMessage = Get_DebugStatusMessage(Snapshot);

    if (Snapshot._IsPartitioned)
    {
        const auto Inputs = Get_ChunkSearchInputs(InVolume);
        const auto& ChunkHandles = InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks();

        Snapshot._Chunks.Reserve(Inputs.Num());

        for (auto ChunkIndex = 0; ChunkIndex < Inputs.Num(); ++ChunkIndex)
        {
            const auto& Input = Inputs[ChunkIndex];
            const auto ChunkIsValid = ChunkHandles.IsValidIndex(ChunkIndex) && ck::IsValid(ChunkHandles[ChunkIndex]);

            auto ChunkSnapshot = FDebugSnapshotChunk{};
            ChunkSnapshot._Bounds = Input._Bounds;
            ChunkSnapshot._ChunkIndex = ChunkIndex;

            if (ChunkIsValid)
            {
                ChunkSnapshot._Epoch = Get_BuildEpoch(ChunkHandles[ChunkIndex]);
                ChunkSnapshot._BuildStage = Get_BuildStage(ChunkHandles[ChunkIndex]);
                ChunkSnapshot._IsBuilt = Get_IsBuilt(ChunkHandles[ChunkIndex]);
            }

            Snapshot._Chunks.Emplace(MoveTemp(ChunkSnapshot));

            if (Input._Octree.IsValid())
            { Append_OctreeDebugSnapshot(*Input._Octree, ChunkIndex, InParams, Snapshot); }
        }

        const auto& Adjacency = Get_ChunkAdjacency(InVolume);

        for (auto FromChunkIndex = 0; FromChunkIndex < Inputs.Num(); ++FromChunkIndex)
        {
            for (const auto ToChunkIndex : Adjacency.Get_NeighborIndices(FromChunkIndex))
            {
                if (ToChunkIndex <= FromChunkIndex || NOT Inputs.IsValidIndex(ToChunkIndex))
                { continue; }

                for (const auto& Portal : Adjacency.Get_Portals(FromChunkIndex, ToChunkIndex))
                {
                    if (NOT Inputs[FromChunkIndex]._Octree.IsValid() || NOT Inputs[ToChunkIndex]._Octree.IsValid())
                    { continue; }

                    const auto FromBounds = Get_NodeBoundsFromAddress(
                        *Inputs[FromChunkIndex]._Octree, Portal.Get_FromCellAddress());
                    const auto ToBounds = Get_NodeBoundsFromAddress(
                        *Inputs[ToChunkIndex]._Octree, Portal.Get_ToCellAddress());

                    if (FromBounds.IsValid == 0 || ToBounds.IsValid == 0)
                    { continue; }

                    Snapshot._Portals.Emplace(FDebugSnapshotPortal{
                        FromBounds.GetCenter(), ToBounds.GetCenter(), Portal._ConnectionPoint,
                        FromChunkIndex, ToChunkIndex});
                }
            }
        }
    }
    else if (BuiltOctree.Get_Octree().IsValid())
    {
        Append_OctreeDebugSnapshot(*BuiltOctree.Get_Octree(), INDEX_NONE, InParams, Snapshot);
    }

    OutSnapshot = MoveTemp(Snapshot);
    return true;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    TryBuild_DebugSnapshotsForWorld(
        const UObject* InWorldContextObject,
        const ck::voxelnav::FDebugSnapshotBuildParams& InParams,
        TArray<ck::voxelnav::FDebugSnapshot>& OutSnapshots)
    -> bool
{
    OutSnapshots.Reset();

    auto* World = InWorldContextObject != nullptr ? InWorldContextObject->GetWorld() : nullptr;
    auto* Subsystem = World != nullptr ? World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>() : nullptr;
    if (Subsystem == nullptr)
    { return false; }

    const auto TransientEntity = Subsystem->Get_TransientEntity();
    const auto TransientEntityIsValid = ck::IsValid(TransientEntity);
    if (NOT TransientEntityIsValid)
    { return false; }

    auto Registry = Subsystem->Get_Registry();
    Registry.View<ck::FFragment_VoxelNavVolume_Params>().ForEach(
        [&](FCk_Entity InEntity, const ck::FFragment_VoxelNavVolume_Params&)
        {
            auto Volume = ck::StaticCast<FCk_Handle_VoxelNavVolume>(ck::MakeHandle(InEntity, TransientEntity));
            auto Snapshot = ck::voxelnav::FDebugSnapshot{};
            if (TryBuild_DebugSnapshot(Volume, InParams, Snapshot))
            { OutSnapshots.Emplace(MoveTemp(Snapshot)); }
        });

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_ChunkSearchInputs(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> TArray<ck::voxelnav::FChunkSearchInput>
{
    auto Inputs = TArray<ck::voxelnav::FChunkSearchInput>{};

    if (NOT Get_IsPartitioned(InVolume))
    { return Inputs; }

    const auto& Chunks = InVolume.Get<ck::FFragment_VoxelNavVolume_Chunks>().Get_Chunks();

    Inputs.Reserve(Chunks.Num());

    for (const auto& Chunk : Chunks)
    {
        auto Input = ck::voxelnav::FChunkSearchInput{};

        if (ck::IsValid(Chunk))
        {
            const auto& ChunkOctree = Chunk.Get<ck::FFragment_VoxelNavVolume_BuiltOctree>();

            Input._ChunkId = Chunk.Get<ck::FFragment_VoxelNavVolume_ChunkIdentity>().Get_ChunkId();
            Input._CellVolumeId = ChunkOctree.Get_VolumeId();
            Input._Bounds = Chunk.Get<ck::FFragment_VoxelNavVolume_Params>().Get_VolumeBounds();
            Input._Octree = ChunkOctree.Get_Octree();
        }

        // A destroyed or unbuilt chunk still occupies its lattice position: dropping it would shift every
        // later chunk's index, and the adjacency table's integer keys index straight into this array.
        Inputs.Emplace(MoveTemp(Input));
    }

    return Inputs;
}

auto
    UCk_Utils_VoxelNavVolume_UE::
    Get_ChunkAdjacency(
        const FCk_Handle_VoxelNavVolume& InVolume)
    -> const ck::voxelnav::FChunkAdjacencyTable&
{
    static const auto EmptyTable = ck::voxelnav::FChunkAdjacencyTable{};

    if (ck::Is_NOT_Valid(InVolume) || NOT InVolume.Has<ck::FFragment_VoxelNavVolume_ChunkAdjacency>())
    { return EmptyTable; }

    return InVolume.Get<ck::FFragment_VoxelNavVolume_ChunkAdjacency>().Get_Table();
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
