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

        // 1. World invalid or paused -> return. Drain already ran this frame, matching the old Tick's IsPaused position.
        const auto World = JoltWorld->Get_World();
        if (ck::Is_NOT_Valid(World) || World->IsPaused())
        { return; }

        // 2. Broadphase optimize if a bulk add/remove requested it. Safe: the async future was consumed upstream.
        if (JoltWorld->Get_OptimizeBroadPhaseRequested())
        { JoltWorld->DoOptimizeBroadPhase(); }

        // 3. Fixed-timestep accumulation. NO ensure in this clamp path -- a spiral-of-death under load is expected.
        const auto FixedHz = FMath::Max(1, UCk_Utils_Jolt_ProjectSettings::Get_FixedTimestepHz());
        const auto FixedDt = 1.0f / static_cast<float>(FixedHz);
        const auto MaxSteps = UCk_Utils_Jolt_ProjectSettings::Get_MaxPhysicsStepsPerFrame();

        JoltWorld->Set_Accumulator(JoltWorld->Get_Accumulator() + static_cast<float>(InDeltaT.Get_Seconds()));

        const auto MaxAccum = static_cast<float>(MaxSteps) * FixedDt;
        if (JoltWorld->Get_Accumulator() > MaxAccum)
        {
            ck::jolt::Verbose(TEXT("Jolt fixed-step: dropping [{}]s of accumulated physics time (spiral-of-death clamp)"),
                JoltWorld->Get_Accumulator() - MaxAccum);
            JoltWorld->Set_Accumulator(MaxAccum);
        }

        const auto NumSteps = FMath::FloorToInt(JoltWorld->Get_Accumulator() / FixedDt);
        JoltWorld->Set_Accumulator(JoltWorld->Get_Accumulator() - static_cast<float>(NumSteps) * FixedDt);
        JoltWorld->Set_Alpha(JoltWorld->Get_Accumulator() / FixedDt);
        JoltWorld->Set_NumStepsLastFrame(NumSteps);
        JoltWorld->Set_PendingSimTime(static_cast<float>(NumSteps) * FixedDt);

        // 4. Zero-step frame: alpha keeps growing; nothing else runs.
        if (NumSteps == 0)
        { return; }

        // 5. Run the step batch: N fixed sub-steps, each an Update followed by a pose capture.
        const auto StepLoop = [JoltWorld, FixedDt, NumSteps]()
        {
            for (auto Step = 0; Step < NumSteps; ++Step)
            {
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
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
