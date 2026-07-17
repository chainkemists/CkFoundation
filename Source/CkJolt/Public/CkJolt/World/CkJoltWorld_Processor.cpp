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

// Moved from CkJolt_Subsystem.cpp when the step relocated into these processors.
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_WaitForAsync"), STAT_CkJolt_WaitForAsync, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_DrainQueue"), STAT_CkJolt_ContactsDrainQueue, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_Update_Async"), STAT_CkJolt_UpdateAsync, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltPhysics_Update"), STAT_CkJolt_Update, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_world_processor
{
    // Resolves the registry's Jolt-world context. A world with no Jolt subsystem never publishes it —
    // an absent/null context is legal, so callers return silently (correct silent path, not an error).
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
        auto* JoltWorld = ck_jolt_world_processor::TryResolve_JoltWorld(_TransientEntity);
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
        auto* JoltWorld = ck_jolt_world_processor::TryResolve_JoltWorld(_TransientEntity);
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
        auto* JoltWorld = ck_jolt_world_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        // World invalid or paused -> gate planning: freeze the accumulator and zero the plan so the executor
        // runs no sub-steps and KinematicPush (PendingSimTime <= 0) early-outs. Matches the pre-split Step's
        // IsPaused position (which returned before touching the accumulator or stepping).
        const auto World = JoltWorld->Get_World();
        if (ck::Is_NOT_Valid(World) || World->IsPaused())
        {
            JoltWorld->Set_NumStepsLastFrame(0);
            JoltWorld->Set_PendingSimTime(0.0f);
            return;
        }

        // Fixed-timestep accumulation — the math lives in ck::jolt::ComputeStepPlan (pure, test-pinned).
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
        auto* JoltWorld = ck_jolt_world_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        // 1. World invalid or paused -> return, exactly as the pre-split Step did: no broadphase optimize and
        //    no stepping while paused (PlanStep also zeroes the plan, but this guard preserves the optimize skip).
        const auto World = JoltWorld->Get_World();
        if (ck::Is_NOT_Valid(World) || World->IsPaused())
        { return; }

        // 2. Broadphase optimize if a bulk add/remove requested it. Safe: the async future was consumed upstream.
        if (JoltWorld->Get_OptimizeBroadPhaseRequested())
        { JoltWorld->DoOptimizeBroadPhase(); }

        // 3. Read the plan from the FJoltWorld (computed by FProcessor_JoltWorld_PlanStep this frame). FixedDt is
        //    recomputed from the same project setting PlanStep used -- constant within the frame, so no drift.
        const auto NumSteps = JoltWorld->Get_NumStepsLastFrame();

        // 4. Zero-step frame: alpha keeps growing; nothing else runs.
        if (NumSteps == 0)
        { return; }

        const auto FixedHz = FMath::Max(1, UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz());
        const auto FixedDt = 1.0f / static_cast<float>(FixedHz);

        // 5. Run the step batch: N fixed sub-steps. Each sub-step advances the characters (ExtendedUpdate)
        //    BEFORE the rigid-body world Update, then captures the body poses.
        const auto StepLoop = [JoltWorld, FixedDt, NumSteps]()
        {
            for (auto Step = 0; Step < NumSteps; ++Step)
            {
                JoltWorld->DoStepCharacters_AnyThread(FixedDt);
                JoltWorld->DoPhysicsUpdate(FixedDt);
                JoltWorld->DoCapturePoses_AnyThread();
            }
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
