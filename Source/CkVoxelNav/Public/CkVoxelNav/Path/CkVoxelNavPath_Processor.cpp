#include "CkVoxelNavPath_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Path/CkVoxelNav_Path_Graph.h"
#include "CkVoxelNav/Path/CkVoxelNav_Path_Refine.h"
#include "CkVoxelNav/Settings/CkVoxelNav_ProjectSettings.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavPath_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavPath_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_path_processor
{
    auto
        Make_SearchParams(
            const ck::FFragment_VoxelNavPath_Params& InParams,
            const FCk_Request_VoxelNavPath_FindPath& InRequest,
            int32 InEpoch)
        -> ck::voxelnav::FPathSearchParams
    {
        auto SearchParams = ck::voxelnav::FPathSearchParams{};

        SearchParams._From = InRequest.Get_From();
        SearchParams._To = InRequest.Get_To();
        SearchParams._AgentRadiusUu = InParams.Get_AgentRadiusUu();
        SearchParams._HeuristicScale = InParams.Get_HeuristicScale();
        SearchParams._NodeSizeCompensation = InParams.Get_NodeSizeCompensation();
        SearchParams._MaxIterations = UCk_Utils_VoxelNav_Settings_UE::Get_MaxPathSearchIterations();
        SearchParams._PlannedAgainstEpoch = InEpoch;

        return SearchParams;
    }

    auto
        Make_RefineParams(
            const FCk_Request_VoxelNavPath_FindPath& InRequest)
        -> ck::voxelnav::FPathRefineParams
    {
        auto RefineParams = ck::voxelnav::FPathRefineParams{};

        RefineParams._VisibilityPruning = InRequest.Get_VisibilityPruning();
        RefineParams._Smoothing = InRequest.Get_Smoothing();
        RefineParams._SmoothingSubdivisions = InRequest.Get_SmoothingSubdivisions();

        return RefineParams;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoxelNavPath_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_VoxelNavPath_Params& InParams,
            FFragment_VoxelNavPath_Result& InResult,
            FFragment_VoxelNavPath_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InPathEntity, InParams, InResult, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InPathEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_VoxelNavPath_HandleRequests::
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_VoxelNavPath_Params& InParams,
            FFragment_VoxelNavPath_Result& InResult,
            const FCk_Request_VoxelNavPath_FindPath& InRequest)
        -> void
    {
        using namespace ck_voxelnav_path_processor;

        auto RequestResult = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InPathEntity, RequestResult);

        const auto Volume = InRequest.Get_Volume();

        const auto DoFailPath = [&](ECk_VoxelNav_PathSearchOutcome InOutcome, int32 InEpoch) -> void
        {
            InResult._Waypoints.Reset();
            InResult._Status = ECk_VoxelNav_PathStatus::Failed;
            InResult._Outcome = InOutcome;
            InResult._Volume = Volume;
            InResult._PlannedAgainstEpoch = InEpoch;
            InResult._RawWaypointCount = 0;
            InResult._PathLengthUu = 0.0f;

            UUtils_Signal_OnVoxelNavPathFailed::Broadcast(InPathEntity, MakePayload(InPathEntity, InOutcome));
        };

        const auto VolumeIsUsable = ck::IsValid(Volume) && UCk_Utils_VoxelNavVolume_UE::Has(Volume);

        CK_ENSURE_IF_NOT(VolumeIsUsable,
            TEXT("VoxelNav Path [{}] was asked to plan against [{}], which is not a VoxelNav Volume"),
            InPathEntity, Volume)
        {}

        if (NOT VolumeIsUsable)
        {
            constexpr auto NoEpoch = 0;
            DoFailPath(ECk_VoxelNav_PathSearchOutcome::InvalidRequest, NoEpoch);
            return;
        }

        const auto& BuiltOctree = Volume.Get<FFragment_VoxelNavVolume_BuiltOctree>();

        // A copy of the published pointer, held for the whole search: a rebuild finishing mid-search swaps
        // the volume's pointer, and this copy is what keeps the structure being walked whole.
        const auto Octree = BuiltOctree.Get_Octree();
        const auto Epoch = BuiltOctree.Get_Epoch();

        // A volume that has not baked yet is an ordinary timing situation, not a caller error - the agent
        // simply asked before the world was ready, and it may ask again.
        if (NOT Octree.IsValid())
        {
            DoFailPath(ECk_VoxelNav_PathSearchOutcome::NoBake, Epoch);
            return;
        }

        const auto Plan = voxelnav::Search_PathGraph(
            *Octree, BuiltOctree.Get_VolumeId(), Make_SearchParams(InParams, InRequest, Epoch));

        if (Plan._Outcome != ECk_VoxelNav_PathSearchOutcome::Succeeded)
        {
            voxelnav::Verbose(TEXT("VoxelNav Path [{}] found no route from [{}] to [{}] on Volume [{}]: [{}]"),
                InPathEntity, InRequest.Get_From(), InRequest.Get_To(), Volume, Plan._Outcome);

            DoFailPath(Plan._Outcome, Plan._PlannedAgainstEpoch);
            return;
        }

        const auto Refined = voxelnav::Refine_Waypoints(*Octree, Plan._Waypoints, Make_RefineParams(InRequest));

        InResult._Waypoints = Refined._Waypoints;
        InResult._Status = ECk_VoxelNav_PathStatus::Ready;
        InResult._Outcome = Plan._Outcome;
        InResult._Volume = Volume;
        InResult._PlannedAgainstEpoch = Plan._PlannedAgainstEpoch;
        InResult._RawWaypointCount = Refined._RawWaypointCount;
        InResult._PathLengthUu = Refined._RefinedLengthUu;

        RequestResult = ECk_Request_OperationResult::Succeeded;

        UUtils_Signal_OnVoxelNavPathReady::Broadcast(InPathEntity, MakePayload(InPathEntity));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavPath_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_VoxelNavPath_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InPathEntity, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
