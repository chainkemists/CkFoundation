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

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_gait
{
    auto
        Get_TrustedHit(
            const FCk_Jolt_HitResult& InHit,
            const FVector& InRayDirection)
        -> bool
    {
        // An inside-origin Jolt ray reports fraction zero at its origin, not a surface contact, and a normal facing
        // along the ray is the back of a surface the ray started behind. Reject both so the caller can retry from
        // outside the geometry.
        return InHit.Get_HasHit() && InHit.Get_Fraction() > 0.0f && InHit.Get_Fraction() <= 1.0f
            && NOT InHit.Get_Position().ContainsNaN() && NOT InHit.Get_Normal().ContainsNaN()
            && NOT InHit.Get_Normal().IsNearlyZero()
            && FVector::DotProduct(InHit.Get_Normal(), InRayDirection) < -KINDA_SMALL_NUMBER;
    }

    // Returns the first trusted hit across the retry spans; the debug probe records the last attempt either way.
    auto
        Get_GroundHit(
            UWorld* InWorld,
            const FVector& InIdeal,
            const FVector& InUp,
            const FVector& InRadial,
            const FCk_ProceduralGait_Probe& InProbe,
            FCk_ProceduralAnimation_DebugProbe* InDebugProbe)
        -> TOptional<FCk_Jolt_HitResult>
    {
        for (auto Attempt = 0; Attempt < ck::ProceduralGroundProbeAttempts; ++Attempt)
        {
            const auto Span = ck::MakeProceduralGroundProbeSpan(Attempt, InProbe.Get_Up(), InProbe.Get_Down(), InProbe.Get_OutwardLean());
            const auto Axis = ck::FProceduralGaitSolver::ComputeTraceAxis(InUp, InRadial, Span.Get_OutwardLean());
            const auto ProbeStart = InIdeal + Axis * Span.Get_UpDistance();
            const auto ProbeEnd = InIdeal - Axis * Span.Get_DownDistance();
            const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, ProbeStart, ProbeEnd, InProbe.Get_QueryFilter());

            if (ck::IsValid(InDebugProbe, ck::IsValid_Policy_NullptrOnly{}))
            {
                InDebugProbe->Set_Start(ProbeStart)
                    .Set_End(ProbeEnd)
                    .Set_AttemptCount(Attempt + 1)
                    .Set_Hit(Hit.Get_HasHit())
                    .Set_HitFraction(Hit.Get_Fraction())
                    .Set_HitPosition(Hit.Get_Position())
                    .Set_HitNormal(Hit.Get_Normal());
            }

            if (Get_TrustedHit(Hit, (ProbeEnd - ProbeStart).GetSafeNormal()))
            { return Hit; }
        }

        return {};
    }

    // Ground found beyond TargetReachFraction of the reach is clamped toward the hip like the query point and probed once
    // more there: the solver's own clamp keeps a target's support-frame height, which across a convex bend lies inside the
    // ground. Ground still beyond ForceStepReachFraction is left untrusted, like a miss, so the plant holds and then gathers
    // toward its rest target instead of stepping down a ledge or into a gap it cannot reach.
    auto
        Get_ReachableGroundHit(
            UWorld* InWorld,
            const FQuat& InBasis,
            const FVector& InBodyLocation,
            const FVector& InHip,
            const FVector& InQuery,
            float InReach,
            const FCk_ProceduralGait_Probe& InProbe,
            const FCk_ProceduralGait_Step& InStep,
            FCk_ProceduralAnimation_DebugProbe* InDebugProbe)
        -> TOptional<FCk_Jolt_HitResult>
    {
        const auto Up = InBasis.GetAxisZ();
        const auto ProbeAt = [&](const FVector& InPoint) -> TOptional<FCk_Jolt_HitResult>
        {
            const auto Radial = FVector::VectorPlaneProject(InPoint - InBodyLocation, Up).GetSafeNormal();
            return Get_GroundHit(InWorld, InPoint, Up, Radial, InProbe, InDebugProbe);
        };

        auto Hit = ProbeAt(InQuery);
        const auto TargetLimit = InStep.Get_TargetReachFraction() * InReach;
        if (Hit.IsSet() && FVector::Dist(Hit->Get_Position(), InHip) > TargetLimit)
        {
            const auto InverseBasis = InBasis.Inverse();
            Hit = ProbeAt(InBasis.RotateVector(ck::FProceduralGaitSolver::ClampToReach(InverseBasis.RotateVector(InHip),
                InverseBasis.RotateVector(Hit->Get_Position()), TargetLimit)));
        }

        if (Hit.IsSet() && FVector::Dist(Hit->Get_Position(), InHip) > InStep.Get_ForceStepReachFraction() * InReach)
        { return {}; }

        return Hit;
    }

    auto
        Get_LegBit(
            int32 InLegIndex)
        -> uint64
    {
        return uint64{1} << InLegIndex;
    }

    auto
        DoPublish_FootPhaseChange(
            FCk_Handle_ProceduralLeg& InLeg,
            const ck::FFragment_ProceduralLeg& InLegComp,
            ECk_ProceduralLeg_FootPhase InPreviousPhase,
            const FVector& InPreviousPosition,
            double InDeltaSeconds)
        -> void
    {
        const auto& Foot = InLegComp.Get_Foot();
        if (Foot.Get_Phase() == InPreviousPhase)
        { return; }

        if (Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted)
        {
            const auto LandingSpeed = InDeltaSeconds > 0.0 ? (Foot.Get_Position() - InPreviousPosition).Size() / InDeltaSeconds : 0.0;
            const auto Footfall = FCk_ProceduralLeg_Footfall{Foot.Get_Position(), Foot.Get_Normal(), Foot.Get_Contact(),
                static_cast<float>(LandingSpeed)};
            ck::UUtils_Signal_OnProceduralLeg_Planted::Broadcast(InLeg, ck::MakePayload(InLeg, Footfall));
            return;
        }

        constexpr auto LiftSpeed = 0.0f;
        const auto Footfall = FCk_ProceduralLeg_Footfall{Foot.Get_Position(), Foot.Get_Normal(), Foot.Get_Contact(), LiftSpeed};
        ck::UUtils_Signal_OnProceduralLeg_Lifted::Broadcast(InLeg, ck::MakePayload(InLeg, Footfall));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralGait_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto& Body = InTransform.Get_Transform();
        const auto BodyFinite = NOT Body.ContainsNaN();
        CK_ENSURE_IF_NOT(BodyFinite,
            TEXT("Procedural gait [{}] body transform contains NaN; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::NanBody);
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Basis = Body.GetRotation();
        const auto InverseBasis = Basis.Inverse();
        const auto Up = Basis.GetAxisZ();

        auto InitialFeet = TArray<FVector, TInlineAllocator<8>>{};
        InitialFeet.Reserve(InGaitComp._Legs.Num());
        for (auto& Leg : InGaitComp._Legs)
        {
            if (ck::Is_NOT_Valid(Leg))
            {
                InitialFeet.Add(InverseBasis.RotateVector(Body.GetLocation()));
                continue;
            }

            const auto& Params = Leg.Get<FFragment_ProceduralLeg_Params>();
            const auto& Placement = Params.Get_Placement();
            const auto Neutral = Body.TransformPosition(Placement.Get_RestFootLocal());
            const auto Hit = ck_procedural_gait::Get_ReachableGroundHit(World, Basis, Body.GetLocation(),
                Body.TransformPosition(Placement.Get_HipLocal()), Neutral, UCk_Utils_ProceduralGait_UE::DoGet_Reach(Params.Get_Chain()),
                InTunables.Get_Probe(), InTunables.Get_Step(), nullptr);
            const auto Trusted = Hit.IsSet();
            const auto Position = Trusted ? Hit->Get_Position() : Neutral;
            const auto Normal = Trusted ? Hit->Get_Normal().GetSafeNormal() : Up;

            // Update freezes a leg disabled before Add from this published foot, so it must already hold the probed pose.
            Leg.Get<FFragment_ProceduralLeg>()._Foot.Set_Position(Position)
                .Set_Normal(Normal)
                .Set_Rotation(Basis)
                .Set_SwingAlpha(0.0f)
                .Set_Phase(ECk_ProceduralLeg_FootPhase::Planted)
                .Set_Contact(Trusted ? ECk_ProceduralLeg_FootContact::Trusted : ECk_ProceduralLeg_FootContact::Guessed);

            InitialFeet.Add(InverseBasis.RotateVector(Position));
        }

        const auto SolverReset = InGaitComp._Solver.Reset(InitialFeet);
        CK_ENSURE_IF_NOT(SolverReset,
            TEXT("Procedural gait [{}] solver rejected the initial foot positions; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::SolverReset);
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        InGaitComp._Basis = Basis;
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Requests& InRequestsComp)
        -> void
    {
        auto Requests = MoveTemp(InRequestsComp._Requests);
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(Requests, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed_Cancelled;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            if (InHandle.Has<FTag_DestroyEntity_Initiate>())
            { return; }

            DoHandleRequest(InHandle, InTunables, InGaitComp, InRequest);
            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_ProceduralGait_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            const FCk_Request_ProceduralGait_ApplyPreset& InRequest)
        -> void
    {
        InTunables = FFragment_ProceduralGait_Tunables{InRequest.Get_Timing(), InRequest.Get_Step(), InRequest.Get_Probe()};

        InGaitComp._Solver.Set_Settings(UCk_Utils_ProceduralGait_UE::DoBuild_SolverSettings(InTunables, InGaitComp._Legs,
            InGaitComp._EnabledMask, InGaitComp._ReachCadenceFloor, InGaitComp._ReachSkippedLegs));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto Dt = InDeltaT.Get_Seconds();
        if (NOT FMath::IsFinite(Dt) || Dt < 0.0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto& Body = InTransform.Get_Transform();
        const auto BodyFinite = NOT Body.ContainsNaN();
        CK_ENSURE_IF_NOT(BodyFinite,
            TEXT("Procedural gait [{}] body transform contains NaN; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::NanBody);
            return;
        }

        const auto& Timing = InTunables.Get_Timing();
        const auto& Step = InTunables.Get_Step();
        const auto& Probe = InTunables.Get_Probe();
        const auto Basis = Body.GetRotation();
        const auto InverseBasis = Basis.Inverse();
        const auto Up = Basis.GetAxisZ();
        const auto Velocity = InGaitComp._VelocityTracker.Update(Body.GetLocation(), InDeltaT);
        const auto PlanarVelocity = InverseBasis.RotateVector(FVector::VectorPlaneProject(Velocity, Up));
        const auto YawDelta = FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(InGaitComp._Basis.GetAxisX(), Basis.GetAxisX()), Up),
            FVector::DotProduct(InGaitComp._Basis.GetAxisX(), Basis.GetAxisX()));
        const auto YawRate = Dt > 0.0 ? FMath::Abs(YawDelta) / Dt : 0.0;
        const auto Lead = (FVector::VectorPlaneProject(Velocity, Up) * Timing.Get_StepDuration().Get_Seconds())
            .GetClampedToMaxSize(Step.Get_MaxVelocityLead());

        InGaitComp._Solver.TransformState(InverseBasis * InGaitComp._Basis);

        const auto LegCount = InGaitComp._Legs.Num();

        auto Inputs = TArray<FProceduralGaitLegInput, TInlineAllocator<8>>{};
        Inputs.SetNum(LegCount);
        auto Outputs = TArray<FProceduralGaitLegOutput, TInlineAllocator<8>>{};
        Outputs.SetNum(LegCount);

        auto AllLost = true;
        auto MeanFootRadius = 0.0;
        auto EnabledCount = 0;
        auto EnabledMask = ~uint64{0};
        for (auto Index = 0; Index < LegCount; ++Index)
        {
            auto& Leg = InGaitComp._Legs[Index];
            auto& Input = Inputs[Index];
            auto& DebugLeg = InDebugComp._ScratchLegs[Index];
            const auto WasEnabled = (InGaitComp._EnabledMask & ck_procedural_gait::Get_LegBit(Index)) != 0;
            const auto LegValid = ck::IsValid(Leg);
            const auto Enabled = LegValid
                && NOT Leg.Has<FTag_DestroyEntity_Initiate>()
                && NOT Leg.Has<FTag_ProceduralLeg_Disabled>();

            Input.Set_Enabled(Enabled);
            DebugLeg.Set_Enabled(Enabled);

            if (NOT LegValid)
            {
                EnabledMask &= ~ck_procedural_gait::Get_LegBit(Index);
                continue;
            }

            const auto& Params = Leg.Get<FFragment_ProceduralLeg_Params>();
            const auto& Placement = Params.Get_Placement();
            auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            const auto Neutral = Body.TransformPosition(Placement.Get_RestFootLocal());
            const auto Hip = Body.TransformPosition(Placement.Get_HipLocal());
            Input.Set_PhaseOffset(Placement.Get_PhaseOffset())
                .Set_StepThresholdScale(Placement.Get_StepThresholdScale());
            DebugLeg.Set_Id(Params.Get_Id());
            DebugLeg.Get_Targeting().Set_HipWorld(Hip)
                .Set_NeutralWorld(Neutral);

            if (NOT Enabled)
            {
                EnabledMask &= ~ck_procedural_gait::Get_LegBit(Index);

                if (WasEnabled)
                {
                    Leg.AddOrGet<FFragment_ProceduralLeg_FrozenPose>()
                        .Set_FootLocal(Body.InverseTransformPosition(LegComp._Foot.Get_Position()))
                        .Set_RotationLocal((InverseBasis * LegComp._Foot.Get_Rotation()).GetNormalized());
                }

                const auto& Frozen = Leg.Get<FFragment_ProceduralLeg_FrozenPose>();
                const auto PreviousPhase = LegComp._Foot.Get_Phase();
                const auto PreviousPosition = LegComp._Foot.Get_Position();
                LegComp._Foot.Set_Position(Body.TransformPosition(Frozen.Get_FootLocal()))
                    .Set_Rotation((Basis * Frozen.Get_RotationLocal()).GetNormalized())
                    .Set_Phase(ECk_ProceduralLeg_FootPhase::Planted)
                    .Set_SwingAlpha(0.0f)
                    .Set_Contact(ECk_ProceduralLeg_FootContact::Guessed);

                // A detached leg stays valid until its destruction completes, and its last pose is frozen here too;
                // freezing a swinging foot as it goes is not a plant.
                if (NOT Leg.Has<FTag_DestroyEntity_Initiate>())
                { ck_procedural_gait::DoPublish_FootPhaseChange(Leg, LegComp, PreviousPhase, PreviousPosition, Dt); }

                // The frozen pose is the source of truth so a re-enabled leg swings from where it is drawn.
                // On the transition frame the solver's own reconcile freezes this same pose.
                const auto SolverHoldsDisabledLeg = NOT InGaitComp._Solver.IsLegEnabled(Index);
                if (SolverHoldsDisabledLeg)
                {
                    const auto PoseSynced = InGaitComp._Solver.SetDisabledPose(Index,
                        InverseBasis.RotateVector(LegComp._Foot.Get_Position()),
                        (InverseBasis * LegComp._Foot.Get_Rotation()).GetNormalized(),
                        InverseBasis.RotateVector(Up));

                    CK_ENSURE_IF_NOT(PoseSynced,
                        TEXT("Procedural gait [{}] could not sync the frozen pose of disabled leg [{}]; feature is failed."),
                        InHandle, Index)
                    {
                        InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::DisabledPoseSync);
                        return;
                    }
                }
                continue;
            }

            if (NOT WasEnabled)
            { Leg.Try_Remove<FFragment_ProceduralLeg_FrozenPose>(); }

            ++EnabledCount;
            const auto Reach = UCk_Utils_ProceduralGait_UE::DoGet_Reach(Params.Get_Chain());
            const auto HipSupport = InverseBasis.RotateVector(Hip);
            const auto Ideal = Basis.RotateVector(FProceduralGaitSolver::ClampToReach(HipSupport,
                InverseBasis.RotateVector(Neutral + Lead), Step.Get_TargetReachFraction() * Reach));
            DebugLeg.Get_Targeting().Set_QueryTarget(Ideal);
            MeanFootRadius += FVector::VectorPlaneProject(Placement.Get_RestFootLocal(), FVector::UpVector).Size();

            const auto Hit = ck_procedural_gait::Get_ReachableGroundHit(World, Basis, Body.GetLocation(), Hip, Ideal, Reach, Probe, Step,
                &DebugLeg.Get_Probe());
            const auto Trusted = Hit.IsSet();
            const auto ProbeAdvanced = InGaitComp._Probes[Index].Advance(Trusted, InDeltaT, Probe.Get_ContactGrace());
            CK_ENSURE_IF_NOT(ProbeAdvanced,
                TEXT("Procedural gait [{}] foot probe of leg [{}] rejected its elapsed time or contact grace; feature is failed."),
                InHandle, Index)
            {
                InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::ProbeAdvance);
                return;
            }

            const auto ProbeState = InGaitComp._Probes[Index].Get_State();
            // A brief miss holds the plant by withholding a target. A prolonged loss lets the
            // leg gather toward its CURRENT rest pose; the fallback never becomes trusted ground.
            const auto TargetValid = ProbeState != EProceduralFootProbeState::Guessing;
            AllLost &= ProbeState == EProceduralFootProbeState::Lost;
            const auto Position = Trusted ? Hit->Get_Position() : Ideal;
            const auto Normal = Trusted ? Hit->Get_Normal().GetSafeNormal() : Up;

            Input.Set_IdealTarget(InverseBasis.RotateVector(Position))
                .Set_GroundNormal(InverseBasis.RotateVector(Normal))
                .Set_FacingDirection(FVector::ForwardVector)
                .Set_TargetValid(TargetValid)
                .Set_ClearanceGroundZ(-FLT_MAX)
                .Set_Hip(HipSupport)
                .Set_Reach(Reach);

            if (LegComp._Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Swinging)
            {
                const auto Foot = LegComp._Foot.Get_Position();
                const auto ClearanceHit = UCk_Utils_JoltQuery_UE::Get_RayCast(World,
                    Foot + Up * Probe.Get_Up(), Foot - Up * Probe.Get_Down(), Probe.Get_QueryFilter());

                if (ck_procedural_gait::Get_TrustedHit(ClearanceHit, -Up))
                { Input.Set_ClearanceGroundZ(InverseBasis.RotateVector(ClearanceHit.Get_Position()).Z); }
            }

            LegComp._Foot.Set_Contact(Trusted ? ECk_ProceduralLeg_FootContact::Trusted : ECk_ProceduralLeg_FootContact::Guessed);
            DebugLeg.Get_Targeting().Set_IdealTarget(Position)
                .Set_TargetValid(TargetValid);
            DebugLeg.Get_Foot().Set_ContactTrusted(Trusted);
            DebugLeg.Get_Probe().Set_State(ProbeState)
                .Set_MissingContact(InGaitComp._Probes[Index].Get_MissingDuration());
        }

        if (EnabledMask != InGaitComp._EnabledMask)
        {
            InGaitComp._EnabledMask = EnabledMask;
            InGaitComp._Solver.Set_Settings(UCk_Utils_ProceduralGait_UE::DoBuild_SolverSettings(InTunables, InGaitComp._Legs,
                InGaitComp._EnabledMask, InGaitComp._ReachCadenceFloor, InGaitComp._ReachSkippedLegs));
            UUtils_Signal_OnProceduralGait_LegSetChanged::Broadcast(InHandle, MakePayload(InHandle, EnabledCount, LegCount));
        }

        auto Airborne = EnabledCount > 0 && AllLost;
        if (UCk_Utils_SurfaceMotion_UE::Has(InHandle))
        {
            Airborne = UCk_Utils_SurfaceMotion_UE::Get_Support(UCk_Utils_SurfaceMotion_UE::CastChecked(InHandle))
                == ECk_SurfaceMotion_Support::Airborne;
        }

        const auto CadenceSpeed = PlanarVelocity.Size() + YawRate * MeanFootRadius / FMath::Max(EnabledCount, 1);
        const auto Solved = InGaitComp._Solver.Step(InDeltaT, CadenceSpeed, PlanarVelocity, Inputs, Outputs, Airborne);
        CK_ENSURE_IF_NOT(Solved, TEXT("Procedural gait [{}] solver rejected runtime inputs; feature is failed, planted state retained."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::SolverStep);
            return;
        }

        for (auto Index = 0; Index < LegCount; ++Index)
        {
            if (NOT Inputs[Index].Get_Enabled())
            { continue; }

            const auto& Output = Outputs[Index];
            auto& Leg = InGaitComp._Legs[Index];
            auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            const auto PreviousPhase = LegComp._Foot.Get_Phase();
            const auto PreviousPosition = LegComp._Foot.Get_Position();
            LegComp._Foot
                .Set_Position(Basis.RotateVector(Output.Get_Position()))
                .Set_Normal(Basis.RotateVector(Output.Get_Normal()))
                .Set_Rotation((Basis * Output.Get_Rotation()).GetNormalized())
                .Set_SwingAlpha(Output.Get_SwingAlpha())
                .Set_Phase(Output.Get_Planted() ? ECk_ProceduralLeg_FootPhase::Planted : ECk_ProceduralLeg_FootPhase::Swinging);

            ck_procedural_gait::DoPublish_FootPhaseChange(Leg, LegComp, PreviousPhase, PreviousPosition, Dt);
        }

        if (InDeltaT > FCk_Time{})
        {
            ++InGaitComp._SolveSequence;

            for (auto Index = 0; Index < LegCount; ++Index)
            {
                const auto& Output = Outputs[Index];
                const auto& State = InGaitComp._Solver.GetLegState(Index);
                InDebugComp._ScratchLegs[Index].Get_Foot().Set_PlantedPosition(Basis.RotateVector(State.Get_Plant().Get_Position()))
                    .Set_SwingTarget(Basis.RotateVector(State.Get_Swing().Get_Target()))
                    .Set_Position(Basis.RotateVector(Output.Get_Position()))
                    .Set_Rotation((Basis * Output.Get_Rotation()).GetNormalized())
                    .Set_Normal(Basis.RotateVector(Output.Get_Normal()))
                    .Set_Planted(Output.Get_Planted())
                    .Set_SwingAlpha(Output.Get_SwingAlpha())
                    .Set_PhaseOffset(InGaitComp._Solver.GetEffectivePhaseOffset(Index));
                InDebugComp._ScratchLegs[Index].Get_Targeting().Set_StepThreshold(InGaitComp._Solver.Get_Settings().Get_Step().Get_Threshold()
                    * Inputs[Index].Get_StepThresholdScale());
            }

            auto& Snapshot = InDebugComp._Snapshot;
            Snapshot.Get_Status().Set_HasAcceptedSample(true);
            Snapshot.Get_Sample().Set_FrameNumber(GFrameCounter)
                .Set_Sequence(InGaitComp._SolveSequence)
                .Set_Time(FCk_Time{World->GetTimeSeconds()});
            Snapshot.Get_Gait().Set_BodyTransform(Body)
                .Set_Velocity(Velocity)
                .Set_CadenceSpeed(CadenceSpeed)
                .Set_CadenceScale(InGaitComp._Solver.Get_LastCadenceScale())
                .Set_Clock(InGaitComp._Solver.GetGaitClock())
                .Set_Airborne(InGaitComp._Solver.IsAirborne())
                .Set_RestTime(InGaitComp._Solver.GetRestTime())
                .Set_SupportNormal(Up)
                .Set_CadenceSpeedRef(InGaitComp._Solver.Get_Settings().Get_Cadence().Get_CadenceSpeedRef())
                .Set_ReachCadenceFloor(InGaitComp._ReachCadenceFloor)
                .Set_ReachSkippedLegs(InGaitComp._ReachSkippedLegs);
            Snapshot.Set_Legs(InDebugComp._ScratchLegs);

            if (InHandle.Has<FFragment_SurfaceMotion>() && InHandle.Has<FFragment_SurfaceMotion_Support>())
            {
                const auto& Motion = InHandle.Get<FFragment_SurfaceMotion>();
                const auto& Support = InHandle.Get<FFragment_SurfaceMotion_Support>();
                Snapshot.Get_Motion().Set_Velocity(Support._Velocity)
                    .Set_RequestedDirection(Motion._Direction)
                    .Set_RequestedSpeed(Motion._Speed)
                    .Set_Grounded(Support._Support == ECk_SurfaceMotion_Support::Grounded)
                    .Set_TrustedContact(Support._ContactQuery == ECk_SurfaceMotion_ContactQuery::Trusted)
                    .Set_MissingContact(Support._MissingContact);
                Snapshot.Get_Gait().Set_SupportNormal(Support._SupportNormal);
            }
        }

        InGaitComp._Basis = Basis;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
