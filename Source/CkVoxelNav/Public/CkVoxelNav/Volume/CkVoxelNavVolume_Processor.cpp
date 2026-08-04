#include "CkVoxelNavVolume_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Settings/CkVoxelNav_ProjectSettings.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_StartBuild);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_Build);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_StartRepair);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_Repair);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_volume_processor
{
    // Jolt's penetration slop is 2uu, so a finest cell approaching it is below the physics' own
    // resolution and its occupancy answers stop meaning anything.
    constexpr auto MinFinestCellSizeUu = 4.0f;

    auto
        Get_ParamsAreBakeable(
            const ck::FFragment_VoxelNavVolume_Params& InParams)
        -> bool
    {
        return InParams.Get_VolumeBounds().IsValid != 0 &&
               InParams.Get_VolumeBounds().GetSize().GetAbsMax() > 0.0 &&
               InParams.Get_FinestCellSizeUu() >= MinFinestCellSizeUu &&
               InParams.Get_ClearanceUu() >= 0.0f;
    }

    auto
        Make_BuildParams(
            const ck::FFragment_VoxelNavVolume_Params& InParams)
        -> ck::voxelnav::FBuildParams
    {
        auto BuildParams = ck::voxelnav::FBuildParams{};

        BuildParams._VolumeBounds = InParams.Get_VolumeBounds();
        BuildParams._FinestCellSizeUu = InParams.Get_FinestCellSizeUu();
        BuildParams._ClearanceUu = InParams.Get_ClearanceUu();

        return BuildParams;
    }

    auto
        Make_BuildBudget(
            const ck::FFragment_VoxelNavVolume_Params& InParams)
        -> ck::voxelnav::FBuildBudget
    {
        constexpr auto MillisecondsToSeconds = 0.001f;

        auto Budget = ck::voxelnav::FBuildBudget{};

        Budget._MaxOccupancyProbes = InParams.Get_BuildBudgetOverride() == ECk_EnableDisable::Enable
            ? InParams.Get_MaxOccupancyProbesPerTickOverride()
            : UCk_Utils_VoxelNav_Settings_UE::Get_MaxOccupancyProbesPerTick();

        Budget._MaxSeconds =
            UCk_Utils_VoxelNav_Settings_UE::Get_MaxBuildMillisecondsPerTick() * MillisecondsToSeconds;

        return Budget;
    }

    auto
        Make_RepairParams(
            const ck::FFragment_VoxelNavVolume_Params& InParams,
            const FBox& InDirtyBounds)
        -> ck::voxelnav::FRepairParams
    {
        auto RepairParams = ck::voxelnav::FRepairParams{};

        RepairParams._VolumeBounds = InParams.Get_VolumeBounds();
        RepairParams._FinestCellSizeUu = InParams.Get_FinestCellSizeUu();
        RepairParams._ClearanceUu = InParams.Get_ClearanceUu();
        RepairParams._DirtyBounds = InDirtyBounds;

        return RepairParams;
    }

    auto
        Make_RepairBudget()
        -> ck::voxelnav::FBuildBudget
    {
        constexpr auto MillisecondsToSeconds = 0.001f;

        auto Budget = ck::voxelnav::FBuildBudget{};

        // A repair rides the same wall-clock guard as a bake but gets its own probe budget: a repair runs
        // during gameplay rather than at load, so the number of probes a frame can afford is a different
        // question from what a bake may spend.
        Budget._MaxOccupancyProbes = UCk_Utils_VoxelNav_Settings_UE::Get_MaxRepairProbesPerTick();
        Budget._MaxSeconds =
            UCk_Utils_VoxelNav_Settings_UE::Get_MaxBuildMillisecondsPerTick() * MillisecondsToSeconds;

        return Budget;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoxelNavVolume_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams)
        -> void
    {
        using namespace ck_voxelnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto ParamsAreBakeable = Get_ParamsAreBakeable(InParams);

        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("VoxelNav Volume [{}] cannot be baked. Bounds [{}], finest cell size [{}]uu (minimum "
                 "[{}]uu - below that the cell is finer than the physics' own penetration slop), "
                 "clearance [{}]uu"),
            InVolumeEntity, InParams.Get_VolumeBounds(), InParams.Get_FinestCellSizeUu(),
            MinFinestCellSizeUu, InParams.Get_ClearanceUu())
        {}

        // The volume stays inert rather than arming a build that would bake garbage. A later
        // Request_Build on it fails loudly through the same validation.
        if (NOT ParamsAreBakeable)
        { return; }

        if (InParams.Get_AutoBuildOnSetup() == ECk_EnableDisable::Disable)
        { return; }

        InVolumeEntity.AddOrGet<FTag_VoxelNavVolume_NeedsBuild>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavVolume_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            FFragment_VoxelNavVolume_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InVolumeEntity, InBuildState, InRepairState, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InVolumeEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_VoxelNavVolume_HandleRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            const FCk_Request_VoxelNavVolume_Build& InRequest)
        -> void
    {
        const auto BuildIsAlreadyUnderway =
            InVolumeEntity.Has<FTag_VoxelNavVolume_NeedsBuild>() ||
            InVolumeEntity.Has<FTag_VoxelNavVolume_BuildInProgress>();

        if (BuildIsAlreadyUnderway && InRequest.Get_ForceRestart() == ECk_EnableDisable::Disable)
        {
            // The caller wanted a current bake and one is on its way, so their intent already holds -
            // an idempotent no-op reports Succeeded, not Failed.
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        // The delegate rides the request across the WHOLE multi-frame build, so a request being displaced
        // has to be completed here or the caller that supplied it waits forever.
        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        InBuildState._PendingRequest = InRequest;

        InVolumeEntity.Try_Remove<FTag_VoxelNavVolume_BuildInProgress>();
        InVolumeEntity.AddOrGet<FTag_VoxelNavVolume_NeedsBuild>();

        // An in-flight repair is ABANDONED rather than raced. Both publish onto the same octree slot, and a
        // repair carries the occupancy of the octree it was seeded from - so one finishing after a bake would
        // overwrite fresh navigation data with a structure derived from the pre-bake world. The dirty region
        // itself is kept: the bake answers it, and if the bake fails the repair still has to run.
        InVolumeEntity.Try_Remove<FTag_VoxelNavVolume_RepairInProgress>();
        voxelnav::Request_ResetRepair(InRepairState._Repair);
        InRepairState._Backend.Reset();

        voxelnav::Verbose(TEXT("VoxelNav Volume [{}] armed for a build"), InVolumeEntity);
    }

    auto
        FProcessor_VoxelNavVolume_HandleRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            const FCk_Request_VoxelNavVolume_CancelBuild& InRequest)
        -> void
    {
        auto Result = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InVolumeEntity, Result);

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        InVolumeEntity.Try_Remove<FTag_VoxelNavVolume_NeedsBuild>();
        InVolumeEntity.Try_Remove<FTag_VoxelNavVolume_BuildInProgress>();

        // Dropping the state IS the cancel - there is no flag for a stage to poll, which is what keeps
        // every stage a pure function. An already-published octree is untouched.
        voxelnav::Request_ResetBuild(InBuildState._Build);
        InBuildState._Backend.Reset();

        // Cancelling a volume that was not building is an idempotent no-op, and an idempotent no-op
        // succeeded: the caller's intent - no build running - holds either way.
        Result = ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_VoxelNavVolume_HandleRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            const FCk_Request_VoxelNavVolume_MarkDirty& InRequest)
        -> void
    {
        const auto DirtyBoundsAreUsable = InRequest.Get_DirtyBounds().IsValid != 0;

        CK_ENSURE_IF_NOT(DirtyBoundsAreUsable,
            TEXT("VoxelNav Volume [{}] was handed degenerate dirty bounds [{}] - there is no region to "
                 "repair"),
            InVolumeEntity, InRequest.Get_DirtyBounds())
        {}

        if (NOT DirtyBoundsAreUsable)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        // A volume with no bake has no occupancy to carry over, so there is nothing a LOCAL repair could
        // produce. The caller's intent does not hold and will not hold on a retry - that is Failed, and a
        // build is the repair for it.
        if (NOT InVolumeEntity.Has<FTag_VoxelNavVolume_Built>())
        {
            voxelnav::Verbose(
                TEXT("VoxelNav Volume [{}] was marked dirty before it published a bake - dropping the dirty "
                     "region; the first bake will rasterize it"), InVolumeEntity);

            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        InRepairState._PendingDirtyBounds = InRepairState._PendingDirtyBounds.IsValid != 0
            ? InRepairState._PendingDirtyBounds + InRequest.Get_DirtyBounds()
            : InRequest.Get_DirtyBounds();

        InRepairState._PendingRequests.Emplace(InRequest);

        InVolumeEntity.AddOrGet<FTag_VoxelNavVolume_NeedsRepair>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavVolume_StartBuild::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState) const
        -> void
    {
        using namespace ck_voxelnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        // Bound once here rather than reached through the fragment below: its members are private to this
        // processor, and a lambda body is a poor place to depend on that friendship.
        auto& Build = InBuildState._Build;
        auto& PendingRequest = InBuildState._PendingRequest;
        auto& HeldBackend = InBuildState._Backend;

        voxelnav::Request_ResetBuild(Build);
        HeldBackend.Reset();

        const auto DoFailBuild = [&]() -> void
        {
            voxelnav::Request_FailBuild(Build);

            PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);

            UUtils_Signal_OnVoxelNavVolumeBuildComplete::Broadcast(InVolumeEntity,
                MakePayload(InVolumeEntity, ECk_SucceededFailed::Failed, Build.Get_Stats()));
        };

        const auto ParamsAreBakeable = Get_ParamsAreBakeable(InParams);

        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("VoxelNav Volume [{}] was armed for a build but its params are not bakeable. Bounds [{}], "
                 "finest cell size [{}]uu, clearance [{}]uu"),
            InVolumeEntity, InParams.Get_VolumeBounds(), InParams.Get_FinestCellSizeUu(),
            InParams.Get_ClearanceUu())
        {}

        if (NOT ParamsAreBakeable)
        {
            DoFailBuild();
            return;
        }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        const auto WorldIsValid = ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{});

        CK_ENSURE_IF_NOT(WorldIsValid,
            TEXT("VoxelNav Volume [{}] has no world, so there is no geometry to bake against"),
            InVolumeEntity)
        {}

        if (NOT WorldIsValid)
        {
            DoFailBuild();
            return;
        }

        auto Backend = MakeUnique<FCk_VoxelNav_GeometryBackend_Jolt>(World);

        const auto BackendIsValid = Backend->Get_IsValid();

        // An invalid backend answers every probe "unoccupied". Accepting one would bake the entire volume
        // as free space and publish it as navigation data - agents would fly straight through the level.
        CK_ENSURE_IF_NOT(BackendIsValid,
            TEXT("VoxelNav Volume [{}] could not resolve a Jolt query session in world [{}]. Refusing to "
                 "bake: an unresolved session reports every probe unoccupied, which would publish the whole "
                 "volume as free space"),
            InVolumeEntity, World->GetName())
        {}

        if (NOT BackendIsValid)
        {
            DoFailBuild();
            return;
        }

        HeldBackend = MoveTemp(Backend);

        InVolumeEntity.AddOrGet<FTag_VoxelNavVolume_BuildInProgress>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavVolume_Build::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            FFragment_VoxelNavVolume_BuiltOctree& InBuiltOctree) const
        -> void
    {
        using namespace ck_voxelnav_volume_processor;

        const auto BackendIsHeld = InBuildState._Backend.IsValid();

        CK_ENSURE_IF_NOT(BackendIsHeld,
            TEXT("VoxelNav Volume [{}] is marked as building but holds no geometry backend"), InVolumeEntity)
        {}

        if (NOT BackendIsHeld)
        {
            voxelnav::Request_FailBuild(InBuildState._Build);
        }
        else
        {
            const auto StageBeforeSlice = InBuildState._Build.Get_Stage();

            voxelnav::Request_AdvanceBuild(InBuildState._Build, Make_BuildParams(InParams),
                *InBuildState._Backend, Make_BuildBudget(InParams));

            const auto SliceCrossedTheVolumeProbe =
                StageBeforeSlice <= ECk_VoxelNav_BuildStage::ProbeVolumeEmpty &&
                InBuildState._Build.Get_Stage() > ECk_VoxelNav_BuildStage::ProbeVolumeEmpty;

            // A volume the broadphase reports as empty bakes as pure free space, which is legal - and also
            // what a volume placed over LANDSCAPE looks like in a packaged build, where CkJolt extracts no
            // landscape heightfields at all. The bake proceeds; this is the one signal that the result may
            // be describing an absence of bodies rather than an absence of obstacles.
            CK_TRIGGER_ENSURE_IF(
                SliceCrossedTheVolumeProbe &&
                    InBuildState._Build.Get_Stats().Get_BroadphaseBodyCount() == 0,
                TEXT("VoxelNav Volume [{}] found NO static bodies anywhere in its bounds [{}] and will bake "
                     "as entirely free space. Legal for an empty volume; expected for one placed over "
                     "landscape, whose heightfields are only extracted into Jolt under WITH_EDITOR"),
                InVolumeEntity, InParams.Get_VolumeBounds());
        }

        if (NOT InBuildState._Build.Get_IsFinished())
        { return; }

        InVolumeEntity.Remove<FTag_VoxelNavVolume_BuildInProgress>();
        InBuildState._Backend.Reset();

        if (InBuildState._Build.Get_Stage() == ECk_VoxelNav_BuildStage::Failed)
        {
            // A failed REBUILD leaves the previously published octree alone: stale navigation data is
            // still navigation data, and dropping it would strand every agent using the volume.
            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            UUtils_Signal_OnVoxelNavVolumeBuildComplete::Broadcast(InVolumeEntity,
                MakePayload(InVolumeEntity, ECk_SucceededFailed::Failed, InBuildState._Build.Get_Stats()));

            return;
        }

        const auto BuildStats = InBuildState._Build.Get_Stats();

        InBuiltOctree._Octree = voxelnav::Request_ReleaseBuiltOctree(InBuildState._Build);
        ++InBuiltOctree._Epoch;

        InVolumeEntity.AddOrGet<FTag_VoxelNavVolume_Built>();

        // A completed bake re-rasterized the whole volume from live geometry, so every pending dirty region
        // has already been answered - and answered better. Carrying them into a repair would spend probes
        // re-deriving occupancy this bake just published, so their callers' intent holds and they complete.
        for (const auto& PendingRepairRequest : InRepairState._PendingRequests)
        { PendingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded); }

        InRepairState._PendingRequests.Reset();
        InRepairState._PendingDirtyBounds = FBox{ForceInit};
        InVolumeEntity.Try_Remove<FTag_VoxelNavVolume_NeedsRepair>();

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Succeeded);

        UUtils_Signal_OnVoxelNavVolumeBuildComplete::Broadcast(InVolumeEntity,
            MakePayload(InVolumeEntity, ECk_SucceededFailed::Succeeded, BuildStats));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavVolume_StartRepair::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            const FFragment_VoxelNavVolume_BuiltOctree& InBuiltOctree) const
        -> void
    {
        InVolumeEntity.Remove<MarkedDirtyBy>();

        auto& Repair = InRepairState._Repair;
        auto& HeldBackend = InRepairState._Backend;

        voxelnav::Request_ResetRepair(Repair);
        HeldBackend.Reset();

        const auto DoFailRepair = [&]() -> void
        {
            voxelnav::Request_FailRepair(Repair);

            for (const auto& PendingRequest : InRepairState._PendingRequests)
            { PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._PendingRequests.Reset();
            InRepairState._PendingDirtyBounds = FBox{ForceInit};

            UUtils_Signal_OnVoxelNavVolumeRepairComplete::Broadcast(InVolumeEntity,
                MakePayload(InVolumeEntity, ECk_SucceededFailed::Failed, Repair.Get_Stats()));
        };

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        const auto WorldIsValid = ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{});

        CK_ENSURE_IF_NOT(WorldIsValid,
            TEXT("VoxelNav Volume [{}] has no world, so there is no geometry to repair against"),
            InVolumeEntity)
        {}

        if (NOT WorldIsValid)
        {
            DoFailRepair();
            return;
        }

        auto Backend = MakeUnique<FCk_VoxelNav_GeometryBackend_Jolt>(World);

        const auto BackendIsValid = Backend->Get_IsValid();

        // Same reasoning as the bake, and worse here: an invalid backend reports every probe unoccupied, so
        // a repair accepting one would carve free space out of the published octree exactly where an
        // obstacle just arrived.
        CK_ENSURE_IF_NOT(BackendIsValid,
            TEXT("VoxelNav Volume [{}] could not resolve a Jolt query session in world [{}]. Refusing to "
                 "repair: an unresolved session reports every probe unoccupied, which would free the space "
                 "the dirty region was raised for"),
            InVolumeEntity, World->GetName())
        {}

        if (NOT BackendIsValid)
        {
            DoFailRepair();
            return;
        }

        if (NOT voxelnav::Request_BeginRepair(Repair, InBuiltOctree.Get_Octree()))
        {
            DoFailRepair();
            return;
        }

        HeldBackend = MoveTemp(Backend);

        InVolumeEntity.AddOrGet<FTag_VoxelNavVolume_RepairInProgress>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavVolume_Repair::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_RepairState& InRepairState,
            FFragment_VoxelNavVolume_BuiltOctree& InBuiltOctree) const
        -> void
    {
        using namespace ck_voxelnav_volume_processor;

        const auto BackendIsHeld = InRepairState._Backend.IsValid();

        CK_ENSURE_IF_NOT(BackendIsHeld,
            TEXT("VoxelNav Volume [{}] is marked as repairing but holds no geometry backend"), InVolumeEntity)
        {}

        if (NOT BackendIsHeld)
        {
            voxelnav::Request_FailRepair(InRepairState._Repair);
        }
        else
        {
            voxelnav::Request_AdvanceRepair(InRepairState._Repair,
                Make_RepairParams(InParams, InRepairState._PendingDirtyBounds),
                *InRepairState._Backend, Make_RepairBudget());
        }

        if (NOT InRepairState._Repair.Get_IsFinished())
        { return; }

        InVolumeEntity.Remove<FTag_VoxelNavVolume_RepairInProgress>();
        InRepairState._Backend.Reset();

        const auto RepairStats = InRepairState._Repair.Get_Stats();

        const auto PendingRequests = MoveTemp(InRepairState._PendingRequests);
        InRepairState._PendingRequests.Reset();

        // The dirty region is consumed whatever the outcome. A failed repair that kept it would re-open the
        // same failing repair every tick for as long as the volume lived.
        InRepairState._PendingDirtyBounds = FBox{ForceInit};

        auto RepairedOctree = voxelnav::Request_ReleaseRepairedOctree(InRepairState._Repair);

        const auto RepairSucceeded = RepairedOctree.IsValid() && RepairedOctree->Get_IsValid();

        if (RepairSucceeded)
        {
            // The swap is the publish: the previous octree survives in every TSharedPtr a search is still
            // holding, and the epoch bump is what makes a path planned against it read Stale.
            InBuiltOctree._Octree = MoveTemp(RepairedOctree);
            ++InBuiltOctree._Epoch;
        }

        const auto RequestResult = RepairSucceeded
            ? ECk_Request_OperationResult::Succeeded
            : ECk_Request_OperationResult::Failed;

        for (const auto& PendingRequest : PendingRequests)
        { PendingRequest.TryFireCompletion(InVolumeEntity, RequestResult); }

        UUtils_Signal_OnVoxelNavVolumeRepairComplete::Broadcast(InVolumeEntity, MakePayload(InVolumeEntity,
            RepairSucceeded ? ECk_SucceededFailed::Succeeded : ECk_SucceededFailed::Failed, RepairStats));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavVolume_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Requests& InRequests,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_RepairState& InRepairState)
        -> void
    {
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        for (const auto& PendingRepairRequest : InRepairState._PendingRequests)
        {
            PendingRepairRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        InRepairState._PendingRequests.Reset();

        InBuildState._Backend.Reset();
        InRepairState._Backend.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
