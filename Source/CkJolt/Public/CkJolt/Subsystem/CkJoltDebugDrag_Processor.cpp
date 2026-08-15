#include "CkJoltDebugDrag_Processor.h"

#if !UE_BUILD_SHIPPING

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/World/CkJoltWorld_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltDebugDrag_Apply);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_JoltDebugDrag_Apply::
        FProcessor_JoltDebugDrag_Apply(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltDebugDrag_Apply::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto* JoltWorld = ck::jolt::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        // Unconditional: the drain is only half of what this does. It also re-reads the live drag, which is how a
        // body destroyed under one takes its constraint with it.
        JoltWorld->Apply_DragRequests();
    }
}

// --------------------------------------------------------------------------------------------------------------------

#endif
