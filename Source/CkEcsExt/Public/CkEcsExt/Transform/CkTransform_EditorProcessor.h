#pragma once

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

namespace ck
{
    // Editor-only transform gizmo. Ghost in the runtime graph.
    class CKECSEXT_API FProcessor_Transform_Preview_EditorTime : public ck_exp::TProcessor<
            FProcessor_Transform_Preview_EditorTime,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_Transform>,
            CK_IF_EDITOR_ONLY_ENTITY,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::EditorOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform) -> void;
    };
}

#endif

// --------------------------------------------------------------------------------------------------------------------
