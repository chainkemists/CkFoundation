#pragma once

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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
            const FFragment_OwningActor_Current& InOwningActorCurrent) const -> void;

    private:
        struct IsmActorComponentInitFunctor
        {
        explicit
            IsmActorComponentInitFunctor(
                FCk_Handle_IsmRenderer& InRendererEntity,
                const FFragment_IsmRenderer_Params& InRendererParams,
                ECk_Mobility InIsmMobility);

        public:
            auto
            operator()(
                UInstancedStaticMeshComponent* InIsmActorComp) -> void;

        private:
            FCk_Handle_IsmRenderer _RendererEntity;
            ck::FFragment_IsmRenderer_Params _RendererParams;
            ECk_Mobility _IsmMobility;
        };
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmRenderer_ClearInstances : public ck_exp::TProcessor<
        FProcessor_IsmRenderer_ClearInstances,
        FCk_Handle_IsmRenderer,
        FFragment_IsmRenderer_Current,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
