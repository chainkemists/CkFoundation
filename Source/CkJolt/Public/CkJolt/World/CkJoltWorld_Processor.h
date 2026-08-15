#pragma once

#include "CkJolt/World/CkJoltWorld.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /*
     * The one way a processor reaches the Jolt world. An absent context is LEGAL — a world with no Jolt subsystem
     * never publishes one — so a null answer is the correct silent path, not an error.
     */
    CKJOLT_API auto TryResolve_JoltWorld(
        const FCk_Handle& InTransientEntity) -> ck::FJoltWorld*;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward-declared for FProcessor_JoltWorld_Step's RunAfter edge only: the scheduler resolves
    // dependencies by canonical type name (entt::type_name), which needs the declaration, not the
    // definition — and including their headers here would be circular.
    class FProcessor_JoltBody_KinematicPush;
    class FProcessor_JoltCharacter_PreStep;

    // --------------------------------------------------------------------------------------------------------------------

    // Consumes the previous frame's async physics step and applies its buffered poses. Anchored after
    // Transform request handling so this frame's transform writes are settled before physics reads them.
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

    // Drains the last step's contact queue to the registered routers. Runs even while paused.
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

    // Fixed-timestep PLANNER: accumulates real delta, clamps the spiral-of-death budget, and writes this
    // frame's NumSteps / Alpha / PendingSimTime onto the FJoltWorld. Runs before the JoltBody kinematic
    // push so KinematicPush can read PendingSimTime. Owns the world-invalid / paused gate.
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

    // Fixed-timestep EXECUTOR: optimizes the broadphase if requested, then runs the planned N sub-steps
    // (Update + pose capture) — sync applies the poses immediately, async dispatches the batch to the task
    // graph for FProcessor_JoltWorld_WaitForAsync to consume next frame.
    class CKJOLT_API FProcessor_JoltWorld_Step : public TProcessorBase<FProcessor_JoltWorld_Step>
    {
    public:
        using Group = FGroup_Transform;
        // After BOTH, so this frame's ECS-driven kinematic targets and character intents ride the same step.
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
