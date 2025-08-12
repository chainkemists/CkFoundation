#include "CkProbe_Utils.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkShapes/CkShapes_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/ShapeCast.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::details
{
    class CastRayCollector : public JPH::CastRayCollector
    {
    public:
        CK_GENERATED_BODY(CastRayCollector);

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

    // --------------------------------------------------------------------------------------------------------------------

    class CastShapeCollector : public JPH::CastShapeCollector
    {
    public:
        CK_GENERATED_BODY(CastShapeCollector);

    public:
        auto
            AddHit(
                const JPH::ShapeCastResult& InResult)
            -> void override
        {
            const auto Entity = static_cast<FCk_Entity::IdType>(_BodyInterface->GetUserData(InResult.mBodyID2));

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

        CK_DEFINE_CONSTRUCTOR(CastShapeCollector, _AnyHandle, _BodyInterface);
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams,
        const FCk_Probe_DebugInfo& InDebugInfo)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(UCk_Utils_Shapes_UE::Has_Any(InHandle),
        TEXT("Cannot Add a Probe to Entity [{}] because it does NOT have any Shape"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ProbeName()),
        TEXT("Cannot Add a Probe to Entity [{}] because it has INVALID Name"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Probe_Params>(InParams);
    InHandle.Add<ck::FFragment_Probe_DebugInfo>(InDebugInfo);
    InHandle.Add<ck::FFragment_Probe_Current>();

    if (InParams.Get_MotionQuality() == ECk_MotionQuality::LinearCast && InParams.Get_MotionType() != ECk_MotionType::Static)
    { InHandle.Add<ck::FTag_Probe_LinearCast>(); }

    InHandle.Add<ck::FTag_Probe_NeedsSetup>();

    auto ProbeHandle = Cast(InHandle);

    Request_EnableDisable(ProbeHandle, FCk_Request_Probe_EnableDisable{InParams.Get_StartingState()});

    return ProbeHandle;
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
    Get_MotionType(
        const FCk_Handle_Probe& InProbe)
    -> ECk_MotionType
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_MotionType();
}

auto
    UCk_Utils_Probe_UE::
    Get_MotionQuality(
        const FCk_Handle_Probe& InProbe)
    -> ECk_MotionQuality
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_MotionQuality();
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
    const auto Result = InProbe.Get<ck::FFragment_Probe_Current>().Get_CurrentOverlaps().Contains(FCk_Probe_OverlapInfo{
       InOtherEntity
    });

    if (Result)
    { return Result; }

    return InProbe.Get<ck::FFragment_Probe_Current>().Get_CurrentOverlaps().Contains(FCk_Probe_OverlapInfo{
       UCk_Utils_ContextOwner_UE::Get_ContextOwner(InOtherEntity)
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
    { return false; }

    const auto& Filter = Get_Filter(InA);

    if (Filter.IsEmpty())
    { return true; }

    const auto& Name = Get_Name(InB);

    return Name.MatchesAny(Filter);
}

auto
    UCk_Utils_Probe_UE::
    Get_CurrentOverlaps(
        const FCk_Handle_Probe& InProbe)
    -> TSet<FCk_Probe_OverlapInfo>
{
    return InProbe.Get<ck::FFragment_Probe_Current>().Get_CurrentOverlaps();
}

auto
    UCk_Utils_Probe_UE::
    Request_EnableDisableDebugDraw(
        FCk_Handle_Probe& InProbe,
        ECk_EnableDisable InEnableDisable)
    -> FCk_Handle_Probe
{
    switch(InEnableDisable)
    {
        case ECk_EnableDisable::Enable:
        {
            InProbe.AddOrGet<ck::FTag_Probe_DebugDraw>();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InProbe.Try_Remove<ck::FTag_Probe_DebugDraw>();
            break;
        }
    }

    return InProbe;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Request_BeginOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_BeginOverlap& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_OverlapUpdated(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_OverlapUpdated& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EndOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EndOverlap& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });
;
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EnableDisable(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EnableDisable& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings)
    -> TArray<FCk_Probe_RayCast_Result>
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDebugDraw = true;
    return Request_MultiLineTrace(InAnyHandle, InSettings, FireOverlaps, TryDebugDraw, *PhysicsSystem);
}

auto
    UCk_Utils_Probe_UE::
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings)
    -> FCk_Probe_RayCast_Result
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDrawDebug = true;

    const auto& Result = Request_SingleLineTrace(InAnyHandle, InSettings, FireOverlaps, TryDrawDebug, *PhysicsSystem);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    Request_DrawLineTrace(InAnyHandle, InSettings, *Result);
    return *Result;
}

auto
    UCk_Utils_Probe_UE::
    Request_LineTrace_Persistent(
        const FCk_Probe_RayCastPersistent_Settings& InSettings)
    -> FCk_Handle_ProbeTrace
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSettings.Get_StartPos()),
        TEXT("Cannot create Persistent Probe Line Trace because the Start Position Entity is INVALID!"))
    { return {}; }

    auto ProbeTrace = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InSettings.Get_StartPos());
    ProbeTrace.Add<ck::FFragment_Probe_Request_RayCast>(InSettings);

    return ck::StaticCast<FCk_Handle_ProbeTrace>(ProbeTrace);
}

