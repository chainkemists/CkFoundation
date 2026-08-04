#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkVoxelNav/Backend/CkVoxelNav_GeometryBackend_Jolt.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Build.h"
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

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoxelNavVolume_Params = FCk_Fragment_VoxelNavVolume_ParamsData;

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
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        // Null until the first build completes.
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

    struct CKVOXELNAV_API FFragment_VoxelNavVolume_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_Requests);

        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    public:
        using RequestType = std::variant<FCk_Request_VoxelNavVolume_Build,
                                         FCk_Request_VoxelNavVolume_CancelBuild>;
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

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VoxelNavVolume_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
