#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Entity stylize effect mask for ISKM Plan-1 proxies (see CkUsf/Claude.md § Stylize). The cel-pattern
// processors' twin: flags are re-asserted per frame (the engine setters early-out when unchanged) so
// submeshes attached after the request inherit the mask, and nothing is allocated — the mask is a direct
// stencil value read from project config, so teardown is just clearing the flags.
//
// One precedence level below the cel pattern: the mask is the LAST of the three claims on the
// Custom-Stencil byte, so every view here excludes outline AND cel-pattern targets, and there are TWO drop
// processors rather than one — a processor view is a conjunction while "a higher claim arrived" is a
// disjunction. Both drop bodies are the same idempotent Try_Remove.
//
// Plan-2 crowd members are not entities; they use the member-indexed API on ACk_Iskm_BatchedCrowd_Actor.

namespace ck
{
    class CKISKMRENDERER_API FProcessor_IskmProxy_StylizeMask_Sync : public ck_exp::TProcessor<
        FProcessor_IskmProxy_StylizeMask_Sync,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_Usf_StylizeMaskTarget>,
        TReadOnly<FFragment_IskmProxy_Current>,
        TExclude<FFragment_Usf_OutlineTarget>,
        TExclude<FFragment_Usf_CelPatternTarget>,
        TExclude<FTag_IskmProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        // Submeshes attach in HandleRequests — run after it so a same-frame attach gets flagged same-frame.
        using RunAfter = TDepList<FProcessor_IskmProxy_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        // No cached world, unlike the cel twin: the mask stencil is project config, so there is no subsystem
        // to resolve per tick.
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskTarget& InTarget,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // An outline arrived on a proxy this feature had already masked; the outline wins. The flags are
    // deliberately NOT cleared — here every claim writes the SAME SKMCs, and the outline's own Sync
    // overwrites the stencil in the same group, so clearing could blank its silhouette for a frame. Only the
    // now-false applied-state is dropped, which is what lets Sync re-apply once the outline is removed.
    class CKISKMRENDERER_API FProcessor_IskmProxy_StylizeMask_DropAppliedOnOutline : public ck_exp::TProcessor<
        FProcessor_IskmProxy_StylizeMask_DropAppliedOnOutline,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_StylizeMaskApplied>,
        TReadOnly<FFragment_Usf_OutlineTarget>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISKMRENDERER_API FProcessor_IskmProxy_StylizeMask_DropAppliedOnCelPattern : public ck_exp::TProcessor<
        FProcessor_IskmProxy_StylizeMask_DropAppliedOnCelPattern,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_StylizeMaskApplied>,
        TReadOnly<FFragment_Usf_CelPatternTarget>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_CelPatternTarget& InCelPatternTarget,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Both higher-precedence targets are excluded so this never overlaps the drop-processors above: clearing
    // custom depth on an outlined or patterned proxy would erase that feature's silhouette.
    class CKISKMRENDERER_API FProcessor_IskmProxy_StylizeMask_Remove : public ck_exp::TProcessor<
        FProcessor_IskmProxy_StylizeMask_Remove,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_StylizeMaskApplied>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Clears the flags while the proxy still owns its SKMC (the proxy EndPlay releases it to the pool AFTER
    // this — it declares RunAfter on us). Release_BaseSKMC strips custom depth again defensively.
    class CKISKMRENDERER_API FProcessor_IskmProxy_StylizeMask_EndPlay : public ck_exp::TProcessor<
        FProcessor_IskmProxy_StylizeMask_EndPlay,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_StylizeMaskApplied>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_IskmProxy_StylizeMaskApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
