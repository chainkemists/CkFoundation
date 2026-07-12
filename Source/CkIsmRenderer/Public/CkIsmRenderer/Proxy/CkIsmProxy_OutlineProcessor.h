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
// Entity outlines for ISM proxies (see CkUsf/DESIGN_EntityOutlines.md). Custom depth is per-component, so an
// outlined proxy's instance is mirrored into a custom-depth-only "shadow ISM" (one per renderer+preset, owned
// by UCk_IsmRenderer_Subsystem_UE). All processors here iterate outlined entities only.

namespace ck
{
    // Apply / re-apply on preset drift. Per-frame over (OutlineTarget + IsmProxy) — self-heals ordering
    // races (outline requested before the proxy finished setup, proxy re-enabled, etc.).
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
        // Refreshed every frame
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Keep the shadow instance glued to the entity transform (movable proxies). Mirrors
    // FProcessor_IsmProxy_TransformInstance's push, incl. the batched MarkRenderStateDirty.
    // Deliberately NOT keyed on FTag_Transform_Updated: that dirty marker is shared by writers in other
    // modules (CkRaySense) we can't declare ordering against without a dependency cycle — the scheduler's
    // conflict advisory would fire on every world. Per-frame over outlined MOVABLE proxies only (a tiny
    // set), and UpdateInstanceTransformById on an unchanged transform is cheap.
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

    // Proxy disabled while outlined: pull the shadow instance too. The OutlineTarget survives, so
    // re-enabling re-applies via Sync.
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

    // Outline removed (Target gone, shadow still applied): undo.
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
