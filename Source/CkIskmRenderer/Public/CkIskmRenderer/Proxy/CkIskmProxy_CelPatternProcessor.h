#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Entity cel patterns for ISKM Plan-1 proxies (see CkUsf/Claude.md § Cel shade (Stylize)). The outline
// processors' twin: flags are re-asserted per frame (the engine setters early-out when unchanged) so
// submeshes attached after the request inherit the pattern. Nothing is allocated — the cel contract is a
// direct stencil value — so teardown is just clearing the flags. Plan-2 crowd members are not entities and
// have no cel-pattern path; see CkIskmRenderer/CLAUDE.md.

namespace ck
{
    class CKISKMRENDERER_API FProcessor_IskmProxy_CelPattern_Sync : public ck_exp::TProcessor<
        FProcessor_IskmProxy_CelPattern_Sync,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_Usf_CelPatternTarget>,
        TReadOnly<FFragment_IskmProxy_Current>,
        TExclude<FFragment_Usf_OutlineTarget>,
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
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternTarget& InTarget,
            const FFragment_IskmProxy_Current& InCurrent) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // An outline arrived on a proxy this feature had already patterned; the outline wins. The flags are
    // deliberately NOT cleared — here both features write the SAME SKMCs, and the outline's own Sync
    // overwrites the stencil in the same group, so clearing could blank its silhouette for a frame. Only the
    // now-false applied-state is dropped, which is what lets Sync re-apply once the outline is removed.
    class CKISKMRENDERER_API FProcessor_IskmProxy_CelPattern_DropAppliedOnOutline : public ck_exp::TProcessor<
        FProcessor_IskmProxy_CelPattern_DropAppliedOnOutline,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_CelPatternApplied>,
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
            const FFragment_IskmProxy_CelPatternApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Outline targets are excluded so this never overlaps the drop-processor above: clearing custom depth on
    // an outlined proxy would erase the outline's silhouette.
    class CKISKMRENDERER_API FProcessor_IskmProxy_CelPattern_Remove : public ck_exp::TProcessor<
        FProcessor_IskmProxy_CelPattern_Remove,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_CelPatternApplied>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_IskmProxy_CelPatternApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Clears the flags while the proxy still owns its SKMC (the proxy EndPlay releases it to the pool AFTER
    // this — it declares RunAfter on us). Release_BaseSKMC strips custom depth again defensively.
    class CKISKMRENDERER_API FProcessor_IskmProxy_CelPattern_EndPlay : public ck_exp::TProcessor<
        FProcessor_IskmProxy_CelPattern_EndPlay,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_CelPatternApplied>,
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
            const FFragment_IskmProxy_CelPatternApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