auto
    UCk_Utils_Probe_UE::
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings)
    -> TArray<FCk_ShapeCast_Result>
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start shape trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDebugDraw = true;
    return Request_MultiShapeTrace(InAnyHandle, InSettings, FireOverlaps, TryDebugDraw, *PhysicsSystem);
}

auto
    UCk_Utils_Probe_UE::
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings)
    -> FCk_ShapeCast_Result
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start shape trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDrawDebug = true;

    const auto& Result = Request_SingleShapeTrace(InAnyHandle, InSettings, FireOverlaps, TryDrawDebug, *PhysicsSystem);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    Request_DrawShapeTrace(InAnyHandle, InSettings, *Result);
    return *Result;
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
    { return InProbeEntity; }

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
    { return InProbeEntity; }

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
    { return InProbeEntity; }

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
    { return InProbeEntity; }

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
    { return InProbeEntity; }

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
    { return InProbeEntity; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    BindTo_OnBeginOverlap_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceBeginOverlap, InProbeTraceEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnBeginOverlap_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceBeginOverlap, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnOverlapUpdated_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceOverlapUpdated, InProbeTraceEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnOverlapUpdated_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceOverlapUpdated, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnEndOverlap_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceEndOverlap, InProbeTraceEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEndOverlap_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceEndOverlap, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

// --------------------------------------------------------------------------------------------------------------------

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
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TArray<FCk_Probe_RayCast_Result>
{
    const auto ConvEnum = [](const ECk_BackFaceMode InBackFaceMode)
    {
        switch (InBackFaceMode)
        {
            case ECk_BackFaceMode::IgnoreBackFaces:
                return JPH::EBackFaceMode::IgnoreBackFaces;
            case ECk_BackFaceMode::CollideWithBackFaces:
                return JPH::EBackFaceMode::CollideWithBackFaces;
            default:
                return JPH::EBackFaceMode::IgnoreBackFaces;
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    const auto& BodyInterface = InPhysicsSystem.GetBodyInterface();

    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& RayCast = JPH::RRayCast{JPH::RayCast{ck::jolt::Conv(StartPos), ck::jolt::Conv(EndPos - StartPos)}};

    const auto& RayCastSettings = JPH::RayCastSettings
    {
        ConvEnum(InSettings.Get_BackFaceModeTriangles()),
        ConvEnum(InSettings.Get_BackFaceModeConvex())
    };
    auto Collector = ck::details::CastRayCollector{InAnyHandle, &BodyInterface};

    InPhysicsSystem.GetNarrowPhaseQuery().CastRay(RayCast, RayCastSettings, Collector);

    auto Result = TArray<FCk_Probe_RayCast_Result>{};
    for (const auto& [Fst, Snd] : Collector.Get_Hits())
    {
        const auto HitLocation = StartPos + Snd * (EndPos - StartPos);

        Result.Emplace(FCk_Probe_RayCast_Result
        {
            Fst,
            HitLocation,
            StartPos - HitLocation,
            StartPos,
            EndPos
        });
    }

    if (InSettings.Get_Filter().IsEmpty())
    { return Result; }

    // --------------------------------------------------------------------------------------------------------------------

    auto FilteredResult = decltype(Result){};

    if (Result.IsEmpty())
    {
        if (InTryDrawDebug)
        { Request_DrawLineTrace(InAnyHandle, InSettings, {}); }
    }

    for (const auto& Hit : Result)
    {
        if (const auto ProbeName = Get_Name(Hit.Get_Probe());
            NOT InSettings.Get_Filter().HasTag(ProbeName))
        { continue; }

        if (InTryDrawDebug)
        { Request_DrawLineTrace(InAnyHandle, InSettings, Hit); }

        FilteredResult.Emplace(Hit);
    }

    if (InFireOverlaps)
    {
        for (const auto& Hit : FilteredResult)
        {
            auto Probe = Hit.Get_Probe();
            Request_BeginOverlap(Probe,
                FCk_Request_Probe_BeginOverlap{InAnyHandle, TArray<FVector>{Hit.Get_HitLocation()}, Hit.Get_NormalDirLen(), nullptr});

            Request_EndOverlap(Probe, FCk_Request_Probe_EndOverlap{InAnyHandle});
        }
    }

    return FilteredResult;
}

auto
    UCk_Utils_Probe_UE::
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TOptional<FCk_Probe_RayCast_Result>
{
    const auto& Result = Request_MultiLineTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InPhysicsSystem);

    if (Result.IsEmpty())
    {
        Request_DrawLineTrace(InAnyHandle, InSettings, {});
        return {};
    }

    Request_DrawLineTrace(InAnyHandle, InSettings, Result[0]);
    return Result[0];
}

auto
    UCk_Utils_Probe_UE::
    Request_DrawLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        TOptional<FCk_Probe_RayCast_Result> InResult)
    -> void
{
    if (NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllLineTraces())
    { return; }

    // Determine if we're on client or server
    const auto IsServer = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAnyHandle);
    const auto IsClient = UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InAnyHandle);

    // Check if we should draw for this net mode
    const auto ShouldDrawServer = IsServer && UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewServerLineTraces();
    const auto ShouldDrawClient = IsClient && UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewClientLineTraces();

    if (NOT (ShouldDrawServer || ShouldDrawClient))
    { return; }

    // Get debug settings
    const auto LineThickness = UCk_Utils_SpatialQuery_Settings::Get_ProbeLineTraceDebugThickness();
    const auto Duration = UCk_Utils_SpatialQuery_Settings::Get_ProbeLineTraceDebugDuration();

    // Choose colors based on client/server
    auto HitColor = FLinearColor::Green;
    auto MissColor = FLinearColor::Red;
    auto NoHitColor = FLinearColor::White;
    auto BoxColor = FLinearColor::Yellow;

    if (IsClient)
    {
        HitColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);   // Cyan (R=0, G=1, B=1)
        MissColor = FLinearColor(1.0f, 0.0f, 1.0f, 1.0f);  // Magenta (R=1, G=0, B=1)
        NoHitColor = FLinearColor::White;  // Keep white for no-hit traces
        BoxColor = FLinearColor::Yellow;   // Keep yellow for hit boxes
    }

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);

    if (ck::IsValid(InResult))
    {
        // Hit case: draw hit portion in hit color, miss portion in miss color
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InResult->Get_StartPos(),
            InResult->Get_HitLocation(), HitColor, Duration, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InResult->Get_HitLocation(), InResult->Get_EndPos(),
            MissColor, Duration, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugBox(WorldContext, InResult->Get_HitLocation(), FVector{1.0},
            BoxColor,
            UKismetMathLibrary::FindLookAtRotation(InResult->Get_HitLocation(), InResult->Get_StartPos()), Duration, LineThickness);
    }
    else
    {
        // No hit case: draw entire trace in no-hit color
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InSettings.Get_StartPos(), InSettings.Get_EndPos(),
            NoHitColor, Duration, LineThickness);
    }
}

