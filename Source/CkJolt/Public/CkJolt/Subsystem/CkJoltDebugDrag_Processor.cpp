#include "CkJoltDebugDrag_Processor.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

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
        // An absent context is legal — a world with no Jolt subsystem never publishes one.
        const auto* WorldPtr = _TransientEntity.Get_RegistryView().TryGetContext<TSharedPtr<FJoltWorld>>();

        if (WorldPtr == nullptr || NOT WorldPtr->IsValid())
        { return; }

        // Self-early-outs on an empty queue, which is every frame nobody is dragging.
        (*WorldPtr)->Apply_DragRequests();
    }
}

// --------------------------------------------------------------------------------------------------------------------
