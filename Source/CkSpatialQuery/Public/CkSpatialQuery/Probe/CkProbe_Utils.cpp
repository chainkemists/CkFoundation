#include "CkProbe_Utils.h"

#include "CkShapes/CkShapes_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/RayCast.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams,
        const FCk_Probe_DebugInfo& InDebugInfo)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(UCk_Utils_Shapes_UE::Has_Any(InHandle),
        TEXT("Cannot Add a Probe to Entity [{}] because it does NOT have any Shape"), InHandle)
    {
        return {};
    }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ProbeName()),
        TEXT("Cannot Add a Probe to Entity [{}] because it has INVALID Name"), InHandle)
    {
        return {};
    }

    switch (const auto& ResponsePolicy = InParams.Get_ResponsePolicy())
    {
        case ECk_ProbeResponse_Policy::Notify:
        {
            CK_ENSURE(
                NOT InParams.Get_Filter().IsEmpty(),
                TEXT("Ill Configured Probe added to Entity [{}]!\n"
                    "A Non-Empty Filter is required for a Probe with Response Policy set to [{}]"),
                InHandle,
                ResponsePolicy);

            break;
        }
        case ECk_ProbeResponse_Policy::Silent:
        {
            CK_ENSURE(
                InParams.Get_Filter().IsEmpty(),
                TEXT("Ill Configured Probe added to Entity [{}]!\n"
                    "An Empty Filter is required for a Probe with Response Policy set to [{}]"),
                InHandle,
                ResponsePolicy);

            break;
        }
        default:
        {
            CK_INVALID_ENUM(ResponsePolicy);
            break;
        };
    }

    InHandle.Add<ck::FFragment_Probe_Params>(InParams);
    InHandle.Add<ck::FFragment_Probe_DebugInfo>(InDebugInfo);
    InHandle.Add<ck::FFragment_Probe_Current>();

    if (InParams.Get_MotionQuality() == ECk_MotionQuality::LinearCast && InParams.Get_MotionType() != ECk_MotionType::Static)
    { InHandle.Add<ck::FTag_Probe_LinearCast>(); }

    InHandle.Add<ck::FTag_Probe_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Probe_UE, FCk_Handle_Probe,
    ck::FFragment_Probe_Params, ck::FFragment_Probe_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Get_Name(
        const FCk_Handle_Probe& InProbe)
        -> FGameplayTag
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_ProbeName();
}

auto
    UCk_Utils_Probe_UE::
    Get_ResponsePolicy(
        const FCk_Handle_Probe& InProbe)
        -> ECk_ProbeResponse_Policy
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_ResponsePolicy();
}

auto
    UCk_Utils_Probe_UE::
    Get_Filter(
        const FCk_Handle_Probe& InProbe)
        -> FGameplayTagContainer
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_Filter();
}

auto
    UCk_Utils_Probe_UE::
    Get_SurfaceInfo(
        const FCk_Handle_Probe& InProbe)
        -> FCk_Probe_SurfaceInfo
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_SurfaceInfo();
}

auto
    UCk_Utils_Probe_UE::
    Get_IsEnabledDisabled(
        const FCk_Handle_Probe& InProbe)
        -> ECk_EnableDisable
{
    if (InProbe.Has<ck::FTag_Probe_Disabled>())
    {
        return ECk_EnableDisable::Disable;
    }

    return ECk_EnableDisable::Enable;
}

auto
    UCk_Utils_Probe_UE::
    Get_IsOverlapping(
        const FCk_Handle_Probe& InProbe)
        -> bool
{
    return InProbe.Has<ck::FTag_Probe_Overlapping>();
}

auto
    UCk_Utils_Probe_UE::
    Get_IsOverlappingWith(
        const FCk_Handle_Probe& InProbe,
        const FCk_Handle& InOtherEntity)
        -> bool
{
    const auto OtherProbe = Cast(InOtherEntity);

    if (ck::Is_NOT_Valid(OtherProbe))
    { return false; }

    return InProbe.Get<ck::FFragment_Probe_Current>().Get_CurrentOverlaps().Contains(FCk_Probe_OverlapInfo{
       OtherProbe
    });
}