auto
    UCk_Utils_Probe_UE::
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TArray<FCk_ShapeCast_Result>
{
    const auto ConvEnum = [](const ECk_BackFaceMode InBackFaceMode)
    {
        switch (InBackFaceMode)
        {
            case ECk_BackFaceMode::IgnoreBackFaces:
                return JPH::EBackFaceMode::IgnoreBackFaces;
            case ECk_BackFaceMode::CollideWithBackFaces:
                return JPH::EBackFaceMode::CollideWithBackFaces;
            default:
                return JPH::EBackFaceMode::IgnoreBackFaces;
        }
    };

    // Create the shape based on trace type
    JPH::Ref<JPH::Shape> JoltShape;

    switch (const auto Shape = InSettings.Get_Shape();
            Shape.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            const auto& Dimensions = Shape.Get_Box();
            const auto Settings = JPH::BoxShapeSettings{ck::jolt::Conv(Dimensions.Get_HalfExtents()), Dimensions.Get_ConvexRadius()};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            JoltShape = ShapeResult.Get();
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            const auto& Dimensions = Shape.Get_Sphere();
            const auto Settings = JPH::SphereShapeSettings{Dimensions.Get_Radius()};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            JoltShape = ShapeResult.Get();
            break;
        }
        case ECk_Shape_Type::Capsule:
        {
            const auto& Dimensions = Shape.Get_Capsule();
            const auto Settings = JPH::CapsuleShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius()};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            JoltShape = ShapeResult.Get();
            break;
        }
        case ECk_Shape_Type::Cylinder:
        {
            const auto& Dimensions = Shape.Get_Capsule();
            const auto Settings = JPH::CylinderShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius()};
            Settings.SetEmbedded();

            auto ShapeResult = Settings.Create();
            JoltShape = ShapeResult.Get();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(Shape.Get_ShapeType());
            return {};
        }
    }

    if (!JoltShape)
    {
        CK_TRIGGER_ENSURE(TEXT("Failed to create shape for trace"));
        return {};
    }

    const auto& BodyInterface = InPhysicsSystem.GetBodyInterface();
    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& Orientation = UKismetMathLibrary::FindLookAtRotation(StartPos, EndPos);

    // Create transform for the start position
    const auto StartTransform = FTransform{Orientation, StartPos};
    const auto Direction = EndPos - StartPos;

    // Create the shape cast
    const auto ShapeCast = JPH::RShapeCast{
        JoltShape,
        JPH::Vec3::sReplicate(1.0f),
        ck::jolt::Conv(StartTransform),
        ck::jolt::Conv(Direction)
    };

    auto ShapeCastSettings = JPH::ShapeCastSettings{};
    ShapeCastSettings.mBackFaceModeTriangles = ConvEnum(InSettings.Get_BackFaceModeTriangles());
    ShapeCastSettings.mBackFaceModeConvex = ConvEnum(InSettings.Get_BackFaceModeConvex());
    // TODO: There are more settings that could be exposed/set here

    auto Collector = ck::details::CastShapeCollector{InAnyHandle, &BodyInterface};
    InPhysicsSystem.GetNarrowPhaseQuery().CastShape(ShapeCast, ShapeCastSettings, JPH::Vec3::sReplicate(0.0f), Collector);

    auto Result = TArray<FCk_ShapeCast_Result>{};
    for (const auto& [Probe, Fraction] : Collector.Get_Hits())
    {
        const auto HitLocation = StartPos + Fraction * Direction;

        Result.Emplace(FCk_ShapeCast_Result
        {
            Probe,
            HitLocation,
            StartPos - HitLocation,
            StartPos,
            EndPos,
            Fraction
        });
    }

    if (InSettings.Get_Filter().IsEmpty())
    {
        if (InTryDrawDebug && Result.IsEmpty())
        { Request_DrawShapeTrace(InAnyHandle, InSettings, {}); }
        return Result;
    }

    // Filter results
    auto FilteredResult = decltype(Result){};

    for (const auto& Hit : Result)
    {
        if (const auto ProbeName = Get_Name(Hit.Get_Probe());
            NOT InSettings.Get_Filter().HasTag(ProbeName))
        { continue; }

        if (InTryDrawDebug)
        { Request_DrawShapeTrace(InAnyHandle, InSettings, Hit); }

        FilteredResult.Emplace(Hit);
    }

    if (InTryDrawDebug && FilteredResult.IsEmpty())
    { Request_DrawShapeTrace(InAnyHandle, InSettings, {}); }

    if (InFireOverlaps)
    {
        for (const auto& Hit : FilteredResult)
        {
            auto Probe = Hit.Get_Probe();
            Request_BeginOverlap(Probe,
                FCk_Request_Probe_BeginOverlap{InAnyHandle, TArray<FVector>{Hit.Get_HitLocation()}, Hit.Get_NormalDirLen(), nullptr});

            Request_EndOverlap(Probe, FCk_Request_Probe_EndOverlap{InAnyHandle});
        }
    }

    return FilteredResult;
}

