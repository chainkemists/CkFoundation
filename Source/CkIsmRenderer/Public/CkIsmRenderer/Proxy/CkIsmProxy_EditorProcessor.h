#pragma once

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Editor-only preview for IsmProxy entities. Renders a wireframe indicator at the entity's
    // transform so designers can see the bounds where instanced meshes would appear at runtime
    // without entering PIE. See Decision D8 in the plan — a full mesh preview is deferred.
    class CKISMRENDERER_API FProcessor_IsmProxy_Preview_EditorTime : public ck_exp::TProcessor<
            FProcessor_IsmProxy_Preview_EditorTime,
            FCk_Handle_IsmProxy,
            ck::TReadOnly<FFragment_IsmProxy_Params>,
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
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
