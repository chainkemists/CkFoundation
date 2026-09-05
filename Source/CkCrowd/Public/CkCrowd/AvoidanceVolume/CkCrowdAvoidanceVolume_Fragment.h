#pragma once

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment_Data.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Algorithm.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

namespace ck
{
    using FFragment_CrowdAvoidanceVolume_Params = FCk_Fragment_CrowdAvoidanceVolume_ParamsData;

    CK_DEFINE_ECS_TAG(FTag_CrowdAvoidanceVolume_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_CrowdAvoidanceVolume_HasRuntime);
    CK_DEFINE_ECS_TAG(FTag_CrowdAvoidanceVolume_Invalid);

    struct CKCROWD_API FFragment_CrowdAvoidanceVolume_ProbeRef
    {
        CK_GENERATED_BODY(FFragment_CrowdAvoidanceVolume_ProbeRef);

        friend class FProcessor_CrowdAvoidanceVolume_Setup;
        friend class FProcessor_CrowdAvoidanceVolume_Monitor;
        friend class FProcessor_CrowdAvoidanceVolume_EndPlay;

    private:
        FCk_Handle_Probe _ProbeChild;
        FCk_Handle_NavSurfaceMarkup _Markup;
        crowd_avoidance_volume::FCk_Obb _AuthoredObb;
        crowd_avoidance_volume::FCk_Obb _PaintedObb;
        FTransform _AuthoredTransform = FTransform::Identity;
        ECk_CrowdAvoidanceVolume_TraversalPolicy _TraversalPolicy =
            ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible;
        float _SecondsSincePaint = 0.0f;
        uint64 _ConfirmationSerial = 0;
        bool _ConfirmedOnMesh = false;

    public:
        CK_PROPERTY_GET(_ProbeChild);
        CK_PROPERTY_GET(_Markup);
        CK_PROPERTY_GET(_AuthoredObb);
        CK_PROPERTY_GET(_PaintedObb);
        CK_PROPERTY_GET(_AuthoredTransform);
        CK_PROPERTY_GET(_TraversalPolicy);
        CK_PROPERTY_GET(_SecondsSincePaint);
        CK_PROPERTY_GET(_ConfirmationSerial);
        CK_PROPERTY_GET(_ConfirmedOnMesh);
    };

    struct FCk_CrowdAvoidanceVolume_Retirement
    {
        int64 _VolumeIdentity = 0;
        FName _VolumeDebugName;
        FTransform _YawWorldTransform = FTransform::Identity;
        ECk_CrowdAvoidanceVolume_TraversalPolicy _TraversalPolicy =
            ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible;
        crowd_avoidance_volume::FCk_Obb _PhysicalObb;
        crowd_avoidance_volume::FCk_Obb _PaintedObb;
        uint64 _ConfirmationSerial = 0;
        uint64 _NavigationRevisionAtUnregister = 0;
    };

    // World-scoped value-only records that outlive their destroyed volume owners until Recast
    // confirms the corresponding area paint has disappeared.
    struct CKCROWD_API FFragment_CrowdAvoidanceVolume_Retirements
    {
        CK_GENERATED_BODY(FFragment_CrowdAvoidanceVolume_Retirements);

        friend class FProcessor_CrowdAvoidanceVolume_EndPlay;
        friend class FProcessor_CrowdAgent_PathRefresh;

    private:
        TArray<FCk_CrowdAvoidanceVolume_Retirement> _Records;

    public:
        CK_PROPERTY_GET(_Records);
    };
}

// --------------------------------------------------------------------------------------------------------------------
