#pragma once

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Draws a transform gizmo for every editor-only transform entity so designers can see where
    // entities live in the level viewport without entering PIE. View is filtered on
    // FTag_EditorOnlyEntity so this processor ghosts out of the runtime graph and never touches
    // non-editor entities even in editor worlds.
    class CKECSEXT_API FProcessor_Transform_Debug_EditorTime : public ck_exp::TProcessor<
            FProcessor_Transform_Debug_EditorTime,
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
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
