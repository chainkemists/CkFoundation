#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"
#include "CkIsmRenderer/Proxy/CkIsmProxy_Processor.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Entity stylize effect mask for ISM proxies — see CkIsmRenderer/CLAUDE.md and CkUsf/Claude.md § Stylize.
// The cel-pattern processors' twin: same shadow-ISM mechanism, same direct-stencil contract (nothing is
// allocated, nothing is released), one precedence level lower. The mask is the LAST of the three claims on
// the Custom-Stencil byte, so every view here excludes outline AND cel-pattern targets.
//
// The stencil comes from project config rather than a per-world subsystem: there is exactly ONE mask value
// per project, so nothing world-local narrows it and the Sync processor needs no subsystem lookup for it.
//
// TWO drop processors rather than one, because a processor view is a conjunction while "a higher claim
// arrived" is a disjunction (outline OR cel pattern). Both bodies tear down and then Try_Remove, so on a
// proxy carrying BOTH higher claims the first to run drains it and the second no longer matches the view
// at all — the applied-state it keys on is gone.
//
// Every teardown path removes the applied-state with Try_Remove: the views below are not all disjoint (a
// DISABLED proxy whose target was cleared matches both _Suspend and _Remove) and their order within the
// group is unspecified. Try_Remove is what makes the loser of that race a no-op rather than an ensure.

namespace ck
{
    // Runs per-frame rather than on a dirty tag so it self-heals ordering races (the mask requested before
    // the proxy finished setup, proxy re-enabled, the project's mask value changed).
    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_Sync : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_Sync,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_Usf_StylizeMaskTarget>,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadOnly<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_Transform>,
        TExclude<FFragment_Usf_OutlineTarget>,
        TExclude<FFragment_Usf_CelPatternTarget>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_NeedsInstanceAdded>,
        TExclude<FTag_IsmProxy_Disabled>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_IsmProxy_AddInstance>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        // The world is still resolved once per tick — unlike the cel twin it is not needed for the stencil,
        // but the shadow-ISM factory lives on the renderer's world subsystem.
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskTarget& InTarget,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Deliberately NOT keyed on FTag_Transform_Updated — that dirty marker is shared with writers in modules
    // we cannot declare ordering against without a dependency cycle. Same rationale the outline and
    // cel-pattern twins carry; see CkIsmRenderer/CLAUDE.md.
    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_TransformSync : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_TransformSync,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_StylizeMaskApplied>,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadOnly<FFragment_Transform>,
        TExclude<FTag_IsmProxy_Disabled>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_NeedsInstanceAdded>,
        TIgnoreInEditor<FTag_IsmProxy_Movable>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance>;

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
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform) -> void;

    private:
        TSet<UInstancedStaticMeshComponent*> _ShadowIsms;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The StylizeMaskTarget survives a suspend, so re-enabling the proxy re-applies via Sync.
    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_Suspend : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_Suspend,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_StylizeMaskApplied>,
        FTag_IsmProxy_Disabled,
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
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // An outline arrived on a proxy this feature had already masked; the outline wins. Unlike the actor path
    // — where the claims share one primitive's stencil byte and the winner simply overwrites it — an ISM
    // mask owns its OWN shadow component, so leaving it alive would keep writing custom depth alongside the
    // outline's shadow at the same pixels. Tear it down; Sync excludes outline targets, so it stays down
    // until the outline is removed and then re-applies.
    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_DropAppliedOnOutline : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_DropAppliedOnOutline,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_StylizeMaskApplied>,
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
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_DropAppliedOnCelPattern : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_DropAppliedOnCelPattern,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_StylizeMaskApplied>,
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
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_CelPatternTarget& InCelPatternTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Both higher-precedence targets are excluded so this never overlaps the drop-processors above — the
    // views must stay disjoint, or a proxy matching two of them would tear the same shadow instance down
    // twice.
    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_Remove : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_Remove,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_StylizeMaskApplied>,
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
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_StylizeMask_EndPlay : public ck_exp::TProcessor<
        FProcessor_IsmProxy_StylizeMask_EndPlay,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_StylizeMaskApplied>,
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
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