auto
    UCk_Utils_Probe_UE::
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TOptional<FCk_ShapeCast_Result>
{
    const auto& Result = Request_MultiShapeTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InPhysicsSystem);

    if (Result.IsEmpty())
    {
        Request_DrawShapeTrace(InAnyHandle, InSettings, {});
        return {};
    }

    Request_DrawShapeTrace(InAnyHandle, InSettings, Result[0]);
    return Result[0];
}

auto
    UCk_Utils_Probe_UE::
    Request_DrawShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        TOptional<FCk_ShapeCast_Result> InResult)
    -> void
{
    if (NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllLineTraces())
    { return; }

    constexpr auto LineThickness = 0.5f;

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);
    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& Orientation = UKismetMathLibrary::FindLookAtRotation(StartPos, EndPos);
    const auto& Shape = InSettings.Get_Shape();

    if (ck::IsValid(InResult))
    {
        const auto& HitLocation = InResult->Get_HitLocation();

        // Draw shapes at start, hit, and end positions
        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Red, 0, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, HitLocation, Orientation,
            FLinearColor::Yellow, 0, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Green, 0, LineThickness);

        // Draw connectors from start to hit and hit to end
        DrawShapeConnector(WorldContext, StartPos, HitLocation, Shape,
            FLinearColor::Red, 0, LineThickness);
        DrawShapeConnector(WorldContext, HitLocation, EndPos, Shape,
            FLinearColor::Green, 0, LineThickness);
    }
    else
    {
        // No hit case: draw shapes at start and end positions
        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Red, 0, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Green, 0, LineThickness);

        // Draw connector from start to end
        DrawShapeConnector(WorldContext, StartPos, EndPos, Shape,
            FLinearColor::White, 0, LineThickness);
    }
}

