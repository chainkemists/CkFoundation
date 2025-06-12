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

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

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
    {
        return {};
    }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ProbeName()),
        TEXT("Cannot Add a Probe to Entity [{}] because it has INVALID Name"), InHandle)
    {
        return {};
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
    {

        return {};
    }

    Request_DrawTrace(InAnyHandle, InSettings, *Result);
    return *Result;
}

auto
    UCk_Utils_Probe_UE::
    Request_LineTrace_Persistent(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCastPersistent_Settings& InSettings)
    -> FCk_Handle_ProbeTrace
{
    auto ProbeTrace = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAnyHandle);
    ProbeTrace.Add<ck::FFragment_Probe_Request_RayCast>(InSettings);

    return ck::StaticCast<FCk_Handle_ProbeTrace>(ProbeTrace);
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

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --

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

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --

    auto FilteredResult = decltype(Result){};

    if (Result.IsEmpty())
    {
        if (InTryDrawDebug)
        { Request_DrawTrace(InAnyHandle, InSettings, {}); }
    }

    for (const auto& Hit : Result)
    {
        const auto ProbeName = Get_Name(Hit.Get_Probe());
        if (NOT InSettings.Get_Filter().HasTag(ProbeName))
        { continue; }

        if (InTryDrawDebug)
        { Request_DrawTrace(InAnyHandle, InSettings, Hit); }

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
        FCk_Handle InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TOptional<FCk_Probe_RayCast_Result>
{
    const auto& Result = Request_MultiLineTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InPhysicsSystem);

    if (Result.IsEmpty())
    {
        Request_DrawTrace(InAnyHandle, InSettings, {});
        return {};
    }

    Request_DrawTrace(InAnyHandle, InSettings, Result[0]);
    return Result[0];
}

auto
    UCk_Utils_Probe_UE::
    Request_DrawTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        TOptional<FCk_Probe_RayCast_Result> InResult)
    -> void
{
    if (NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllLineTraces())
    { return; }

    constexpr auto LineThickness = 0.5;

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);

    if (ck::IsValid(InResult))
    {
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, {}, {}, InResult->Get_StartPos(),
            InResult->Get_HitLocation(), FLinearColor::Green, 0, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, {}, {}, InResult->Get_HitLocation(), InResult->Get_EndPos(),
            FLinearColor::Red, 0, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugBox(WorldContext, {}, {}, InResult->Get_HitLocation(), FVector{1.0},
            FLinearColor::Yellow,
            UKismetMathLibrary::FindLookAtRotation(InResult->Get_HitLocation(), InResult->Get_StartPos()), 0, LineThickness);
    }
    else
    {
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, {}, {}, InSettings.Get_StartPos(), InSettings.Get_EndPos(),
            FLinearColor::White, 0, LineThickness);
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
