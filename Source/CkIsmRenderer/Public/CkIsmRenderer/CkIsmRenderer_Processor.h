#pragma once

#include "CkIsmRenderer_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmRenderer_Setup : public ck_exp::TProcessor<
        FProcessor_IsmRenderer_Setup,
        FCk_Handle_IsmRenderer,
        FFragment_IsmRenderer_Params,
        FFragment_OwningActor_Current,
        FTag_IsmRenderer_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_IsmRenderer_NeedsSetup;

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
            const FFragment_IsmRenderer_Params& InParams,
            const FFragment_OwningActor_Current& InOwningActorCurrent) const
            -> void;

    private:
        TMap<TWeakObjectPtr<UStaticMesh>, int32> _MeshToType;
        int32 _NextAvailableMeshType = -1;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmRenderer_ClearInstances : public ck_exp::TProcessor<
        FProcessor_IsmRenderer_ClearInstances,
        FCk_Handle_IsmRenderer,
        FFragment_AntAgent_Renderer_Current,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmRenderer_HandleRequests : public ck_exp::TProcessor<
        FProcessor_IsmRenderer_HandleRequests,
        FCk_Handle_IsmRenderer,
        FFragment_AntAgent_Renderer_Current,
        FFragment_InstancedStaticMeshRenderer_Requests,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_InstancedStaticMeshRenderer_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent,
            FFragment_InstancedStaticMeshRenderer_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent,
            const FCk_Request_IsmRenderer_NewInstance& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