auto
    UCk_Utils_Probe_UE::
    Get_CanOverlapWith(
        const FCk_Handle_Probe& InA,
        const FCk_Handle_Probe& InB)
        -> bool
{
    if (Get_ResponsePolicy(InA) == ECk_ProbeResponse_Policy::Silent)
    {
        return false;
    }

    const auto& Filter = Get_Filter(InA);
    const auto& Name = Get_Name(InB);

    return Name.MatchesAny(Filter);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Request_BeginOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_BeginOverlap& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_OverlapUpdated(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_OverlapUpdated& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EndOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EndOverlap& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EnableDisable(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EnableDisable& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

namespace ck::details
{
    class CastRayCollector : public JPH::CastRayCollector
    {
    public:
        CK_GENERATED_BODY(CastRayCollector);

    public:
        struct FCk_ProbeBeginOverlaps
        {
            CK_GENERATED_BODY(FCk_ProbeBeginOverlaps);

            FCk_Handle_Probe _Probe;
            TOptional<FCk_Request_Probe_BeginOverlap> _BeginOverlap;
            TOptional<FCk_Request_Probe_OverlapUpdated> _UpdateOverlap;

            CK_DEFINE_CONSTRUCTORS(FCk_ProbeBeginOverlaps, _Probe, _BeginOverlap, _UpdateOverlap);

            CK_PROPERTY_GET(_Probe);
            CK_PROPERTY_GET(_BeginOverlap);
            CK_PROPERTY_GET(_UpdateOverlap);
        };

    public:
        auto
        AddHit(
            const ResultType& InResult)
        -> void override
        {
            const auto Entity = static_cast<FCk_Entity::IdType>(_BodyInterface->GetUserData(InResult.mBodyID));

            if (_AnyHandle.Get_Entity().Get_ID() == Entity)
            { return; }

            const auto OtherProbe = UCk_Utils_Probe_UE::Cast(_AnyHandle.Get_ValidHandle(Entity));

            _Hits.Emplace(std::make_pair(OtherProbe, InResult.mFraction));
        }

    private:
        FCk_Handle _AnyHandle;
        const JPH::BodyInterface* _BodyInterface;

        TArray<std::pair<FCk_Handle_Probe, float>> _Hits;

    public:
        CK_PROPERTY_GET(_Hits);

        CK_DEFINE_CONSTRUCTOR(CastRayCollector, _AnyHandle, _BodyInterface);
    };
}

auto
    UCk_Utils_Probe_UE::
    Request_LineTrace(
        const FCk_Handle& InAnyHandle,
        const FVector InStartPos,
        const FVector InEndPos)
    -> TArray<FCk_Probe_RayCast_Result>
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();
    const auto& BodyInterface = PhysicsSystem->GetBodyInterface();

    const auto& RayCast = JPH::RRayCast{JPH::RayCast{ck::jolt::Conv(InStartPos), ck::jolt::Conv(InEndPos - InStartPos)}};

    const auto& RayCastSettings = JPH::RayCastSettings{JPH::EBackFaceMode::CollideWithBackFaces, JPH::EBackFaceMode::CollideWithBackFaces};
    auto Collector = ck::details::CastRayCollector{InAnyHandle, &BodyInterface};

    PhysicsSystem->GetNarrowPhaseQuery().CastRay(RayCast, RayCastSettings, Collector);

    auto Result = TArray<FCk_Probe_RayCast_Result>{};
    for (const auto& [Fst, Snd] : Collector.Get_Hits())
    {
        Result.Emplace(FCk_Probe_RayCast_Result
        {
            Fst,
            InStartPos + Snd * (InEndPos - InStartPos)
        });
    }
    return Result;
}

auto
    UCk_Utils_Probe_UE::
    Request_LineTrace_Filter(
        const FCk_Handle& InAnyHandle,
        FVector InStartPos,
        FVector InEndPos,
        FGameplayTagContainer InFilter)
    -> TArray<FCk_Probe_RayCast_Result>
{
    const auto& Result = Request_LineTrace(InAnyHandle, InStartPos, InEndPos);

    auto FilteredResult = decltype(Result){};

    for (const auto& Hit : Result)
    {
        if (NOT Get_Name(Hit.Get_Probe()).MatchesAny(InFilter))
        { continue; }

        FilteredResult.Emplace(Hit);
    }

    return FilteredResult;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Bind to OnBeginOverlap for Probe [{}] because its Response Policy is NOT Notify"), InProbeEntity)
    {
        return InProbeEntity;
    }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeBeginOverlap, InProbeEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Unbind from OnBeginOverlap for Probe [{}] because its Response Policy is NOT Notify"),
        InProbeEntity)
    {
        return InProbeEntity;
    }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeBeginOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnOverlapUpdated(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnOverlapUpdated& InDelegate)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Bind to OnOverlapUpdated for Probe [{}] because its Response Policy is NOT Notify"),
        InProbeEntity)
    {
        return InProbeEntity;
    }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeOverlapUpdated, InProbeEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnOverlapUpdated(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnOverlapUpdated& InDelegate)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Unbind from OnOverlapUpdated for Probe [{}] because its Response Policy is NOT Notify"),
        InProbeEntity)
    {
        return InProbeEntity;
    }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeOverlapUpdated, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnEndOverlap(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Bind to OnEndOverlap for Probe [{}] because its Response Policy is NOT Notify"), InProbeEntity)
    {
        return InProbeEntity;
    }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Unbind from OnEndOverlap for Probe [{}] because its Response Policy is NOT Notify"), InProbeEntity)
    {
        return InProbeEntity;
    }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnEnableDisable(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnEnableDisable& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeEnableDisable, InProbeEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEnableDisable& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeEnableDisable, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    Request_MarkProbe_AsOverlapping(
        FCk_Handle_Probe& InProbeEntity)
        -> void
{
    InProbeEntity.AddOrGet<ck::FTag_Probe_Overlapping>();
}

auto
    UCk_Utils_Probe_UE::
    Request_MarkProbe_AsNotOverlapping(
        FCk_Handle_Probe& InProbeEntity)
        -> void
{
    InProbeEntity.Remove<ck::FTag_Probe_Overlapping>();
}

// --------------------------------------------------------------------------------------------------------------------
