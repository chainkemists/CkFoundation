#pragma once

#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward-declared for the RunAfter edge only: the scheduler resolves dependencies by canonical type name,
    // which needs the declaration, not the definition.
    class FProcessor_JoltBody_SleepStateMirror;

    // --------------------------------------------------------------------------------------------------------------------

    /*
     * Pumps every registered, demanding FCk_Jolt_DebugDrawTarget from the live Jolt world. This is the ONLY
     * producer of debug-draw target contents — a consumer (a Slate debugger, a preview viewport) never touches
     * JPH::PhysicsSystem itself, so it can never race the step.
     *
     * The window between WaitForAsync (previous frame's async step consumed) and Step (this frame's kicked) is
     * the only point where Jolt state is stable in BOTH sync and async physics modes. SleepStateMirror runs
     * first so the frame's activation events are already reflected. No demanding target = immediate early-out.
     */
    class CKJOLT_API FProcessor_JoltDebugDraw_Capture : public TProcessorBase<FProcessor_JoltDebugDraw_Capture>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync, FProcessor_JoltBody_SleepStateMirror>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltDebugDraw_Capture(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
