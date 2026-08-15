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

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_WaitForAsync);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_DrainEvents);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_PlanStep);
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltWorld_Step);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltPhysics_WaitForAsync"), STAT_CkJolt_WaitForAsync, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_DrainQueue"), STAT_CkJolt_ContactsDrainQueue, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_Update_Async"), STAT_CkJolt_UpdateAsync, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_Update"), STAT_CkJolt_Update, STATGROUP_CkJolt);

// The whole fixed-step pump; wraps the nested STAT_CkJolt_Update/_UpdateAsync (the Update loop alone).
DECLARE_CYCLE_STAT(TEXT("JoltWorld_Step"), STAT_CkJolt_WorldStep, STATGROUP_CkJolt);

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

        if (WorldBlocksStep || DebugPauseBlocksStep)
        {
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
        // pause is READ here, never consumed: PlanStep owns the one-shot, and this is the frame it granted.
        const auto World = JoltWorld->Get_World();
        const auto DebugPauseBlocksStep = JoltWorld->Get_IsDebugPaused() &&
            NOT JoltWorld->Get_StepOnceGrantedThisFrame();

        if (ck::Is_NOT_Valid(World) || World->IsPaused() || DebugPauseBlocksStep)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkJolt_WorldStep);

        // Safe here: the async future was consumed upstream.
        if (JoltWorld->Get_OptimizeBroadPhaseRequested())
        { JoltWorld->DoOptimizeBroadPhase(); }

        const auto NumSteps = JoltWorld->Get_NumStepsLastFrame();

        if (NumSteps == 0)
        { return; }

        // Recomputed from the same project setting PlanStep read — constant within the frame, so no drift.
        const auto FixedHz = FMath::Max(1, UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz());
        const auto FixedDt = 1.0f / static_cast<float>(FixedHz);

        // Characters advance BEFORE the rigid-body Update within each sub-step.
        //
        // The measured span is the frame's SOLVE — every DoPhysicsUpdate this frame runs, character stepping and
        // pose capture excluded — because that is the number a stats panel means by "step time". It is taken
        // inside the loop rather than around the dispatch so the async branch measures the task-graph thread's
        // own work instead of the cost of handing it off.
        const auto StepLoop = [JoltWorld, FixedDt, NumSteps]()
        {
            auto UpdateSeconds = 0.0;

            for (auto Step = 0; Step < NumSteps; ++Step)
            {
                JoltWorld->DoStepCharacters_AnyThread(FixedDt);

                const auto UpdateStartSeconds = FPlatformTime::Seconds();
                JoltWorld->DoPhysicsUpdate(FixedDt);
                UpdateSeconds += FPlatformTime::Seconds() - UpdateStartSeconds;

                JoltWorld->DoCapturePoses_AnyThread();
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
