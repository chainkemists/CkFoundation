#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Actor-backed entities: write the cel pattern's Custom-Stencil value onto the entity's owning actor.
    // Per-frame over patterned entities only (view = CelPatternTarget + OwningActor) — cost scales with the
    // patterned count.
    //
    // Outline targets are EXCLUDED because they own the same Custom-Stencil byte. Request_SetCelPattern
    // rejects the other ordering outright, so this exclusion only covers an outline applied afterwards; the
    // outline's own sync processor then takes the value over.
    class CKUSF_API FProcessor_Usf_CelPatternActor_Sync : public TProcessor<
        FProcessor_Usf_CelPatternActor_Sync,
        TReadOnly<FFragment_Usf_CelPatternTarget>,
        TReadOnly<FFragment_OwningActor_Current>,
        TExclude<FFragment_Usf_OutlineTarget>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternTarget& InTarget,
            const FFragment_OwningActor_Current& InOwningActor) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // An outline arrived on an entity this feature had already patterned. The outline now owns the
    // Custom-Stencil byte, so the applied-state is a LIE and must be dropped — without this the entity
    // keeps a cache saying "already patterned", and when the outline is removed again the sync processor
    // early-outs on it and the pattern never comes back. The byte is deliberately NOT cleared here: it
    // belongs to the outline now.
    class CKUSF_API FProcessor_Usf_CelPatternActor_DropAppliedOnOutline : public TProcessor<
        FProcessor_Usf_CelPatternActor_DropAppliedOnOutline,
        TReadOnly<FFragment_Usf_CelPatternApplied_Actor>,
        TReadOnly<FFragment_Usf_OutlineTarget>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternApplied_Actor& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Undo when the target fragment is gone but the actor-path stencil is still written. Outline targets
    // are excluded so this never overlaps the drop-processor above: an outlined entity's applied-state is
    // that one's business, and disabling custom depth here would erase the outline's silhouette.
    class CKUSF_API FProcessor_Usf_CelPatternActor_Remove : public TProcessor<
        FProcessor_Usf_CelPatternActor_Remove,
        TReadOnly<FFragment_Usf_CelPatternApplied_Actor>,
        TExclude<FFragment_Usf_CelPatternTarget>,
        TExclude<FFragment_Usf_OutlineTarget>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternApplied_Actor& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUSF_API FProcessor_Usf_CelPatternActor_EndPlay : public TProcessor<
        FProcessor_Usf_CelPatternActor_EndPlay,
        TReadOnly<FFragment_Usf_CelPatternApplied_Actor>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternApplied_Actor& InApplied) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
