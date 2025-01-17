#pragma once

#include "CkProbe_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkProbe_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace JPH
{
    class Body;
}

class UCk_Utils_Probe_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Probe_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Probe_Updated);
    CK_DEFINE_ECS_TAG(FTag_Probe_Overlapping);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Probe_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Params);

    public:
        using ParamsType = FCk_Fragment_Probe_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Probe_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Probe_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Current);

    public:
        friend class FProcessor_Probe_Setup;
        friend class FProcessor_Probe_UpdateTransform;
        friend class FProcessor_Probe_HandleRequests;
        friend class FProcessor_Probe_Teardown;
        friend class UCk_Utils_Probe_UE;

    private:
        // Add your properties here

        JPH::Body* _RigidBody = nullptr;
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
            FCk_Request_Probe_EndOverlap>;
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

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Probe_Replicate;
}

UCLASS(Blueprintable)
class CKSPATIALQUERY_API UCk_Fragment_Probe_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Probe_Rep);

public:
    friend class ck::FProcessor_Probe_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------
