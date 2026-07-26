#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"
#include "CkIsmRenderer/Proxy/CkIsmProxy_Processor.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Entity outlines for ISM proxies — see CkIsmRenderer/CLAUDE.md and CkUsf/DESIGN_EntityOutlines.md.

namespace ck
{
    // Runs per-frame rather than on a dirty tag so it self-heals ordering races (outline requested
    // before the proxy finished setup, proxy re-enabled, preset swapped).
    class CKISMRENDERER_API FProcessor_IsmProxy_Outline_Sync : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Outline_Sync,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_Usf_OutlineTarget>,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadOnly<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_Transform>,
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
            const FFragment_Usf_OutlineTarget& InTarget,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Deliberately NOT keyed on FTag_Transform_Updated — that dirty marker is shared with writers in
    // modules we cannot declare ordering against without a dependency cycle. See CkIsmRenderer/CLAUDE.md.
    class CKISMRENDERER_API FProcessor_IsmProxy_Outline_TransformSync : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Outline_TransformSync,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_OutlineApplied>,
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
            const FFragment_IsmProxy_OutlineApplied& InApplied,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform) -> void;

    private:
        TSet<UInstancedStaticMeshComponent*> _ShadowIsms;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The OutlineTarget survives a suspend, so re-enabling the proxy re-applies via Sync.
    class CKISMRENDERER_API FProcessor_IsmProxy_Outline_Suspend : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Outline_Suspend,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_OutlineApplied>,
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
            const FFragment_IsmProxy_OutlineApplied& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_Outline_Remove : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Outline_Remove,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_OutlineApplied>,
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
            const FFragment_IsmProxy_OutlineApplied& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_Outline_EndPlay : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Outline_EndPlay,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_OutlineApplied>,
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
            const FFragment_IsmProxy_OutlineApplied& InApplied) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
