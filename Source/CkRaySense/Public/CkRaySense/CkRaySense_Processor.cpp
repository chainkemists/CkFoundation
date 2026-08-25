#include "CkRaySense_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"

#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkRaySense/CkRaySense_Log.h"
#include "CkRaySense/CkRaySense_Stats.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRaySense/CkRaySense_Utils.h"

#include <Kismet/KismetSystemLibrary.h>

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_LineTrace_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_BoxSweep_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_SphereSweep_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_CapsuleSweep_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_CylinderSweep_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_RaySense_CancelPendingRequests);

namespace ck_raysense
{
    namespace cvar
    {
        static auto DebugDrawAllTraces = false;
        static auto CVar_DebugDrawAllTraces = FAutoConsoleVariableRef(TEXT("ck.RaySense.DebugDrawAllTraces"),
            DebugDrawAllTraces,
            TEXT("Draw the debug information of all RaySense traces performed"));

        static auto DebugDrawTraceDuration = static_cast<float>(FCk_Time::HundredMilliseconds().Get_Seconds());
        static auto CVar_DebugDrawTraceDuration = FAutoConsoleVariableRef(TEXT("ck.RaySense.DebugDrawTraceDuration"),
            DebugDrawTraceDuration,
            TEXT("How long should RaySense trace debug draw last"));
    }

