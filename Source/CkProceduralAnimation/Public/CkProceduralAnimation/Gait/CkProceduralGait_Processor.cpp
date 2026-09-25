#include "CkProceduralAnimation/Gait/CkProceduralGait_Processor.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/Query/CkJoltQuery_Utils.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_gait
{
    auto
        Get_TrustedHit(
            const FCk_Jolt_HitResult& InHit)
        -> bool
    {
        // An inside-origin Jolt ray reports fraction zero at its origin, not a surface
        // contact. Reject it so the caller can retry from outside the geometry.
        return InHit.Get_HasHit() && InHit.Get_Fraction() > 0.0f && InHit.Get_Fraction() <= 1.0f
            && NOT InHit.Get_Position().ContainsNaN() && NOT InHit.Get_Normal().ContainsNaN()
            && NOT InHit.Get_Normal().IsNearlyZero();
    }

    auto
        Get_LegBit(
            int32 InLegIndex)
        -> uint64
    {
        return uint64{1} << InLegIndex;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralGait_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ProceduralGait_Params& InParams,
            FFragment_ProceduralGait_Current& InCurrent,
            FFragment_ProceduralGait_Requests& InRequests)
        -> void
    {
        auto Requests = MoveTemp(InRequests._Requests);
        InRequests._Requests.Reset();

        algo::ForEachRequest(Requests, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed_Cancelled;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            if (InHandle.Has<FTag_DestroyEntity_Initiate>())
            { return; }

            DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_ProceduralGait_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ProceduralGait_Params& InParams,
            FFragment_ProceduralGait_Current& InCurrent,
            const FCk_Request_ProceduralGait_ApplyPreset& InRequest)
        -> void
    {
        InParams = FFragment_ProceduralGait_Params{InRequest.Get_Timing(), InRequest.Get_Step(), InRequest.Get_Probe()};

        const auto EnabledCount = UCk_Utils_ProceduralGait_UE::Get_EnabledLegCount(InHandle);
        InCurrent._Solver.Set_Settings(UCk_Utils_ProceduralGait_UE::DoBuild_SolverSettings(InParams, EnabledCount));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Params& InParams,
            FFragment_ProceduralGait_Current& InCurrent,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto Dt = InDeltaT.Get_Seconds();
        if (InCurrent._Failed || NOT FMath::IsFinite(Dt) || Dt < 0.0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        {
            InCurrent._Ready = false;
            return;
        }

        const auto& Body = InTransform.Get_Transform();
        if (Body.ContainsNaN())
        {
            InCurrent._Ready = false;
            InCurrent._Failed = true;
            return;
        }

        const auto& Timing = InParams.Get_Timing();
        const auto& Step = InParams.Get_Step();
        const auto& Probe = InParams.Get_Probe();
        const auto Basis = Body.GetRotation();
        const auto InverseBasis = Basis.Inverse();
        const auto Up = Basis.GetAxisZ();
        const auto Velocity = InCurrent._VelocityTracker.Update(Body.GetLocation(), InDeltaT);
        const auto PlanarVelocity = InverseBasis.RotateVector(FVector::VectorPlaneProject(Velocity, Up));
        const auto YawDelta = InCurrent._Initialized
            ? FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(InCurrent._Basis.GetAxisX(), Basis.GetAxisX()), Up),
                FVector::DotProduct(InCurrent._Basis.GetAxisX(), Basis.GetAxisX()))
            : 0.0;
        const auto YawRate = Dt > 0.0 ? FMath::Abs(YawDelta) / Dt : 0.0;
        const auto Lead = (Velocity * Timing.Get_StepDuration().Get_Seconds()).GetClampedToMaxSize(Step.Get_MaxVelocityLead());

        if (InCurrent._Initialized)
        { InCurrent._Solver.TransformState(InverseBasis * InCurrent._Basis); }

        const auto LegCount = InCurrent._Legs.Num();
        auto InitialFeet = TArray<FVector, TInlineAllocator<8>>{};
        if (NOT InCurrent._Initialized)
        { InitialFeet.Reserve(LegCount); }

        auto AllLost = true;
        auto MeanFootRadius = 0.0;
        auto EnabledCount = 0;
        auto EnabledMask = ~uint64{0};
        for (auto Index = 0; Index < LegCount; ++Index)
        {
            auto& Leg = InCurrent._Legs[Index];
            auto& Input = InCurrent._Inputs[Index];
            auto& DebugLeg = InCurrent._DebugScratchLegs[Index];
            const auto WasEnabled = (InCurrent._EnabledMask & ck_procedural_gait::Get_LegBit(Index)) != 0;
            const auto LegValid = ck::IsValid(Leg);
            const auto Enabled = LegValid
                && NOT Leg.Has<FTag_DestroyEntity_Initiate>()
                && NOT Leg.Has<FTag_ProceduralLeg_Disabled>();

            Input.Set_Enabled(Enabled);
            DebugLeg.Set_Enabled(Enabled);

            if (NOT LegValid)
            {
                EnabledMask &= ~ck_procedural_gait::Get_LegBit(Index);
                if (NOT InCurrent._Initialized)
                { InitialFeet.Add(InverseBasis.RotateVector(Body.GetLocation())); }
                continue;
            }

            const auto& Placement = Leg.Get<FFragment_ProceduralLeg_Params>().Get_Placement();
            auto& LegCurrent = Leg.Get<FFragment_ProceduralLeg_Current>();
            const auto Neutral = Body.TransformPosition(Placement.Get_RestFootLocal());
            DebugLeg.Set_Id(Leg.Get<FFragment_ProceduralLeg_Params>().Get_Id());
            DebugLeg.Get_Targeting().Set_HipWorld(Body.TransformPosition(Placement.Get_HipLocal()))
                .Set_NeutralWorld(Neutral);

            if (NOT Enabled)
            {
                EnabledMask &= ~ck_procedural_gait::Get_LegBit(Index);
                if (NOT InCurrent._Initialized)
                { InitialFeet.Add(InverseBasis.RotateVector(Neutral)); }

                if (WasEnabled)
                {
                    const auto FootPosition = InCurrent._Initialized ? LegCurrent._Foot.Get_Position() : Neutral;
                    const auto FootRotation = InCurrent._Initialized ? LegCurrent._Foot.Get_Rotation() : Basis;
                    LegCurrent._FrozenFootLocal = Body.InverseTransformPosition(FootPosition);
                    LegCurrent._FrozenRotationLocal = InverseBasis * FootRotation;
                    LegCurrent._Frozen = true;
                }

                LegCurrent._Foot.Set_Position(Body.TransformPosition(LegCurrent._FrozenFootLocal))
                    .Set_Rotation((Basis * LegCurrent._FrozenRotationLocal).GetNormalized())
                    .Set_Planted(true)
                    .Set_SwingAlpha(0.0f)
                    .Set_ContactTrusted(false);

                // The frozen pose is the source of truth so a re-enabled leg swings from where it is drawn.
                // On the transition frame the solver's own reconcile freezes this same pose.
                const auto SolverHoldsDisabledLeg = InCurrent._Initialized && NOT InCurrent._Solver.IsLegEnabled(Index);
                if (SolverHoldsDisabledLeg)
                {
                    const auto PoseSynced = InCurrent._Solver.SetDisabledPose(Index,
                        InverseBasis.RotateVector(LegCurrent._Foot.Get_Position()),
                        (InverseBasis * LegCurrent._Foot.Get_Rotation()).GetNormalized(),
                        InverseBasis.RotateVector(Up));

                    CK_ENSURE_IF_NOT(PoseSynced,
                        TEXT("Procedural gait [{}] could not sync the frozen pose of disabled leg [{}]; feature is failed."),
                        InHandle, Index)
                    {
                        InCurrent._Ready = false;
                        InCurrent._Failed = true;
                        return;
                    }
                }
                continue;
            }

            if (NOT WasEnabled)
            { LegCurrent._Frozen = false; }

            ++EnabledCount;
            const auto Ideal = Neutral + Lead;
            DebugLeg.Get_Targeting().Set_QueryTarget(Ideal);
            MeanFootRadius += FVector::VectorPlaneProject(Placement.Get_RestFootLocal(), FVector::UpVector).Size();
            const auto Radial = FVector::VectorPlaneProject(Ideal - Body.GetLocation(), Up).GetSafeNormal();

            auto Hit = FCk_Jolt_HitResult{};
            for (auto Attempt = 0; Attempt < ProceduralGroundProbeAttempts; ++Attempt)
            {
                const auto Span = MakeProceduralGroundProbeSpan(Attempt, Probe.Get_Up(), Probe.Get_Down(), Probe.Get_OutwardLean());
                const auto Axis = FProceduralGaitSolver::ComputeTraceAxis(Up, Radial, Span.Get_OutwardLean());
                const auto ProbeStart = Ideal + Axis * Span.Get_UpDistance();
                const auto ProbeEnd = Ideal - Axis * Span.Get_DownDistance();
                Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(World, ProbeStart, ProbeEnd, Probe.Get_QueryFilter());
                DebugLeg.Get_Probe().Set_Start(ProbeStart)
                    .Set_End(ProbeEnd)
                    .Set_AttemptCount(Attempt + 1)
                    .Set_Hit(Hit.Get_HasHit())
                    .Set_HitFraction(Hit.Get_Fraction())
                    .Set_HitPosition(Hit.Get_Position())
                    .Set_HitNormal(Hit.Get_Normal());

                if (ck_procedural_gait::Get_TrustedHit(Hit))
                { break; }
            }

            const auto Trusted = ck_procedural_gait::Get_TrustedHit(Hit);
            InCurrent._Probes[Index].Advance(Trusted, InDeltaT, Probe.Get_ContactGrace());
            const auto ProbeState = InCurrent._Probes[Index].Get_State();
            // A brief miss holds the plant by withholding a target. A prolonged loss lets the
            // leg gather toward its CURRENT rest pose; the fallback never becomes trusted ground.
            const auto TargetValid = ProbeState != EProceduralFootProbeState::Guessing;
            AllLost &= ProbeState == EProceduralFootProbeState::Lost;
            const auto Position = Trusted ? Hit.Get_Position() : Ideal;
            const auto Normal = Trusted ? Hit.Get_Normal().GetSafeNormal() : Up;

            Input.Set_IdealTarget(InverseBasis.RotateVector(Position))
                .Set_GroundNormal(InverseBasis.RotateVector(Normal))
                .Set_FacingDirection(FVector::ForwardVector)
                .Set_PhaseOffset(Placement.Get_PhaseOffset())
                .Set_StepThresholdScale(Placement.Get_StepThresholdScale())
                .Set_TargetValid(TargetValid)
                .Set_ClearanceGroundZ(-FLT_MAX);

            if (NOT InCurrent._Initialized)
            { InitialFeet.Add(Input.Get_IdealTarget()); }
            else if (NOT LegCurrent._Foot.Get_Planted())
            {
                const auto Foot = LegCurrent._Foot.Get_Position();
                const auto ClearanceHit = UCk_Utils_JoltQuery_UE::Get_RayCast(World,
                    Foot + Up * Probe.Get_Up(), Foot - Up * Probe.Get_Down(), Probe.Get_QueryFilter());

                if (ck_procedural_gait::Get_TrustedHit(ClearanceHit))
                { Input.Set_ClearanceGroundZ(InverseBasis.RotateVector(ClearanceHit.Get_Position()).Z); }
            }

            LegCurrent._Foot.Set_ContactTrusted(Trusted);
            DebugLeg.Get_Targeting().Set_IdealTarget(Position)
                .Set_TargetValid(TargetValid);
            DebugLeg.Get_Foot().Set_ContactTrusted(Trusted);
            DebugLeg.Get_Probe().Set_State(ProbeState)
                .Set_MissingContact(InCurrent._Probes[Index].Get_MissingDuration());
        }

        if (NOT InCurrent._Initialized)
        {
            if (NOT InCurrent._Solver.Reset(InitialFeet))
            {
                InCurrent._Failed = true;
                return;
            }

            InCurrent._Initialized = true;
        }

        if (EnabledMask != InCurrent._EnabledMask)
        {
            if (Timing.Get_MaxSimultaneousSwings() == 0)
            { InCurrent._Solver.Set_Settings(UCk_Utils_ProceduralGait_UE::DoBuild_SolverSettings(InParams, EnabledCount)); }

            InCurrent._EnabledMask = EnabledMask;
            UUtils_Signal_OnProceduralGait_LegSetChanged::Broadcast(InHandle, MakePayload(InHandle, EnabledCount, LegCount));
        }

        auto Airborne = AllLost;
        if (UCk_Utils_SurfaceMotion_UE::Has(InHandle))
        { Airborne = NOT UCk_Utils_SurfaceMotion_UE::Get_IsGrounded(UCk_Utils_SurfaceMotion_UE::CastChecked(InHandle)); }

        const auto CadenceSpeed = PlanarVelocity.Size() + YawRate * MeanFootRadius / FMath::Max(EnabledCount, 1);
        const auto Solved = InCurrent._Solver.Step(InDeltaT, CadenceSpeed, PlanarVelocity,
            InCurrent._Inputs, InCurrent._Outputs, Airborne);
        CK_ENSURE_IF_NOT(Solved, TEXT("Procedural gait solver rejected runtime inputs; feature is failed, planted state retained."))
        {
            InCurrent._Ready = false;
            InCurrent._Failed = true;
            return;
        }

        for (auto Index = 0; Index < LegCount; ++Index)
        {
            if (NOT InCurrent._Inputs[Index].Get_Enabled())
            { continue; }

            const auto& Output = InCurrent._Outputs[Index];
            InCurrent._Legs[Index].Get<FFragment_ProceduralLeg_Current>()._Foot
                .Set_Position(Basis.RotateVector(Output.Get_Position()))
                .Set_Normal(Basis.RotateVector(Output.Get_Normal()))
                .Set_Rotation((Basis * Output.Get_Rotation()).GetNormalized())
                .Set_SwingAlpha(Output.Get_SwingAlpha())
                .Set_Planted(Output.Get_Planted());
        }

        if (InDeltaT > FCk_Time{})
        {
            for (auto Index = 0; Index < LegCount; ++Index)
            {
                const auto& Output = InCurrent._Outputs[Index];
                const auto& State = InCurrent._Solver.GetLegState(Index);
                InCurrent._DebugScratchLegs[Index].Get_Foot().Set_PlantedPosition(Basis.RotateVector(State.Get_Plant().Get_Position()))
                    .Set_SwingTarget(Basis.RotateVector(State.Get_Swing().Get_Target()))
                    .Set_Position(Basis.RotateVector(Output.Get_Position()))
                    .Set_Rotation((Basis * Output.Get_Rotation()).GetNormalized())
                    .Set_Normal(Basis.RotateVector(Output.Get_Normal()))
                    .Set_Planted(Output.Get_Planted())
                    .Set_SwingAlpha(Output.Get_SwingAlpha())
                    .Set_PhaseOffset(InCurrent._Solver.GetEffectivePhaseOffset(Index));
                InCurrent._DebugScratchLegs[Index].Get_Targeting().Set_StepThreshold(InCurrent._Solver.Get_Settings().Get_Step().Get_Threshold()
                    * InCurrent._Inputs[Index].Get_StepThresholdScale());
            }

            auto& Snapshot = InCurrent._DebugSnapshot;
            Snapshot.Get_Status().Set_HasAcceptedSample(true);
            Snapshot.Get_Sample().Set_FrameNumber(GFrameCounter)
                .Set_Sequence(Snapshot.Get_Sample().Get_Sequence() + 1)
                .Set_Time(FCk_Time{World->GetTimeSeconds()});
            Snapshot.Get_Gait().Set_BodyTransform(Body)
                .Set_Velocity(Velocity)
                .Set_CadenceSpeed(CadenceSpeed)
                .Set_CadenceScale(InCurrent._Solver.Get_LastCadenceScale())
                .Set_Clock(InCurrent._Solver.GetGaitClock())
                .Set_Airborne(InCurrent._Solver.IsAirborne())
                .Set_RestTime(InCurrent._Solver.GetRestTime())
                .Set_SupportNormal(Up);
            Snapshot.Set_Legs(InCurrent._DebugScratchLegs);

            if (InHandle.Has<FFragment_SurfaceMotion_Current>())
            {
                const auto& Motion = InHandle.Get<FFragment_SurfaceMotion_Current>();
                Snapshot.Get_Motion().Set_Velocity(Motion._Velocity)
                    .Set_RequestedDirection(Motion._Direction)
                    .Set_RequestedSpeed(Motion._Speed)
                    .Set_Grounded(Motion._Grounded)
                    .Set_TrustedContact(Motion._TrustedContact)
                    .Set_MissingContact(Motion._MissingContact);
                Snapshot.Get_Gait().Set_SupportNormal(Motion._SupportNormal);
            }
        }

        InCurrent._Basis = Basis;
        InCurrent._Ready = true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
