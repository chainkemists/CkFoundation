#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ism_proxy
{
    // Every site that builds an ISM instance transform must compose identically, or a shadow instance drifts
    // off the main one it mirrors. See CkIsmRenderer/CLAUDE.md.
    CKISMRENDERER_API auto
    Get_TransformWithLocalOffset(
        const ck::FFragment_IsmProxy_Params& InParams,
        const FTransform& InTransform) -> FTransform;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_IsmRenderer_ClearInstances;

    class CKISMRENDERER_API FProcessor_IsmProxy_Setup : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Setup,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadWrite<FFragment_IsmProxy_Current>,
        FTag_IsmProxy_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_IsmRenderer_ClearInstances>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FTag_IsmProxy_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_AddInstance : public ck_exp::TProcessor<
        FProcessor_IsmProxy_AddInstance,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadWrite<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_Transform>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_Disabled>,
        FTag_IsmProxy_NeedsInstanceAdded,
        TExclude<FTag_SceneNode_RelativeTransformUpdated>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_IsmProxy_Setup>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FTag_IsmProxy_NeedsInstanceAdded;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InCurrentTransform) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_TransformInstance : public ck_exp::TProcessor<
        FProcessor_IsmProxy_TransformInstance,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadOnly<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_Transform>,
        TExclude<FTag_IsmProxy_Disabled>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_NeedsInstanceAdded>,
        TIgnoreInEditor<FTag_IsmProxy_Movable>,
        FTag_Transform_Updated,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;
        using MarkedDirtyBy = FTag_Transform_Updated;

        // DoTick batches MarkRenderStateDirty across the instances visited by the base view. With no
        // dirty transforms there is no world lookup, batch state, or render-state work to perform.
        using MainPassRequiredFragments = entt::type_list<FTag_Transform_Updated>;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
        TSet<UInstancedStaticMeshComponent*> _Isms;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG : public ck_exp::TProcessor<
        FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadOnly<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_Transform>,
        TExclude<FTag_IsmProxy_Disabled>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_NeedsInstanceAdded>,
        TExclude<FTag_IsmProxy_Movable>,
        FTag_Transform_Updated,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;
        using TProcessor::TProcessor;
        using MarkedDirtyBy = FTag_Transform_Updated;
        // Shares MarkedDirtyBy with the real transform writer, so this read-only check must be
        // ordered after it — both to read pushed transforms and to settle the scheduler advisory.
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance>;

    public:
        static auto
        DidTransformChange(
            const FFragment_Transform& InTransform,
            const FFragment_IsmProxy_Params& InParams,
            const FTransform& InInstanceTransform) -> bool;

        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_EndPlay : public ck_exp::TProcessor<
        FProcessor_IsmProxy_EndPlay,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadWrite<FFragment_IsmProxy_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_HandleRequests : public ck_exp::TProcessor<
        FProcessor_IsmProxy_HandleRequests,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadWrite<FFragment_IsmProxy_Current>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TReadOnly<FFragment_IsmProxy_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_IsmProxy_AddInstance>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FFragment_IsmProxy_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;
    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_IsmProxy_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomInstanceData& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomInstanceDataValue& InRequest) const -> void;

        auto DoHandleRequest(
            FCk_Handle_IsmProxy& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomPrimitiveData& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_EnableDisable& InRequest) const -> void;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_HandleLateCustomDataRequests : public ck_exp::TProcessor<
        FProcessor_IsmProxy_HandleLateCustomDataRequests,
        FCk_Handle_IsmProxy,
        TReadOnly<FFragment_IsmProxy_Params>,
        TReadWrite<FFragment_IsmProxy_Current>,
        TReadOnly<FFragment_IsmProxy_LateCustomDataRequests>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_DeferredApply;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FFragment_IsmProxy_LateCustomDataRequests;

    public:
        using TProcessor::TProcessor;

        auto
        DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_IsmProxy_LateCustomDataRequests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomInstanceDataValue& InRequest) const -> bool;

    private:
        TWeakObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed proxy's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKISMRENDERER_API FProcessor_IsmProxy_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CancelPendingRequests,
        FCk_Handle_IsmProxy,
        ck::TReadOnly<FFragment_IsmProxy_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Requests& InRequestsComp)
            -> void;
    };

    class CKISMRENDERER_API FProcessor_IsmProxy_CancelPendingLateCustomDataRequests : public ck_exp::TProcessor<
        FProcessor_IsmProxy_CancelPendingLateCustomDataRequests,
        FCk_Handle_IsmProxy,
        ck::TReadOnly<FFragment_IsmProxy_LateCustomDataRequests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_LateCustomDataRequests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
