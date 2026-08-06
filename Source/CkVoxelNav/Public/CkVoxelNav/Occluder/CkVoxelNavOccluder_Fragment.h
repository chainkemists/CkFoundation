#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkVoxelNav/Occluder/CkVoxelNavOccluder_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoxelNavOccluder_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoxelNavOccluder_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoxelNavOccluder_Params = FCk_VoxelNavOccluder_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    /** The pose the volumes were last told about.
     *
     *  It holds BOUNDS rather than a transform because bounds are what a repair consumes, and because one
     *  box comparison covers translation, rotation and scale in the same units the threshold is expressed
     *  in. Upstream compared transforms with `FTransform::Equals` at its default epsilon, so an occluder
     *  jittering a thousandth of a unit rebuilt navigation data every frame while changing no voxel.
     *
     *  It is updated only when a dirty region is actually EMITTED, so slow sub-threshold drift accumulates
     *  against the last pose that was published rather than against last frame's - drift below the
     *  threshold still crosses it eventually instead of being lost one frame at a time. */
    struct CKVOXELNAV_API FFragment_VoxelNavOccluder_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavOccluder_Current);

        friend class FProcessor_VoxelNavOccluder_Setup;
        friend class FProcessor_VoxelNavOccluder_Track;
        friend class ::UCk_Utils_VoxelNavOccluder_UE;

    private:
        FBox _TrackedBounds = FBox{ForceInit};
        int32 _TimesDirtied = 0;

    public:
        CK_PROPERTY_GET(_TrackedBounds);
        CK_PROPERTY_GET(_TimesDirtied);
    };
}

// --------------------------------------------------------------------------------------------------------------------
