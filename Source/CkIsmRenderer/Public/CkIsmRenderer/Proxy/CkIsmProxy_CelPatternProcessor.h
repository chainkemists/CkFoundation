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

// --------------------------------------------------------------------------------------------------------------------
// Entity cel patterns for ISM proxies — see CkIsmRenderer/CLAUDE.md and CkUsf/Claude.md § Cel shade (Stylize).
// Structurally the outline processors' twin (same shadow-ISM mechanism), with two deliberate differences: the
// shadow is keyed on the stencil VALUE rather than a preset — the cel contract is a direct value, so nothing
// is allocated and nothing is released — and an outline arriving on a patterned proxy tears the cel shadow
// DOWN rather than merely dropping the applied-state, because here the two features own separate components
// and leaving both alive would put two custom-depth writers on the same pixels.
//
// Every teardown path removes the applied-state with Try_Remove: the views below are not all disjoint (a
// DISABLED proxy whose target was cleared matches both _Suspend and _Remove) and their order within the
// group is unspecified. The teardown itself is idempotent — the second pass finds the instance id already
// invalid.

namespace ck
{
    // Runs per-frame rather than on a dirty tag so it self-heals ordering races (pattern requested before the
    // proxy finished setup, proxy re-enabled, world stencil base changed).
    class CKISMRENDERER_API FProcessor_IsmProxy_CelPattern_Sync : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CelPattern_Sync,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_Usf_CelPatternTarget>,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadOnly<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_Transform>,
        TExclude<FFragment_Usf_OutlineTarget>,
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
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternTarget& InTarget,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Deliberately NOT keyed on FTag_Transform_Updated — that dirty marker is shared with writers in modules
    // we cannot declare ordering against without a dependency cycle. Same rationale the outline twin carries;
    // see CkIsmRenderer/CLAUDE.md.
    class CKISMRENDERER_API FProcessor_IsmProxy_CelPattern_TransformSync : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CelPattern_TransformSync,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_CelPatternApplied>,
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
            const FFragment_IsmProxy_CelPatternApplied& InApplied,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform) -> void;

    private:
        TSet<UInstancedStaticMeshComponent*> _ShadowIsms;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The CelPatternTarget survives a suspend, so re-enabling the proxy re-applies via Sync.
    class CKISMRENDERER_API FProcessor_IsmProxy_CelPattern_Suspend : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CelPattern_Suspend,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_CelPatternApplied>,
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
            const FFragment_IsmProxy_CelPatternApplied& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // An outline arrived on a proxy this feature had already patterned; the outline wins. Unlike the actor
    // path — where the two features share one primitive's stencil byte and the outline simply overwrites it —
    // an ISM cel pattern owns its OWN shadow component, so leaving it alive would keep writing custom depth
    // alongside the outline's shadow at the same pixels. Tear it down; Sync excludes outline targets, so it
    // stays down until the outline is removed and then re-applies.
    class CKISMRENDERER_API FProcessor_IsmProxy_CelPattern_DropAppliedOnOutline : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CelPattern_DropAppliedOnOutline,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_CelPatternApplied>,
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
            const FFragment_IsmProxy_CelPatternApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Outline targets are excluded so this never overlaps the drop-processor above — the two views must stay
    // disjoint, or an entity matching both would tear the same shadow instance down twice.
    class CKISMRENDERER_API FProcessor_IsmProxy_CelPattern_Remove : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CelPattern_Remove,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_CelPatternApplied>,
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
            const FFragment_IsmProxy_CelPatternApplied& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_CelPattern_EndPlay : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CelPattern_EndPlay,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_CelPatternApplied>,
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
            const FFragment_IsmProxy_CelPatternApplied& InApplied) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