auto
    UCk_Utils_Probe_UE::
    DrawShapeAtLocation(
        UWorld* InWorld,
        const FCk_ShapeCast_Settings& InSettings,
        const FVector& InLocation,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InDuration,
        float InThickness)
    -> void
{
    switch (const auto Shape = InSettings.Get_Shape();
            Shape.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            const auto& BoxParams = Shape.Get_Box();
            UCk_Utils_DebugDraw_UE::DrawDebugBox(InWorld, {}, {}, InLocation, BoxParams.Get_HalfExtents(),
                InColor, InRotation, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            const auto& SphereParams = Shape.Get_Sphere();
            UCk_Utils_DebugDraw_UE::DrawDebugSphere(InWorld, {}, {}, InLocation, SphereParams.Get_Radius(),
                12, InColor, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Capsule:
        {
            const auto& CapsuleParams = Shape.Get_Capsule();
            UCk_Utils_DebugDraw_UE::DrawDebugCapsule(InWorld, {}, {}, InLocation, CapsuleParams.Get_HalfHeight(),
                CapsuleParams.Get_Radius(), InRotation, InColor, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Cylinder:
        {
            const auto& CylinderParams = Shape.Get_Cylinder();
            // Transform for the cylinder orientation
            const FTransform Transform{InRotation, InLocation};

            // Calculate start and end points for the cylinder
            const FVector CylinderStart = Transform.TransformPosition(FVector(0, 0, -CylinderParams.Get_HalfHeight()));
            const FVector CylinderEnd = Transform.TransformPosition(FVector(0, 0, CylinderParams.Get_HalfHeight()));

            constexpr int32 NumSegments = 12;
            UCk_Utils_DebugDraw_UE::DrawDebugCylinder(InWorld, {}, {}, CylinderStart, CylinderEnd,
                CylinderParams.Get_Radius(), NumSegments, InColor, InDuration, InThickness);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(Shape.Get_ShapeType());
            break;
        };
    }
}

auto
    UCk_Utils_Probe_UE::
    DrawShapeConnector(
        UWorld* InWorld,
        const FVector& InStartPos,
        const FVector& InEndPos,
        const FCk_AnyShape& InShape,
        const FLinearColor& InColor,
        float InDuration,
        float InThickness)
    -> void
{
    const FVector Direction = InEndPos - InStartPos;
    const float Distance = Direction.Size();

    if (Distance < KINDA_SMALL_NUMBER)
    { return; }

    const FVector MidPoint = (InStartPos + InEndPos) * 0.5f;
    const FRotator ConnectorOrientation = UKismetMathLibrary::FindLookAtRotation(InStartPos, InEndPos);

    switch (InShape.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            // Box connector: Box spanning the distance with original cross-section
            const auto& BoxParams = InShape.Get_Box();
            const FVector ConnectorHalfExtents = FVector{Distance * 0.5f, BoxParams.Get_HalfExtents().Y, BoxParams.Get_HalfExtents().Z};

            UCk_Utils_DebugDraw_UE::DrawDebugBox(InWorld, {}, {}, MidPoint, ConnectorHalfExtents,
                InColor, ConnectorOrientation, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            // Sphere connector: Capsule with sphere's radius and distance as height
            const auto& SphereParams = InShape.Get_Sphere();
            const float Radius = SphereParams.Get_Radius();
            const float HalfHeight = (Distance * 0.5f) + Radius;

            const auto CapsuleOrientation = ConnectorOrientation + FRotator(90, 0, 0);

            UCk_Utils_DebugDraw_UE::DrawDebugCapsule(InWorld, {}, {}, MidPoint, HalfHeight, Radius,
                CapsuleOrientation, InColor, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Capsule:
        case ECk_Shape_Type::Cylinder:
        {
            // Capsule & Cylinder connector: Simple line
            UCk_Utils_DebugDraw_UE::DrawDebugLine(InWorld, {}, {}, InStartPos, InEndPos,
                InColor, InDuration, InThickness);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InShape.Get_ShapeType());
            break;
        }
    }
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