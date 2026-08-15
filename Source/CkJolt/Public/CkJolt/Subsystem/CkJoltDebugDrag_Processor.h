#pragma once

#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /*
     * Drains the debug-drag request queue onto the Jolt world (P6-D47). The requests come from a debugger click, so
     * they arrive on the Slate tick — this processor is what moves their effect into the one window where mutating
     * the simulation is safe: after the previous frame's async step was consumed, and BEFORE this frame's is kicked.
     *
     * After KinematicPush for the same reason FProcessor_JoltWorld_Step is: the anchor is a kinematic body and the
     * kinematic writer owns that pass, so the drag must not land in the middle of it.
     *
     * DEV-ONLY and SIM-MUTATING. It is the only processor in the debug facility that changes what the simulation
     * does, and it belongs to the authority alone — see CkJolt/CLAUDE.md § "Debug drag".
     */
    class CKJOLT_API FProcessor_JoltDebugDrag_Apply : public TProcessorBase<FProcessor_JoltDebugDrag_Apply>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync, FProcessor_JoltBody_KinematicPush>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltDebugDrag_Apply(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
