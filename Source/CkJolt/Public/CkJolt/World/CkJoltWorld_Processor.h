#pragma once

#include "CkJolt/World/CkJoltWorld.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Defined by the JoltBody / JoltCharacter feature quartets (Body/CkJoltBody_Processor.h,
    // Character/CkJoltCharacter_Processor.h). Forward-declared here for FProcessor_JoltWorld_Step's RunAfter
    // edge only — the scheduler resolves the dependency by canonical type name (entt::type_name), which needs
    // the declaration, not the definition. Including those headers here would be circular (they depend on
    // this one).
    class FProcessor_JoltBody_KinematicPush;
    class FProcessor_JoltCharacter_PreStep;

    // --------------------------------------------------------------------------------------------------------------------

    // Consumes the previous frame's async physics step (if any) and applies its buffered poses on the game
    // thread. First link of the Jolt step chain, anchored after Transform request handling so this frame's
    // transform writes are settled before physics reads them.
    class CKJOLT_API FProcessor_JoltWorld_WaitForAsync : public TProcessorBase<FProcessor_JoltWorld_WaitForAsync>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltWorld_WaitForAsync(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains the contact queue produced by the last step and routes it to registered consumers (Probe
    // overlap translation, etc.). Runs even while paused, matching the pre-split subsystem behavior.
    class CKJOLT_API FProcessor_JoltWorld_DrainEvents : public TProcessorBase<FProcessor_JoltWorld_DrainEvents>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltWorld_DrainEvents(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Fixed-timestep PLANNER: accumulates real delta, clamps the spiral-of-death budget, and computes this
    // frame's NumSteps / Alpha / PendingSimTime onto the FJoltWorld. Runs before the JoltBody kinematic
    // push so KinematicPush can read PendingSimTime. The world-invalid / paused gate lives here — planning
    // does not advance while paused, and the plan is zeroed so the executor (below) runs no sub-steps.
    class CKJOLT_API FProcessor_JoltWorld_PlanStep : public TProcessorBase<FProcessor_JoltWorld_PlanStep>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_DrainEvents>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltWorld_PlanStep(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Fixed-timestep EXECUTOR: reads the plan (NumSteps / FixedDt) from the FJoltWorld, optimizes the
    // broadphase if requested, then runs N fixed sub-steps (Update + pose capture) — in sync mode applying
    // the poses immediately; in async mode dispatching the step batch to the task graph, consumed by
    // FProcessor_JoltWorld_WaitForAsync next frame. Runs after the JoltBody kinematic push so ECS-driven
    // kinematic targets are staged into the same step.
    class CKJOLT_API FProcessor_JoltWorld_Step : public TProcessorBase<FProcessor_JoltWorld_Step>
    {
    public:
        using Group = FGroup_Transform;
        // After BOTH the JoltBody kinematic push AND the JoltCharacter pre-step, so this frame's ECS-driven
        // kinematic targets and character intents are staged into the same step.
        using RunAfter = TDepList<FProcessor_JoltBody_KinematicPush, FProcessor_JoltCharacter_PreStep>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltWorld_Step(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
