#include "CkJoltWorld_Processor.h"

#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"

#include <Async/Async.h>
#include <Engine/World.h>
#include <ProfilingDebugging/CountersTrace.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_WaitForAsync);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_TransformWriters);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_DrainEvents);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_PlanStep);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_Step);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltPhysics_WaitForAsync"), STAT_CkJolt_WaitForAsync, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_DrainQueue"), STAT_CkJolt_ContactsDrainQueue, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_Update_Async"), STAT_CkJolt_UpdateAsync, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_Update"), STAT_CkJolt_Update, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltStep_Characters"), STAT_CkJolt_StepCharacters, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltStep_PhysicsSystemUpdate"), STAT_CkJolt_StepPhysicsSystemUpdate, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltStep_PoseCapture"), STAT_CkJolt_StepPoseCapture, STATGROUP_CkJolt);

// The whole fixed-step pump; wraps the nested STAT_CkJolt_Update/_UpdateAsync (the Update loop alone).
DECLARE_CYCLE_STAT(TEXT("JoltWorld_Step"), STAT_CkJolt_WorldStep, STATGROUP_CkJolt);

// Global trace counters are meaningful for the packaged, single-world performance lane. Atomic storage keeps
// multi-world PIE race-free, but values from concurrent worlds still interleave and must not be read as one census.
TRACE_DECLARE_ATOMIC_INT_COUNTER(CkJolt_FixedSteps, TEXT("CkJolt/Fixed Steps"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(CkJolt_TotalBodies, TEXT("CkJolt/Total Bodies"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(
    CkJolt_ActiveRigidBodySamplesSum,
    TEXT("CkJolt/Active Rigid Body Samples Sum"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(CkJolt_MaxActiveRigidBodies, TEXT("CkJolt/Max Active Rigid Bodies"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(
    CkJolt_ActiveSoftBodySamplesSum,
    TEXT("CkJolt/Active Soft Body Samples Sum"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(CkJolt_MaxActiveSoftBodies, TEXT("CkJolt/Max Active Soft Bodies"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(CkJolt_RegisteredCharacters, TEXT("CkJolt/Registered Characters"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(
    CkJolt_TouchingManifoldCallbacksPerBatch,
    TEXT("CkJolt/Touching Manifold Callbacks Per Batch"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(
    CkJolt_MaxTouchingManifoldCallbacksPerStep,
    TEXT("CkJolt/Max Touching Manifold Callbacks Per Step"));
TRACE_DECLARE_ATOMIC_INT_COUNTER(CkJolt_UpdateErrorBits, TEXT("CkJolt/Update Error Bits"));

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
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
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_JoltWorld_WaitForAsync::
        FProcessor_JoltWorld_WaitForAsync(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltWorld_WaitForAsync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck::jolt::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        if (JoltWorld->Get_AsyncFuture().IsValid())
        {
            SCOPE_CYCLE_COUNTER(STAT_CkJolt_WaitForAsync);
            JoltWorld->WaitForAsyncStep();
        }

        // Dirty-flag guarded: a no-op in sync mode because Step already applied + cleared this frame's poses.
        JoltWorld->DoApplyPoseBuffer_GameThread(_TransientEntity);
        JoltWorld->DoApplyCharacterPoses_GameThread(_TransientEntity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_JoltWorld_TransformWriters::
        FProcessor_JoltWorld_TransformWriters(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltWorld_TransformWriters::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_JoltWorld_DrainEvents::
        FProcessor_JoltWorld_DrainEvents(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltWorld_DrainEvents::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck::jolt::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkJolt_ContactsDrainQueue);
        JoltWorld->DrainEventsAndRoute();
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_JoltWorld_PlanStep::
        FProcessor_JoltWorld_PlanStep(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltWorld_PlanStep::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck::jolt::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        // Invalid or paused: freeze the accumulator and zero the plan, so the executor runs no sub-steps
        // and KinematicPush (PendingSimTime <= 0) early-outs.
        //
        // The world's own block is decided FIRST and the DEBUG gate is consumed only when it does not apply: a
        // step-once spent on a frame the ENGINE is already pausing would step nothing and the click would be
        // silently lost. The gate is consumed here and only here — a step-once granted at this point plans
        // exactly one step, which is why it cannot be a flag the Step processor interprets for itself.
        const auto World = JoltWorld->Get_World();
        const auto WorldBlocksStep = ck::Is_NOT_Valid(World) || World->IsPaused();
        const auto DebugPauseBlocksStep = NOT WorldBlocksStep && JoltWorld->TryConsume_DebugPauseGate();

        // A GRANT outranks the debug pause, which is why it is read here rather than beside the step-once branch
        // below. A snapshot load holds game time at a standstill, so InDeltaT plans zero steps and the bodies a
        // load waits on would never settle; the grant is the loader saying "step anyway, N times". Consumed here
        // and only here — like the debug gate, and for the same reason — and AFTER that gate, so the step-once
        // one-shot is still reset on a granted frame.
        if (NOT WorldBlocksStep)
        {
            if (const auto GrantedSteps = JoltWorld->TryConsume_GrantedFixedSteps();
                GrantedSteps > 0)
            {
                const auto GrantedFixedHz = FMath::Max(1, UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz());
                const auto GrantedFixedDt = 1.0f / static_cast<float>(GrantedFixedHz);

                // All four fields, exactly as the ordinary plan writes them. The accumulator is CLEARED rather
                // than left standing: a granted batch is not a payment against banked real time, and carrying
                // that time forward would burst on the frame the hold releases. Alpha 0 renders at the pose the
                // grant just produced instead of interpolating toward one nothing is heading for.
                JoltWorld->Set_Accumulator(0.0f);
                JoltWorld->Set_Alpha(0.0f);
                JoltWorld->Set_NumStepsLastFrame(GrantedSteps);
                JoltWorld->Set_PendingSimTime(GrantedSteps * GrantedFixedDt);
                return;
            }
        }

        if (WorldBlocksStep || DebugPauseBlocksStep)
        {
            // The pending grant is NOT spent on a frame that steps nothing — it waits for the first frame that
            // runs — but this frame's consumed count must not be left standing, or the Step processor reads the
            // previous frame's grant as though it were this one's.
            JoltWorld->Set_GrantedStepsThisFrame(0);
            JoltWorld->Set_NumStepsLastFrame(0);
            JoltWorld->Set_PendingSimTime(0.0f);
            return;
        }

        // A GRANTED step-once is exactly one fixed step, not floor(accumulator / dt) of them: the user asked to
        // advance the sim by one step, and the accumulator holds whatever real time piled up while they were
        // reading the screen. It is left untouched, so resuming continues from where the pause began.
        if (JoltWorld->Get_StepOnceGrantedThisFrame())
        {
            const auto GrantedFixedHz = FMath::Max(1, UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz());
            const auto GrantedFixedDt = 1.0f / static_cast<float>(GrantedFixedHz);

            JoltWorld->Set_NumStepsLastFrame(1);
            JoltWorld->Set_PendingSimTime(GrantedFixedDt);
            return;
        }

        const auto Plan = ck::jolt::ComputeStepPlan(
            JoltWorld->Get_Accumulator(),
            static_cast<float>(InDeltaT.Get_Seconds()),
            UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz(),
            UCk_Utils_Jolt_ProjectSettings::Get_MaxPhysicsStepsPerFrame());

        if (Plan.DroppedTime > 0.0f)
        {
            ck::jolt::Verbose(TEXT("Jolt fixed-step: dropping [{}]s of accumulated physics time (spiral-of-death clamp)"),
                Plan.DroppedTime);
        }

        JoltWorld->Set_Accumulator(Plan.NewAccumulator);
        JoltWorld->Set_Alpha(Plan.Alpha);
        JoltWorld->Set_NumStepsLastFrame(Plan.NumSteps);
        JoltWorld->Set_PendingSimTime(Plan.PendingSimTime);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_JoltWorld_Step::
        FProcessor_JoltWorld_Step(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltWorld_Step::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck::jolt::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        // Paused skips the broadphase optimize too — PlanStep already zeroed the plan, but not this. The debug
        // pause is READ here, never consumed: PlanStep owns both one-shots, and this is the frame it granted.
        const auto World = JoltWorld->Get_World();
        const auto DebugPauseBlocksStep = JoltWorld->Get_IsDebugPaused() &&
            NOT JoltWorld->Get_StepOnceGrantedThisFrame() &&
            JoltWorld->Get_GrantedStepsThisFrame() == 0;

        if (ck::Is_NOT_Valid(World) || World->IsPaused() || DebugPauseBlocksStep)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkJolt_WorldStep);

        // Safe here: the async future was consumed upstream.
        if (JoltWorld->Get_OptimizeBroadPhaseRequested())
        { JoltWorld->DoOptimizeBroadPhase(); }

        const auto NumSteps = JoltWorld->Get_NumStepsLastFrame();
#if COUNTERSTRACE_ENABLED
        const auto TraceCountersEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(CountersChannel);
#else
        constexpr auto TraceCountersEnabled = false;
#endif
        // Sample census data after the prior async step has been consumed and before dispatching the next one. In
        // particular, Get_NumBodies_AnyThread takes Jolt's body-list mutex and must not perturb the async span.
        const auto TotalBodies = TraceCountersEnabled ? JoltWorld->Get_NumBodies_AnyThread() : 0;
        const auto RegisteredCharacters = TraceCountersEnabled
            ? JoltWorld->Get_NumRegisteredCharacters_AnyThread()
            : 0;

        if (NumSteps == 0)
        {
            if (TraceCountersEnabled)
            {
                TRACE_COUNTER_SET_ALWAYS(CkJolt_FixedSteps, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_TotalBodies, TotalBodies);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_ActiveRigidBodySamplesSum, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_MaxActiveRigidBodies, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_ActiveSoftBodySamplesSum, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_MaxActiveSoftBodies, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_RegisteredCharacters, RegisteredCharacters);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_TouchingManifoldCallbacksPerBatch, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_MaxTouchingManifoldCallbacksPerStep, 0);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_UpdateErrorBits, 0);
            }

            return;
        }

        // Recomputed from the same project setting PlanStep read — constant within the frame, so no drift.
        const auto FixedHz = FMath::Max(1, UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz());
        const auto FixedDt = 1.0f / static_cast<float>(FixedHz);

        // Characters advance BEFORE the rigid-body Update within each sub-step.
        //
        // The measured span is the frame's SOLVE — every DoPhysicsUpdate this frame runs, character stepping and
        // pose capture excluded — because that is the number a stats panel means by "step time". It is taken
        // inside the loop rather than around the dispatch so the async branch measures the task-graph thread's
        // own work instead of the cost of handing it off.
        const auto StepLoop =
            [JoltWorld, FixedDt, NumSteps, TraceCountersEnabled, TotalBodies, RegisteredCharacters]()
        {
            auto UpdateSeconds = 0.0;
            auto ActiveRigidBodiesTotal = int32{0};
            auto MaxActiveRigidBodies = int32{0};
            auto ActiveSoftBodiesTotal = int32{0};
            auto MaxActiveSoftBodies = int32{0};
            auto TouchingManifoldCallbacksPerBatch = int32{0};
            auto MaxTouchingManifoldCallbacksPerStep = int32{0};
            auto UpdateErrorBits = uint32{0};

            for (auto Step = 0; Step < NumSteps; ++Step)
            {
                {
                    SCOPE_CYCLE_COUNTER(STAT_CkJolt_StepCharacters);
                    JoltWorld->DoStepCharacters_AnyThread(FixedDt);
                }

                if (TraceCountersEnabled)
                {
                    const auto ActiveRigidBodies = JoltWorld->Get_NumActiveRigidBodies_AnyThread();
                    ActiveRigidBodiesTotal += ActiveRigidBodies;
                    MaxActiveRigidBodies = FMath::Max(MaxActiveRigidBodies, ActiveRigidBodies);

                    const auto ActiveSoftBodies = JoltWorld->Get_NumActiveSoftBodies_AnyThread();
                    ActiveSoftBodiesTotal += ActiveSoftBodies;
                    MaxActiveSoftBodies = FMath::Max(MaxActiveSoftBodies, ActiveSoftBodies);
                }

                const auto UpdateStartSeconds = FPlatformTime::Seconds();
                {
                    SCOPE_CYCLE_COUNTER(STAT_CkJolt_StepPhysicsSystemUpdate);
                    UpdateErrorBits |= JoltWorld->DoPhysicsUpdate(FixedDt);
                }
                UpdateSeconds += FPlatformTime::Seconds() - UpdateStartSeconds;

                if (TraceCountersEnabled)
                {
                    const auto TouchingManifoldCallbacksThisStep = JoltWorld->Get_ContactPairsLastStep();
                    TouchingManifoldCallbacksPerBatch += TouchingManifoldCallbacksThisStep;
                    MaxTouchingManifoldCallbacksPerStep = FMath::Max(
                        MaxTouchingManifoldCallbacksPerStep,
                        TouchingManifoldCallbacksThisStep);
                }

                {
                    SCOPE_CYCLE_COUNTER(STAT_CkJolt_StepPoseCapture);
                    JoltWorld->DoCapturePoses_AnyThread();
                }
            }

            if (TraceCountersEnabled)
            {
                TRACE_COUNTER_SET_ALWAYS(CkJolt_FixedSteps, NumSteps);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_TotalBodies, TotalBodies);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_ActiveRigidBodySamplesSum, ActiveRigidBodiesTotal);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_MaxActiveRigidBodies, MaxActiveRigidBodies);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_ActiveSoftBodySamplesSum, ActiveSoftBodiesTotal);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_MaxActiveSoftBodies, MaxActiveSoftBodies);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_RegisteredCharacters, RegisteredCharacters);
                TRACE_COUNTER_SET_ALWAYS(
                    CkJolt_TouchingManifoldCallbacksPerBatch,
                    TouchingManifoldCallbacksPerBatch);
                TRACE_COUNTER_SET_ALWAYS(
                    CkJolt_MaxTouchingManifoldCallbacksPerStep,
                    MaxTouchingManifoldCallbacksPerStep);
                TRACE_COUNTER_SET_ALWAYS(CkJolt_UpdateErrorBits, static_cast<int64>(UpdateErrorBits));
            }

            constexpr auto MillisecondsPerSecond = 1000.0;
            JoltWorld->Set_LastStepDurationMs(static_cast<float>(UpdateSeconds * MillisecondsPerSecond));
        };

        if (JoltWorld->Get_AsyncMode())
        {
            JoltWorld->Set_PendingAsyncStep(Async(EAsyncExecution::TaskGraph, [StepLoop]()
            {
                SCOPE_CYCLE_COUNTER(STAT_CkJolt_UpdateAsync);
                StepLoop();
            }));
        }
        else
        {
            SCOPE_CYCLE_COUNTER(STAT_CkJolt_Update);
            StepLoop();
            JoltWorld->DoApplyPoseBuffer_GameThread(_TransientEntity);
            JoltWorld->DoApplyCharacterPoses_GameThread(_TransientEntity);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
