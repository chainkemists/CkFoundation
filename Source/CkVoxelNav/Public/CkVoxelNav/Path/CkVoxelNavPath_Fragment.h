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
        // The From position, every cell centre in order, then the To position. Empty unless Ready.
        TArray<FVector> _Waypoints;

        ECk_VoxelNav_PathStatus _Status = ECk_VoxelNav_PathStatus::None;

        ECk_VoxelNav_PathSearchOutcome _Outcome = ECk_VoxelNav_PathSearchOutcome::Succeeded;

        FCk_Handle_VoxelNavVolume _Volume;

        int32 _PlannedAgainstEpoch = 0;

    public:
        CK_PROPERTY_GET(_Waypoints);
        CK_PROPERTY_GET(_Status);
        CK_PROPERTY_GET(_Outcome);
        CK_PROPERTY_GET(_Volume);
        CK_PROPERTY_GET(_PlannedAgainstEpoch);
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
