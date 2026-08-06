#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend_Jolt.h"
#include "CkVoxelNav/Chunk/CkVoxelNav_Chunk_Types.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Build.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Repair.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoxelNavVolume_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_NeedsBuild);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_BuildInProgress);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_Built);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_NeedsRepair);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_RepairInProgress);

    // A volume too large for one chunk, stamped at composition. It never bakes an octree of its own - it
    // owns the chunk children that do, and every query on it routes to one of them.
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_Partitioned);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoxelNavVolume_Params = FCk_VoxelNavVolume_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    /** The published bake. `_Epoch` bumps on every completed (re)build, so a path planned against an older
     *  one can tell it is stale and replan.
     *
     *  The octree is held as TSharedPtr<const FOctree> and swapped ATOMICALLY at the end of a build: a
     *  rebuild assembles its own octree in the build state and never touches this one, so a search holding
     *  a copy of the pointer keeps walking a whole, valid structure for as long as it needs it. That is
     *  what makes post-bake reads lock-free and safe at horde scale. */
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_BuiltOctree
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_BuiltOctree);

        friend class FProcessor_VoxelNavVolume_Build;
        friend class FProcessor_VoxelNavVolume_Repair;
        friend class FProcessor_VoxelNavVolume_AggregateChunks;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        // Null until the first build completes. Stays null on a PARTITIONED volume, which owns chunk
        // children that each hold one of these instead - only `_Epoch` is meaningful there.
        TSharedPtr<const voxelnav::FOctree> _Octree;
        voxelnav::FVolumeId _VolumeId;
        int32 _Epoch = 0;

    public:
        CK_PROPERTY_GET(_Octree);
        CK_PROPERTY_GET(_VolumeId);
        CK_PROPERTY_GET(_Epoch);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** Everything a build in flight needs, and nothing the finished octree carries.
     *
     *  `_Backend` is the CONCRETE Jolt backend rather than the interface, because only the concrete type
     *  answers Get_IsValid — an invalid session reports every probe as unoccupied, so a build that skipped
     *  that gate would bake an empty world and call it free space. It is created when a build starts and
     *  dropped the moment one ends, so the pinned Jolt session never outlives the bake.
     *
     *  `_PendingRequest` carries the caller's completion delegate across the whole multi-frame build: the
     *  drain that accepted it cannot report the outcome, because the outcome is frames away. */
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_BuildState
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_BuildState);

        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class FProcessor_VoxelNavVolume_StartBuild;
        friend class FProcessor_VoxelNavVolume_Build;
        friend class FProcessor_VoxelNavVolume_AggregateChunks;
        friend class FProcessor_VoxelNavVolume_CancelPendingRequests;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        voxelnav::FBuildState _Build;
        TUniquePtr<FCk_VoxelNav_GeometryBackend_Jolt> _Backend;
        FCk_Request_VoxelNavVolume_Build _PendingRequest;

    public:
        CK_PROPERTY_GET(_Build);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** A local repair in flight, plus the dirty region waiting for one.
     *
     *  `_PendingDirtyBounds` ACCUMULATES: several occluders may dirty the same volume in one frame, and one
     *  repair over their union costs less than one repair each. It is consumed - and reset - when a repair
     *  starts, so a dirty box arriving mid-repair opens the next one rather than corrupting this one.
     *
     *  `_PendingRequests` carries the callers' completion delegates across the whole multi-frame repair, for
     *  the same reason the build state carries one: the drain that accepted them cannot report an outcome
     *  that is frames away. A list rather than a single request because dirty requests coalesce. */
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_RepairState
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_RepairState);

        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class FProcessor_VoxelNavVolume_Build;
        friend class FProcessor_VoxelNavVolume_StartRepair;
        friend class FProcessor_VoxelNavVolume_Repair;
        friend class FProcessor_VoxelNavVolume_CancelPendingRequests;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        voxelnav::FRepairState _Repair;
        TUniquePtr<FCk_VoxelNav_GeometryBackend_Jolt> _Backend;
        FBox _PendingDirtyBounds = FBox{ForceInit};
        TArray<FCk_Request_VoxelNavVolume_MarkDirty> _PendingRequests;

    public:
        CK_PROPERTY_GET(_Repair);
        CK_PROPERTY_GET(_PendingDirtyBounds);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** The parent side of a partition: the chunk children it owns, and what it has already aggregated from
     *  them.
     *
     *  The handles here are a RUNTIME CHILD RECORD, not navigation data. Nothing serialized and nothing in
     *  the adjacency table names an entity - a chunk's identity there is FChunkId, which is a pure function
     *  of the authored params. This array is how the volume reaches its own children in one step instead of
     *  scanning the world for them, which is precisely what upstream's TActorIterator hot-path lookups did.
     *
     *  Chunks are stored in PARTITION LATTICE ORDER, so a chunk's array position IS its FChunkId index and
     *  the adjacency table's integer keys index straight into this array. */
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_Chunks
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_Chunks);

        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class FProcessor_VoxelNavVolume_AggregateChunks;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        TArray<FCk_Handle_VoxelNavVolume> _Chunks;
        FIntVector _Divisions = FIntVector{1, 1, 1};

        // The combined chunk epoch this volume last aggregated. Epochs only increase, so a difference here
        // means at least one chunk rebaked or repaired since - which is the whole trigger.
        int64 _AggregatedChunkEpochSum = -1;

        // A whole-volume bake was asked for and has been fanned out to the chunks. It is what tells the
        // aggregation whether the change it is about to publish is a BAKE completing or an obstacle having
        // moved - the two have separate signals, and a listener waiting for the first bake must not be woken
        // by the second.
        bool _FanOutBuildPending = true;

    public:
        CK_PROPERTY_GET(_Chunks);
        CK_PROPERTY_GET(_Divisions);
        CK_PROPERTY_GET(_AggregatedChunkEpochSum);
        CK_PROPERTY_GET(_FanOutBuildPending);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** The baked crossings between a partitioned volume's chunks. Its own fragment rather than a member of
     *  the chunk record, so the type that a search and a serializer touch stays free of anything that names
     *  an entity. */
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_ChunkAdjacency
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_ChunkAdjacency);

        friend class FProcessor_VoxelNavVolume_AggregateChunks;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        voxelnav::FChunkAdjacencyTable _Table;

    public:
        CK_PROPERTY_GET(_Table);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** The child side of a partition. A chunk entity is an ORDINARY volume entity in every other respect -
     *  it bakes, repairs and publishes through the same processors - and this fragment is the only thing
     *  that says whose chunk it is. */
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_ChunkIdentity
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_ChunkIdentity);

        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        voxelnav::FChunkId _ChunkId;

    public:
        CK_PROPERTY_GET(_ChunkId);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOXELNAV_API FFragment_VoxelNavVolume_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_Requests);

        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    public:
        using RequestType = std::variant<FCk_Request_VoxelNavVolume_Build,
                                         FCk_Request_VoxelNavVolume_CancelBuild,
                                         FCk_Request_VoxelNavVolume_MarkDirty>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Fires on BOTH outcomes, which is why it carries a result: a listener that only ever heard about
    // successes would wait forever on a volume whose bake failed.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVOXELNAV_API,
        OnVoxelNavVolumeBuildComplete,
        FCk_Delegate_VoxelNavVolume_OnBuildComplete,
        FCk_Handle_VoxelNavVolume,
        ECk_SucceededFailed,
        FCk_VoxelNav_BuildStats);

    // Distinct from the build signal on purpose: a listener that wants to know when navigation data CHANGED
    // wants both, but a listener waiting for the FIRST bake would be woken by every obstacle that moves.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVOXELNAV_API,
        OnVoxelNavVolumeRepairComplete,
        FCk_Delegate_VoxelNavVolume_OnRepairComplete,
        FCk_Handle_VoxelNavVolume,
        ECk_SucceededFailed,
        FCk_VoxelNav_RepairStats);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VoxelNavVolume_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
