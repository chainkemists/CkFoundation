#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Entity outlines for ISKM Plan-1 proxies (see CkUsf/DESIGN_EntityOutlines.md). Flags are re-asserted per
// frame (the engine setters early-out when unchanged) so submeshes attached after the outline inherit it.
// Plan-2 crowd members are not entities — their outline rides ACk_Iskm_BatchedCrowd_Actor's member API.

namespace ck
{
    class CKISKMRENDERER_API FProcessor_IskmProxy_Outline_Sync : public ck_exp::TProcessor<
        FProcessor_IskmProxy_Outline_Sync,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_Usf_OutlineTarget>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_Usf_OutlineTarget& InTarget,
            const FFragment_IskmProxy_Current& InCurrent) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISKMRENDERER_API FProcessor_IskmProxy_Outline_Remove : public ck_exp::TProcessor<
        FProcessor_IskmProxy_Outline_Remove,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_OutlineApplied>,
        TReadOnly<FFragment_IskmProxy_Current>,
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
            const FFragment_IskmProxy_OutlineApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Clears the flags while the proxy still owns its SKMC (the proxy EndPlay releases it to the pool
    // AFTER this — it declares RunAfter on us). Release_BaseSKMC strips custom depth again defensively.
    class CKISKMRENDERER_API FProcessor_IskmProxy_Outline_EndPlay : public ck_exp::TProcessor<
        FProcessor_IskmProxy_Outline_EndPlay,
        FCk_Handle_IskmProxy,
        TReadOnly<FFragment_IskmProxy_OutlineApplied>,
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
            const FFragment_IskmProxy_OutlineApplied& InApplied,
            const FFragment_IskmProxy_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
