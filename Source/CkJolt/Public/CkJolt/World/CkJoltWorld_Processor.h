#pragma once

#include "CkJolt/World/CkJoltWorld.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

    // Fixed-timestep pump: accumulates real delta, runs N fixed sub-steps (Update + pose capture), and in
    // sync mode applies the poses immediately; in async mode the step batch is dispatched to the task graph
    // and consumed by FProcessor_JoltWorld_WaitForAsync next frame.
    class CKJOLT_API FProcessor_JoltWorld_Step : public TProcessorBase<FProcessor_JoltWorld_Step>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_DrainEvents>;

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
