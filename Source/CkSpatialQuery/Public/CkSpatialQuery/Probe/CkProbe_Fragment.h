#pragma once

#include "CkProbe_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Probe_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck { namespace details
    {
        template <typename T_ShapeFragment>
        class TProcessor_ProbeSetup;
    }

    CK_DEFINE_ECS_TAG(FTag_Probe_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Probe_LinearCast);
    CK_DEFINE_ECS_TAG(FTag_Probe_Overlapping);
    CK_DEFINE_ECS_TAG(FTag_Probe_Disabled);
    CK_DEFINE_ECS_TAG(FTag_Probe_DebugDraw);
    CK_DEFINE_ECS_TAG(FTag_Probe_ShapeUpdated);
    CK_DEFINE_ECS_TAG(FTag_Probe_MotionType_Static);
    CK_DEFINE_ECS_TAG(FTag_Probe_PersistContacts);
    CK_DEFINE_ECS_TAG(FTag_ProbeTrace);
    CK_DEFINE_ECS_TAG(FTag_ProbeTrace_DebugDraw);
    CK_DEFINE_ECS_TAG(FTag_ProbeTrace_Disabled);

    // --------------------------------------------------------------------------------------------------------------------

    // The retained immutable residue of FCk_Probe_Spec: the matching half (read per contact pair
    // on the hottest path in the module) plus the motion config Setup and the public getters serve.
    // _StartingState and _PersistContacts are DISSOLVED — they live only in FTag_Probe_Disabled /
    // FTag_Probe_PersistContacts, set at Add.
    struct CKSPATIALQUERY_API FFragment_Probe_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Params);

    private:
        FGameplayTag _ProbeName;
        ECk_ProbeResponse_Policy _ResponsePolicy = ECk_ProbeResponse_Policy::Notify;
        FGameplayTagContainer _Filter;
        ECk_Probe_ContextOverlapPolicy _ContextOverlapPolicy = ECk_Probe_ContextOverlapPolicy::DifferentContextOnly;
        ECk_MotionType _MotionType = ECk_MotionType::Static;
        ECk_MotionQuality _MotionQuality = ECk_MotionQuality::Discrete;
        FCk_Probe_SurfaceInfo _SurfaceInfo;

    public:
        CK_PROPERTY_GET(_ProbeName);
        CK_PROPERTY_GET(_ResponsePolicy);
        CK_PROPERTY_GET(_Filter);
        CK_PROPERTY_GET(_ContextOverlapPolicy);
        CK_PROPERTY_GET(_MotionType);
        CK_PROPERTY_GET(_MotionQuality);
        CK_PROPERTY_GET(_SurfaceInfo);

    public:
        FFragment_Probe_Params() = default;
        explicit FFragment_Probe_Params(const FCk_Probe_Spec& InSpec)
            : _ProbeName(InSpec.Get_ProbeName())
            , _ResponsePolicy(InSpec.Get_ResponsePolicy())
            , _Filter(InSpec.Get_Filter())
            , _ContextOverlapPolicy(InSpec.Get_ContextOverlapPolicy())
            , _MotionType(InSpec.Get_MotionType())
            , _MotionQuality(InSpec.Get_MotionQuality())
            , _SurfaceInfo(InSpec.Get_SurfaceInfo())
        {}
    };
    using FFragment_Probe_DebugInfo = FCk_Probe_DebugInfo;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Probe_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Current);

    public:
        template <typename T_ShapeFragment>
        friend class details::TProcessor_ProbeSetup;
        friend class FProcessor_Probe_UpdateTransform;
        friend class FProcessor_Probe_HandleRequests;
        friend class FProcessor_Probe_EndPlay;
        friend class UCk_Utils_Probe_UE;

    private:
        JPH::BodyID _BodyId;
        TSet<FCk_Probe_OverlapInfo> _CurrentOverlaps;
        entt::connection _ShapeDimensionsChangedConnection;

    public:
        CK_PROPERTY_GET(_BodyId);
        CK_PROPERTY_GET(_CurrentOverlaps);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Probe_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Probe_Requests);

        using RequestType = std::variant<FCk_Request_Probe_BeginOverlap, FCk_Request_Probe_OverlapUpdated,
            FCk_Request_Probe_EndOverlap, FCk_Request_Probe_EnableDisable>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ProbeTrace_RayCast = FCk_Probe_RayCastPersistent_Settings;
    using FFragment_ProbeTrace_ShapeCast = FCk_Probe_ShapeCastPersistent_Settings;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeBeginOverlap,
        FCk_Delegate_Probe_OnBeginOverlap,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnBeginOverlap);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeOverlapUpdated,
        FCk_Delegate_Probe_OnOverlapUpdated,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnOverlapUpdated);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeEndOverlap,
        FCk_Delegate_Probe_OnEndOverlap,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnEndOverlap);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeEnableDisable,
        FCk_Delegate_Probe_OnEnableDisable,
        FCk_Handle_Probe,
        FCk_Probe_Payload_OnEnableDisable);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeTraceBeginOverlap,
        FCk_Delegate_ProbeTrace_OnBeginOverlap,
        FCk_Handle_ProbeTrace,
        FCk_Probe_Payload_OnBeginOverlap);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeTraceOverlapUpdated,
        FCk_Delegate_ProbeTrace_OnOverlapUpdated,
        FCk_Handle_ProbeTrace,
        FCk_Probe_Payload_OnOverlapUpdated);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSPATIALQUERY_API,
        OnProbeTraceEndOverlap,
        FCk_Delegate_ProbeTrace_OnEndOverlap,
        FCk_Handle_ProbeTrace,
        FCk_Probe_Payload_OnEndOverlap);
}

// --------------------------------------------------------------------------------------------------------------------
