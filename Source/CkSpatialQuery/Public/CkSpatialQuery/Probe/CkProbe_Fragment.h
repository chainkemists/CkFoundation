#pragma once

#include "CkProbe_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace JPH
{
    class Body;
}

class UCk_Utils_Probe_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace details
    {
        class FProcessor_BoxProbe_Setup;
        class FProcessor_SphereProbe_Setup;
        class FProcessor_CapsuleProbe_Setup;
        class FProcessor_CylinderProbe_Setup;
    }

    CK_DEFINE_ECS_TAG(FTag_Probe_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Probe_Updated);
    CK_DEFINE_ECS_TAG(FTag_Probe_Overlapping);
    CK_DEFINE_ECS_TAG(FTag_Probe_Disabled);

    CK_DEFINE_ECS_TAG(FTag_Probe_MotionType_Static);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Probe_Params = FCk_Fragment_Probe_ParamsData;
    using FFragment_Probe_DebugInfo = FCk_Probe_DebugInfo;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Probe_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Current);

    public:
        friend class details::FProcessor_BoxProbe_Setup;
        friend class details::FProcessor_SphereProbe_Setup;
        friend class details::FProcessor_CapsuleProbe_Setup;
        friend class details::FProcessor_CylinderProbe_Setup;
        friend class FProcessor_Probe_UpdateTransform;
        friend class FProcessor_Probe_HandleRequests;
        friend class FProcessor_Probe_Teardown;
        friend class UCk_Utils_Probe_UE;

    private:
        JPH::Body* _RigidBody = nullptr;
        TSet<FCk_Probe_OverlapInfo> _CurrentOverlaps;

    public:
        CK_PROPERTY_GET(_RigidBody);
        CK_PROPERTY_GET(_CurrentOverlaps);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Probe_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Requests);

    public:
        friend class FProcessor_Probe_HandleRequests;
        friend class UCk_Utils_Probe_UE;

    public:
        using RequestType = std::variant<FCk_Request_Probe_BeginOverlap, FCk_Request_Probe_OverlapPersisted,
            FCk_Request_Probe_EndOverlap, FCk_Request_Probe_EnableDisable>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeBeginOverlap,
        FCk_Delegate_Probe_OnBeginOverlap_MC,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnBeginOverlap);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeOverlapPersisted,
        FCk_Delegate_Probe_OnOverlapPersisted_MC,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnOverlapPersisted);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeEndOverlap,
        FCk_Delegate_Probe_OnEndOverlap_MC,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnEndOverlap);
}

// --------------------------------------------------------------------------------------------------------------------
