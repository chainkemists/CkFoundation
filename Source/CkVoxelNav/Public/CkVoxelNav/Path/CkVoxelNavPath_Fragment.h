#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoxelNavPath_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_VoxelNavPath_Params = FCk_Fragment_VoxelNavPath_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    /** The last plan this agent asked for.
     *
     *  `_Volume` and `_PlannedAgainstEpoch` are kept together on purpose: the pair is what makes staleness
     *  answerable at read time, without a rebuild having to find every path that ever planned against it. */
    struct CKVOXELNAV_API FFragment_VoxelNavPath_Result
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavPath_Result);

        friend class FProcessor_VoxelNavPath_HandleRequests;
        friend class ::UCk_Utils_VoxelNavPath_UE;

    private:
        /** The route to fly, AFTER whatever refinement the request asked for: the From position, the cells
         *  the search kept, then the To position. Empty unless Ready. Refinement writes back here rather
         *  than alongside, because a caller reading two waypoint lists would have to know which one the
         *  agent is meant to follow. */
        TArray<FVector> _Waypoints;

        ECk_VoxelNav_PathStatus _Status = ECk_VoxelNav_PathStatus::None;

        ECk_VoxelNav_PathSearchOutcome _Outcome = ECk_VoxelNav_PathSearchOutcome::Succeeded;

        FCk_Handle_VoxelNavVolume _Volume;

        int32 _PlannedAgainstEpoch = 0;

        // What the search produced before refinement. Against the current waypoint count it is the only way
        // to see whether refinement did anything - the raw path is not kept.
        int32 _RawWaypointCount = 0;

        float _PathLengthUu = 0.0f;

    public:
        CK_PROPERTY_GET(_Waypoints);
        CK_PROPERTY_GET(_Status);
        CK_PROPERTY_GET(_Outcome);
        CK_PROPERTY_GET(_Volume);
        CK_PROPERTY_GET(_PlannedAgainstEpoch);
        CK_PROPERTY_GET(_RawWaypointCount);
        CK_PROPERTY_GET(_PathLengthUu);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOXELNAV_API FFragment_VoxelNavPath_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavPath_Requests);

        friend class FProcessor_VoxelNavPath_HandleRequests;
        friend class ::UCk_Utils_VoxelNavPath_UE;

    public:
        using RequestType = std::variant<FCk_Request_VoxelNavPath_FindPath>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVOXELNAV_API,
        OnVoxelNavPathReady,
        FCk_Delegate_VoxelNavPath_OnPathReady,
        FCk_Handle_VoxelNavPath);

    // Carries WHY, because every failure reason asks the caller for a different repair.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVOXELNAV_API,
        OnVoxelNavPathFailed,
        FCk_Delegate_VoxelNavPath_OnPathFailed,
        FCk_Handle_VoxelNavPath,
        ECk_VoxelNav_PathSearchOutcome);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VoxelNavPath_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
