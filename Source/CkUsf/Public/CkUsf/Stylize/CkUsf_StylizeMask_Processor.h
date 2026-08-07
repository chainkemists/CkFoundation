#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Actor-backed entities: write the project's stylize-mask Custom-Stencil value onto the entity's
    // owning actor. Per-frame over masked entities only (view = StylizeMaskTarget + OwningActor) — cost
    // scales with the masked count.
    //
    // Outline AND cel-pattern targets are EXCLUDED because both own the same byte and both outrank the
    // mask. Request_AddToStylizeMask rejects the other ordering outright, so this exclusion only covers
    // one of those arriving afterwards; that feature's own sync processor then takes the value over.
    class CKUSF_API FProcessor_Usf_StylizeMaskActor_Sync : public TProcessor<
        FProcessor_Usf_StylizeMaskActor_Sync,
        TReadOnly<FFragment_Usf_StylizeMaskTarget>,
        TReadOnly<FFragment_OwningActor_Current>,
        TExclude<FFragment_Usf_OutlineTarget>,
        TExclude<FFragment_Usf_CelPatternTarget>,
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
            const FFragment_Usf_StylizeMaskTarget& InTarget,
            const FFragment_OwningActor_Current& InOwningActor) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // A higher-precedence claim arrived on an entity this feature had already masked. It now owns the
    // Custom-Stencil byte, so the applied-state is a LIE and must be dropped — without this the entity
    // keeps a cache saying "already masked", and when the outline or pattern is removed again the sync
    // processor early-outs on it and the mask never comes back. The byte is deliberately NOT cleared
    // here: it belongs to the other feature now, and all three share the actor's ONE primitive stencil.
    //
    // TWO processors rather than one, because a processor view is a conjunction and the condition is a
    // disjunction (outline OR cel pattern). Both bodies undo and then Try_Remove, so on an entity
    // carrying BOTH higher claims the first to run drains it and the second no longer matches the view
    // at all — the applied-state it keys on is gone.
    class CKUSF_API FProcessor_Usf_StylizeMaskActor_DropAppliedOnOutline : public TProcessor<
        FProcessor_Usf_StylizeMaskActor_DropAppliedOnOutline,
        TReadOnly<FFragment_Usf_StylizeMaskApplied_Actor>,
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
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUSF_API FProcessor_Usf_StylizeMaskActor_DropAppliedOnCelPattern : public TProcessor<
        FProcessor_Usf_StylizeMaskActor_DropAppliedOnCelPattern,
        TReadOnly<FFragment_Usf_StylizeMaskApplied_Actor>,
        TReadOnly<FFragment_Usf_CelPatternTarget>,
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
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied,
            const FFragment_Usf_CelPatternTarget& InCelPatternTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Undo when the target fragment is gone but the actor-path stencil is still written. Higher-precedence
    // targets are excluded so this never overlaps the drop-processor above: their applied-state is that
    // one's business, and disabling custom depth here would erase their silhouette.
    class CKUSF_API FProcessor_Usf_StylizeMaskActor_Remove : public TProcessor<
        FProcessor_Usf_StylizeMaskActor_Remove,
        TReadOnly<FFragment_Usf_StylizeMaskApplied_Actor>,
        TExclude<FFragment_Usf_StylizeMaskTarget>,
        TExclude<FFragment_Usf_OutlineTarget>,
        TExclude<FFragment_Usf_CelPatternTarget>,
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
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUSF_API FProcessor_Usf_StylizeMaskActor_EndPlay : public TProcessor<
        FProcessor_Usf_StylizeMaskActor_EndPlay,
        TReadOnly<FFragment_Usf_StylizeMaskApplied_Actor>,
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
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