    auto
        ShouldDrawTraces()
        -> bool
    {
        return cvar::DebugDrawAllTraces && NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
    }
}

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("RaySense::LineTrace"), STAT_RaySense_LineTrace, STATGROUP_CkRaySense);
DECLARE_CYCLE_STAT(TEXT("RaySense::BoxSweep"), STAT_RaySense_BoxSweep, STATGROUP_CkRaySense);
DECLARE_CYCLE_STAT(TEXT("RaySense::SphereSweep"), STAT_RaySense_SphereSweep, STATGROUP_CkRaySense);
DECLARE_CYCLE_STAT(TEXT("RaySense::CapsuleSweep"), STAT_RaySense_CapsuleSweep, STATGROUP_CkRaySense);
DECLARE_CYCLE_STAT(TEXT("RaySense::DiscreteOverlap"), STAT_RaySense_DiscreteOverlap, STATGROUP_CkRaySense);
DECLARE_DWORD_COUNTER_STAT(TEXT("RaySense Traces Issued"), STAT_RaySense_TracesIssued, STATGROUP_CkRaySense);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_raysense
{
    // The engine trace helpers take raw actor arrays; the fragment stores weak refs. Stale entries
    // resolve to null, which the engine ignore-list handling tolerates. Ignore lists are typically
    // 0-2 entries, so the per-trace copy is negligible.
    auto Get_ResolvedActorsToIgnore(
        const ck::FFragment_RaySense_Params& InParams) -> TArray<AActor*>
    {
        return ck::algo::Transform<TArray<AActor*>>(
            InParams.Get_DataToIgnore().Get_ActorsToIgnore(),
            [](const TWeakObjectPtr<AActor>& InActor) { return InActor.Get(); });
    }

    template<typename T_Handle>
    auto Request_ProcessTraceHit(
        T_Handle& InHandle,
        const ck::FFragment_RaySense_Params& InParams,
        const FHitResult& InHitResult) -> void
    {
        if (UCk_Utils_RaySense_UE::Get_ShouldIgnoreTraceHit(InHandle, InHitResult))
        { return; }

        auto Result = FCk_RaySense_HitResult{InHitResult.ImpactPoint, InHitResult.ImpactNormal}
        .Set_ImpactPhysMat(InHitResult.PhysMaterial)
        .Set_MaybeHitActor(InHitResult.GetActor())
        .Set_MaybeHitComponent(InHitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(InHitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(InHitResult.GetActor()) : FCk_Handle{});

        switch (InParams.Get_CollisionResponse())
        {
            case ECk_RaySense_CollisionResponse_Policy::Overlap: break;
            case ECk_RaySense_CollisionResponse_Policy::Collide:
            {
                UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocation(InHandle,
                    FCk_Request_Transform_SetLocation{Result.Get_ImpactPoint()}, {});
                break;
            }
        }

        ck::UUtils_Signal_OnRaySenseTraceHit::Broadcast(InHandle, ck::MakePayload(InHandle, Result));
    }

    template<typename T_Handle, typename T_DiscreteOverlapFn, typename T_ContinuousSweepFn>
    auto DoSweepTrace(
        T_Handle& InHandle,
        const ck::FFragment_RaySense_Params& InParams,
        const ck::FFragment_Transform_Previous& InTransform_Prev,
        const ck::FFragment_Transform& InTransform,
        T_DiscreteOverlapFn&& InDiscreteOverlapFn,
        T_ContinuousSweepFn&& InContinuousSweepFn) -> void
    {
        auto World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Could NOT get the World for entity [{}]. RaySense will NOT work"), InHandle)
        { return; }

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        constexpr auto TraceComplex = false;
        constexpr auto IgnoreSelf = true;
        auto HitResult = FHitResult{};

        const auto Hit = [&]() -> bool
        {
            if (InParams.Get_CollisionQuality() == ECk_RaySense_CollisionQuality::Discrete)
            {
                SCOPE_CYCLE_COUNTER(STAT_RaySense_DiscreteOverlap);
                return InDiscreteOverlapFn(World, CurrTransform, InParams);
            }

            return InContinuousSweepFn(
                World,
                PrevTransform,
                CurrTransform,
                InParams,
                TraceComplex,
                IgnoreSelf,
                HitResult);
        }();

        if (NOT Hit)
        { return; }

        // Discrete's OverlapAnyTest does NOT fill HitResult, so a discrete hit broadcasts a zeroed
        // FCk_RaySense_HitResult (and Collide snaps to the origin) — pre-existing, deliberate.
        Request_ProcessTraceHit(InHandle, InParams, HitResult);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RaySense_LineTrace_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RaySense_Params& InParams,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform)
            -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_RaySense_LineTrace);
        INC_DWORD_STAT(STAT_RaySense_TracesIssued);

        auto World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Could NOT get the World for entity [{}]. RaySense will NOT work"), InHandle)
        { return; }

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        constexpr auto TraceComplex = false;
        constexpr auto IgnoreSelf = true;
        auto HitResult = FHitResult{};

        const auto Hit = UKismetSystemLibrary::LineTraceSingle(
            World,
            PrevTransform.GetLocation(),
            CurrTransform.GetLocation(),
            UEngineTypes::ConvertToTraceType(InParams.Get_CollisionChannel()),
            TraceComplex,
            ck_raysense::Get_ResolvedActorsToIgnore(InParams),
            ck_raysense::ShouldDrawTraces() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
            HitResult,
            IgnoreSelf,
            FLinearColor::Red,
            FLinearColor::Green,
            ck_raysense::cvar::DebugDrawTraceDuration);

        if (NOT Hit)
        { return; }

        ck_raysense::Request_ProcessTraceHit(InHandle, InParams, HitResult);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_BoxSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform)
            -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_RaySense_BoxSweep);
        INC_DWORD_STAT(STAT_RaySense_TracesIssued);

        ck_raysense::DoSweepTrace(
            InHandle, InParams, InTransform_Prev, InTransform,
            [&](UWorld* InWorld, const FTransform& InCurrTransform, const FFragment_RaySense_Params& InP) -> bool
            {
                const auto& Shape = FCollisionShape::MakeBox(InShape.Get_Dimensions().Get_HalfExtents());
                return InWorld->OverlapAnyTestByChannel(InCurrTransform.GetLocation(), InCurrTransform.GetRotation(), InP.Get_CollisionChannel(), Shape);
            },
            [&](UWorld* InWorld, const FTransform& InPrevTransform, const FTransform& InCurrTransform,
                const FFragment_RaySense_Params& InP, bool bTraceComplex, bool bIgnoreSelf, FHitResult& OutHitResult) -> bool
            {
                return UKismetSystemLibrary::BoxTraceSingle(
                    InWorld,
                    InPrevTransform.GetLocation(),
                    InCurrTransform.GetLocation(),
                    InShape.Get_Dimensions().Get_HalfExtents(),
                    InCurrTransform.GetRotation().Rotator(),
                    UEngineTypes::ConvertToTraceType(InP.Get_CollisionChannel()),
                    bTraceComplex,
                    ck_raysense::Get_ResolvedActorsToIgnore(InP),
                    ck_raysense::ShouldDrawTraces() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
                    OutHitResult,
                    bIgnoreSelf,
                    FLinearColor::Red,
                    FLinearColor::Green,
                    ck_raysense::cvar::DebugDrawTraceDuration);
            });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_SphereSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform)
            -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_RaySense_SphereSweep);
        INC_DWORD_STAT(STAT_RaySense_TracesIssued);

        ck_raysense::DoSweepTrace(
            InHandle, InParams, InTransform_Prev, InTransform,
            [&](UWorld* InWorld, const FTransform& InCurrTransform, const FFragment_RaySense_Params& InP) -> bool
            {
                const auto& Shape = FCollisionShape::MakeSphere(InShape.Get_Dimensions().Get_Radius());
                return InWorld->OverlapAnyTestByChannel(InCurrTransform.GetLocation(), InCurrTransform.GetRotation(), InP.Get_CollisionChannel(), Shape);
            },
            [&](UWorld* InWorld, const FTransform& InPrevTransform, const FTransform& InCurrTransform,
                const FFragment_RaySense_Params& InP, bool bTraceComplex, bool bIgnoreSelf, FHitResult& OutHitResult) -> bool
            {
                return UKismetSystemLibrary::SphereTraceSingle(
                    InWorld,
                    InPrevTransform.GetLocation(),
                    InCurrTransform.GetLocation(),
                    InShape.Get_Dimensions().Get_Radius(),
                    UEngineTypes::ConvertToTraceType(InP.Get_CollisionChannel()),
                    bTraceComplex,
                    ck_raysense::Get_ResolvedActorsToIgnore(InP),
                    ck_raysense::ShouldDrawTraces() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
                    OutHitResult,
                    bIgnoreSelf,
                    FLinearColor::Red,
                    FLinearColor::Green,
                    ck_raysense::cvar::DebugDrawTraceDuration);
            });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_CapsuleSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform)
            -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_RaySense_CapsuleSweep);
        INC_DWORD_STAT(STAT_RaySense_TracesIssued);

        ck_raysense::DoSweepTrace(
            InHandle, InParams, InTransform_Prev, InTransform,
            [&](UWorld* InWorld, const FTransform& InCurrTransform, const FFragment_RaySense_Params& InP) -> bool
            {
                const auto& Shape = FCollisionShape::MakeCapsule(InShape.Get_Dimensions().Get_Radius(), InShape.Get_Dimensions().Get_HalfHeight());
                return InWorld->OverlapAnyTestByChannel(InCurrTransform.GetLocation(), InCurrTransform.GetRotation(), InP.Get_CollisionChannel(), Shape);
            },
            [&](UWorld* InWorld, const FTransform& InPrevTransform, const FTransform& InCurrTransform,
                const FFragment_RaySense_Params& InP, bool bTraceComplex, bool bIgnoreSelf, FHitResult& OutHitResult) -> bool
            {
                return UKismetSystemLibrary::CapsuleTraceSingle(
                    InWorld,
                    InPrevTransform.GetLocation(),
                    InCurrTransform.GetLocation(),
                    InShape.Get_Dimensions().Get_Radius(),
                    InShape.Get_Dimensions().Get_HalfHeight(),
                    UEngineTypes::ConvertToTraceType(InP.Get_CollisionChannel()),
                    bTraceComplex,
                    ck_raysense::Get_ResolvedActorsToIgnore(InP),
                    ck_raysense::ShouldDrawTraces() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
                    OutHitResult,
                    bIgnoreSelf,
                    FLinearColor::Red,
                    FLinearColor::Green,
                    ck_raysense::cvar::DebugDrawTraceDuration);
            });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_CylinderSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform)
            -> void
    {
        CK_TRIGGER_ENSURE(TEXT("Cylinder shape is NOT supported by Unreal. It is only supported by Jolt. Collisions for "
            "[{}] will NOT work"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RaySense_Params& InParams,
            const FFragment_RaySense_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](
        FFragment_RaySense_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, Visitor([&](
            const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_RaySense_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_RaySense_EnableDisable& InRequest)
        -> void
    {
        switch(InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                InHandle.Try_Remove<FTag_RaySense_Disabled>();
                break;
            }
            case ECk_EnableDisable::Disable:
            {
                InHandle.AddOrGet<FTag_RaySense_Disabled>();
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RaySense_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
