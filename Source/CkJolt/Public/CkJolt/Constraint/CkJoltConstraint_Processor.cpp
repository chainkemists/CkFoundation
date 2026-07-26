#include "CkJoltConstraint_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SpringSettings.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltConstraint_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltConstraint_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltConstraint_LivenessReap);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltConstraint_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_constraint_processor
{
    auto
        TryResolve_JoltWorld(
            const FCk_Handle& InTransientEntity)
        -> ck::FJoltWorld*
    {
        const auto* WorldPtr = InTransientEntity.Get_RegistryView().TryGetContext<TSharedPtr<ck::FJoltWorld>>();
        if (WorldPtr == nullptr || NOT WorldPtr->IsValid())
        { return nullptr; }

        return WorldPtr->Get();
    }

    auto
        TryResolve_PhysicsSystem(
            const FCk_Handle& InTransientEntity)
        -> TWeakPtr<JPH::PhysicsSystem>
    {
        const auto* Ctx = InTransientEntity.Get_RegistryView().TryGetContext<TWeakPtr<JPH::PhysicsSystem>>();
        if (Ctx == nullptr)
        { return {}; }

        return *Ctx;
    }

    auto
        Conv(
            ECk_JoltConstraint_SpringMode InMode)
        -> JPH::ESpringMode
    {
        switch (InMode)
        {
            case ECk_JoltConstraint_SpringMode::FrequencyAndDamping: return JPH::ESpringMode::FrequencyAndDamping;
            case ECk_JoltConstraint_SpringMode::StiffnessAndDamping: return JPH::ESpringMode::StiffnessAndDamping;
            default:
                CK_INVALID_ENUM(InMode);
                return JPH::ESpringMode::FrequencyAndDamping;
        }
    }

    // NOT triggered by the world-anchor sentinel — callers exclude that case first.
    auto
        Get_IsBodyEntityGone(
            const FCk_Handle& InBodyEntity)
        -> bool
    {
        return ck::Is_NOT_Valid(InBodyEntity) ||
               UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InBodyEntity, ECk_EntityLifetime_DestructionPhase::BeginDestroy);
    }

    enum class EResolveOutcome : uint8 { Ready, Retry, Failed };

    struct FResolvedBody
    {
        EResolveOutcome _Outcome = EResolveOutcome::Failed;
        JPH::BodyID _BodyId;
    };

    auto
        Resolve_BodyId(
            const FCk_Handle& InConstraintEntity,
            FCk_Handle InBodyEntity,
            const TCHAR* InWhich)
        -> FResolvedBody
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InBodyEntity),
            TEXT("JoltConstraint on Entity [{}]: body [{}] entity is INVALID — the constraint will never be created."),
            InConstraintEntity, InWhich)
        { return {}; }

        CK_ENSURE_IF_NOT(InBodyEntity.Has<ck::FFragment_JoltBody_Current>(),
            TEXT("JoltConstraint on Entity [{}]: body [{}] entity [{}] has NO JoltBody feature — the constraint will never be created."),
            InConstraintEntity, InWhich, InBodyEntity)
        { return {}; }

        const auto& BodyCurrent = InBodyEntity.Get<ck::FFragment_JoltBody_Current>();
        if (NOT BodyCurrent.Get_BodyAdded())
        { return FResolvedBody{EResolveOutcome::Retry, {}}; }

        return FResolvedBody{EResolveOutcome::Ready, BodyCurrent.Get_BodyId()};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_JoltConstraint_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _PhysicsSystem = ck_jolt_constraint_processor::TryResolve_PhysicsSystem(_TransientEntity);
        if (ck::Is_NOT_Valid(_PhysicsSystem.Pin()))
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltConstraint_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            FFragment_JoltConstraint_Current& InCurrent)
        -> void
    {
        using namespace JPH;
        using namespace ck_jolt_constraint_processor;

        InHandle.Remove<MarkedDirtyBy>();

        const auto ResolvedA = Resolve_BodyId(InHandle, InCurrent.Get_BodyA(), TEXT("A"));
        if (ResolvedA._Outcome == EResolveOutcome::Failed)
        { return; }

        auto ResolvedB = FResolvedBody{EResolveOutcome::Ready, BodyID{}};
        if (NOT InCurrent.Get_BodyBIsWorldAnchor())
        {
            ResolvedB = Resolve_BodyId(InHandle, InCurrent.Get_BodyB(), TEXT("B"));
            if (ResolvedB._Outcome == EResolveOutcome::Failed)
            { return; }
        }

        if (ResolvedA._Outcome == EResolveOutcome::Retry || ResolvedB._Outcome == EResolveOutcome::Retry)
        {
            // The body exists but its batched AddBodies pass has not run yet — retry next frame.
            InHandle.Add<MarkedDirtyBy>();
            return;
        }

        auto Settings = Ref<TwoBodyConstraintSettings>{};

        switch (InParams.Get_ConstraintType())
        {
            case ECk_JoltConstraint_Type::Distance:
            {
                const auto MinDistance = InParams.Get_MinDistance();
                const auto MaxDistance = InParams.Get_MaxDistance();

                CK_ENSURE_IF_NOT(MinDistance < 0.0f || MaxDistance < 0.0f || MinDistance <= MaxDistance,
                    TEXT("JoltConstraint on Entity [{}]: Distance range [{} .. {}] is inverted — the constraint will never be created."),
                    InHandle, MinDistance, MaxDistance)
                { return; }

                auto* Distance = new DistanceConstraintSettings{};
                Distance->mSpace = EConstraintSpace::WorldSpace;
                Distance->mPoint1 = ck::jolt::Conv(InParams.Get_WorldAnchorA());
                Distance->mPoint2 = ck::jolt::Conv(InParams.Get_WorldAnchorB());
                Distance->mMinDistance = MinDistance;
                Distance->mMaxDistance = MaxDistance;

                if (InParams.Get_UseSpring() == ECk_EnableDisable::Enable)
                {
                    Distance->mLimitsSpringSettings = SpringSettings{
                        Conv(InParams.Get_SpringMode()),
                        InParams.Get_SpringFrequencyOrStiffness(),
                        InParams.Get_SpringDamping()};
                }

                Settings = Distance;
                break;
            }
            case ECk_JoltConstraint_Type::Point:
            {
                auto* Point = new PointConstraintSettings{};
                Point->mSpace = EConstraintSpace::WorldSpace;
                Point->mPoint1 = ck::jolt::Conv(InParams.Get_WorldAnchorA());
                Point->mPoint2 = ck::jolt::Conv(InParams.Get_WorldAnchorB());

                Settings = Point;
                break;
            }
            case ECk_JoltConstraint_Type::Hinge:
            {
                CK_ENSURE_IF_NOT(NOT InParams.Get_HingeAxis().IsNearlyZero(),
                    TEXT("JoltConstraint on Entity [{}]: Hinge axis is (near) zero — the constraint will never be created."),
                    InHandle)
                { return; }

                const auto HingeAxis = ck::jolt::Conv(InParams.Get_HingeAxis()).Normalized();
                const auto NormalAxis = HingeAxis.GetNormalizedPerpendicular();
                const auto Pivot = ck::jolt::Conv(InParams.Get_WorldAnchorA());

                auto* Hinge = new HingeConstraintSettings{};
                Hinge->mSpace = EConstraintSpace::WorldSpace;
                Hinge->mPoint1 = Pivot;
                Hinge->mPoint2 = Pivot;
                Hinge->mHingeAxis1 = HingeAxis;
                Hinge->mHingeAxis2 = HingeAxis;
                Hinge->mNormalAxis1 = NormalAxis;
                Hinge->mNormalAxis2 = NormalAxis;
                Hinge->mLimitsMin = FMath::DegreesToRadians(FMath::Clamp(InParams.Get_LimitsMinDegrees(), -180.0f, 0.0f));
                Hinge->mLimitsMax = FMath::DegreesToRadians(FMath::Clamp(InParams.Get_LimitsMaxDegrees(), 0.0f, 180.0f));
                Hinge->mMaxFrictionTorque = InParams.Get_MaxFrictionTorque();

                Settings = Hinge;
                break;
            }
        }

        CK_ENSURE_IF_NOT(Settings.GetPtr() != nullptr,
            TEXT("JoltConstraint on Entity [{}]: no settings built for type [{}]."),
            InHandle, InParams.Get_ConstraintType())
        { return; }

        // Jolt asserts body1 != body2; identical bodies were rejected at Create.
        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        auto* Constraint = BodyInterface.CreateConstraint(Settings.GetPtr(), ResolvedA._BodyId, ResolvedB._BodyId);

        CK_ENSURE_IF_NOT(Constraint != nullptr,
            TEXT("JoltConstraint on Entity [{}]: CreateConstraint FAILED."), InHandle)
        { return; }

        PhysicsSystem->AddConstraint(Constraint);

        // Wake the bodies or a sleeping one hangs mid-air until something else wakes it.
        BodyInterface.ActivateConstraint(Constraint);

        InCurrent._Constraint = Constraint;
        InCurrent._ConstraintAdded = true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltConstraint_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _PhysicsSystem = ck_jolt_constraint_processor::TryResolve_PhysicsSystem(_TransientEntity);
        if (ck::Is_NOT_Valid(_PhysicsSystem.Pin()))
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltConstraint_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            FFragment_JoltConstraint_Current& InCurrent,
            FFragment_JoltConstraint_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_JoltConstraint_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            const FFragment_JoltConstraint_Current& InCurrent,
            const FCk_Request_JoltConstraint_SetEnabled& InRequest) const
        -> void
    {
        if (InCurrent._Constraint.GetPtr() == nullptr)
        { return; }

        InCurrent._Constraint->SetEnabled(InRequest.Get_Enabled() == ECk_EnableDisable::Enable);

        // Re-enabling must wake the bodies or the constraint stays visually inert until something else does.
        if (InRequest.Get_Enabled() == ECk_EnableDisable::Enable)
        {
            const auto PhysicsSystem = _PhysicsSystem.Pin();
            if (ck::IsValid(PhysicsSystem))
            { PhysicsSystem->GetBodyInterface().ActivateConstraint(InCurrent._Constraint.GetPtr()); }
        }
    }

    auto
        FProcessor_JoltConstraint_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            const FFragment_JoltConstraint_Current& InCurrent,
            const FCk_Request_JoltConstraint_Distance_SetRange& InRequest) const
        -> void
    {
        if (InCurrent._Constraint.GetPtr() == nullptr)
        { return; }

        CK_ENSURE_IF_NOT(InParams.Get_ConstraintType() == ECk_JoltConstraint_Type::Distance,
            TEXT("Distance_SetRange on JoltConstraint [{}] of type [{}] — request ignored."),
            InHandle, InParams.Get_ConstraintType())
        { return; }

        CK_ENSURE_IF_NOT(InRequest.Get_MinDistance() <= InRequest.Get_MaxDistance(),
            TEXT("Distance_SetRange on JoltConstraint [{}]: range [{} .. {}] is inverted — request ignored."),
            InHandle, InRequest.Get_MinDistance(), InRequest.Get_MaxDistance())
        { return; }

        auto* Distance = static_cast<JPH::DistanceConstraint*>(InCurrent._Constraint.GetPtr());
        Distance->SetDistance(InRequest.Get_MinDistance(), InRequest.Get_MaxDistance());

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::IsValid(PhysicsSystem))
        { PhysicsSystem->GetBodyInterface().ActivateConstraint(InCurrent._Constraint.GetPtr()); }
    }

    auto
        FProcessor_JoltConstraint_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            const FFragment_JoltConstraint_Current& InCurrent,
            const FCk_Request_JoltConstraint_Hinge_SetMotor& InRequest) const
        -> void
    {
        if (InCurrent._Constraint.GetPtr() == nullptr)
        { return; }

        CK_ENSURE_IF_NOT(InParams.Get_ConstraintType() == ECk_JoltConstraint_Type::Hinge,
            TEXT("Hinge_SetMotor on JoltConstraint [{}] of type [{}] — request ignored."),
            InHandle, InParams.Get_ConstraintType())
        { return; }

        auto* Hinge = static_cast<JPH::HingeConstraint*>(InCurrent._Constraint.GetPtr());

        switch (InRequest.Get_MotorState())
        {
            case ECk_JoltConstraint_MotorState::Off:
            {
                Hinge->SetMotorState(JPH::EMotorState::Off);
                break;
            }
            case ECk_JoltConstraint_MotorState::Velocity:
            {
                Hinge->SetTargetAngularVelocity(FMath::DegreesToRadians(InRequest.Get_TargetAngularVelocityDegS()));
                Hinge->SetMotorState(JPH::EMotorState::Velocity);
                break;
            }
            case ECk_JoltConstraint_MotorState::Position:
            {
                Hinge->SetTargetAngle(FMath::DegreesToRadians(InRequest.Get_TargetAngleDegrees()));
                Hinge->SetMotorState(JPH::EMotorState::Position);
                break;
            }
        }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::IsValid(PhysicsSystem))
        { PhysicsSystem->GetBodyInterface().ActivateConstraint(InCurrent._Constraint.GetPtr()); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltConstraint_LivenessReap::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _PhysicsSystem = ck_jolt_constraint_processor::TryResolve_PhysicsSystem(_TransientEntity);
        _JoltWorld = ck_jolt_constraint_processor::TryResolve_JoltWorld(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltConstraint_LivenessReap::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltConstraint_Current& InCurrent) const
        -> void
    {
        using namespace ck_jolt_constraint_processor;

        if (InCurrent._Constraint.GetPtr() == nullptr)
        { return; }

        const auto BodyAGone = Get_IsBodyEntityGone(InCurrent.Get_BodyA());
        const auto BodyBGone = NOT InCurrent.Get_BodyBIsWorldAnchor() && Get_IsBodyEntityGone(InCurrent.Get_BodyB());

        if (NOT (BodyAGone || BodyBGone))
        { return; }

        // ASYNC GUARD: mirrors FProcessor_JoltBody_EndPlay — RemoveConstraint must not race an in-flight step.
        if (_JoltWorld != nullptr && _JoltWorld->Get_AsyncFuture().IsValid())
        { _JoltWorld->WaitForAsyncStep(); }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (ck::IsValid(PhysicsSystem) && InCurrent.Get_ConstraintAdded())
        { PhysicsSystem->RemoveConstraint(InCurrent._Constraint.GetPtr()); }

        InCurrent._Constraint = nullptr;
        InCurrent._ConstraintAdded = false;

        // The constraint entity is inert now — let it die properly next frame (its EndPlay will no-op).
        auto DestroyHandle = static_cast<FCk_Handle>(InHandle);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(DestroyHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_JoltConstraint_EndPlay::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _PhysicsSystem = ck_jolt_constraint_processor::TryResolve_PhysicsSystem(_TransientEntity);
        _JoltWorld = ck_jolt_constraint_processor::TryResolve_JoltWorld(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltConstraint_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltConstraint_Current& InCurrent) const
        -> void
    {
        if (InCurrent._Constraint.GetPtr() == nullptr)
        { return; }

        // ASYNC GUARD: mirrors FProcessor_JoltBody_EndPlay.
        if (_JoltWorld != nullptr && _JoltWorld->Get_AsyncFuture().IsValid())
        { _JoltWorld->WaitForAsyncStep(); }

        const auto PhysicsSystem = _PhysicsSystem.Pin();

        CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
            TEXT("PhysicsSystem is INVALID during JoltConstraint [{}] teardown — subsystem already deinitialized?"), InHandle)
        {
            InCurrent._Constraint = nullptr;
            return;
        }

        if (InCurrent.Get_ConstraintAdded())
        { PhysicsSystem->RemoveConstraint(InCurrent._Constraint.GetPtr()); }

        InCurrent._Constraint = nullptr;
        InCurrent._ConstraintAdded = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
