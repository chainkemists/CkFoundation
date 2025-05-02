#pragma once

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkIsmRenderer/CkIsmRenderer_Log.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment.h"

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
        template<typename T_IsmCompType>
        struct IsmActorComponentInitFunctor
        {
            explicit
            IsmActorComponentInitFunctor(
                const FCk_Handle_IsmRenderer& InRendererEntity,
                const FFragment_IsmRenderer_Params& InRendererParams,
                ECk_Mobility InIsmMobility);

        public:
            auto
            operator()(
                T_IsmCompType* InIsmActorComp) -> void;

        private:
            FCk_Handle_IsmRenderer _RendererEntity;
            FFragment_IsmRenderer_Params _RendererParams;
            ECk_Mobility _IsmMobility;
        };
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmRenderer_ClearInstances : public ck_exp::TProcessor<
        FProcessor_IsmRenderer_ClearInstances,
        FCk_Handle_IsmRenderer,
        FFragment_IsmRenderer_Current,
        FTag_IsmRenderer_Movable,
        FTag_IsmRenderer_UpdateByRecreating,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_IsmCompType>
    FProcessor_IsmRenderer_Setup::IsmActorComponentInitFunctor<T_IsmCompType>::
        IsmActorComponentInitFunctor(
            const FCk_Handle_IsmRenderer& InRendererEntity,
            const FFragment_IsmRenderer_Params& InRendererParams,
            ECk_Mobility InIsmMobility)
        : _RendererEntity(InRendererEntity)
        , _RendererParams(InRendererParams)
        , _IsmMobility(InIsmMobility)
    {
    }

    template <typename T_IsmCompType>
    auto
        FProcessor_IsmRenderer_Setup::IsmActorComponentInitFunctor<T_IsmCompType>::
        operator()(
            T_IsmCompType* InIsmActorComp)
        -> void
    {
        if (ck::Is_NOT_Valid(_RendererEntity))
        { return; }

        const auto& Params = _RendererParams.Get_Params();
        const auto& MeshToRender = Params->Get_Mesh();

        switch (_IsmMobility)
        {
            case ECk_Mobility::Static:
            {
                InIsmActorComp->SetHasPerInstancePrevTransforms(false);
                break;
            }
            case ECk_Mobility::Movable:
            {
                // TODO: We tried to set previous transforms for better TSR results. However, that was not the case.
                // UDN promises that UE5.4 should have the renderer automatically setting previous transform to
                // make the TSR work properly (however in our testing in 5.4, that's not the case)
                InIsmActorComp->SetHasPerInstancePrevTransforms(true);
                _RendererEntity.Add<FTag_IsmRenderer_Movable>();
                break;
            }
            case ECk_Mobility::Stationary:
            {
                ismrenderer::Warning(TEXT("Ism does not support mobility of type [{}]"), _IsmMobility);
                break;
            }
            case ECk_Mobility::Count:
            default:
            {
                CK_INVALID_ENUM(_IsmMobility);
                break;
            }
        }

        _RendererEntity.Get<FFragment_IsmRenderer_Current>()._IsmComponent = InIsmActorComp;

        InIsmActorComp->SetCollisionEnabled(UCk_Utils_Enum_UE::ConvertToECollisionEnabled(Params->Get_PhysicsInfo().Get_Collision()));
        InIsmActorComp->SetCollisionProfileName(Params->Get_PhysicsInfo().Get_CollisionProfileName().Name);
        InIsmActorComp->CastShadow = Params->Get_LightingInfo().Get_CastShadows() == ECk_EnableDisable::Enable;
        InIsmActorComp->SetStaticMesh(MeshToRender);
        InIsmActorComp->NumCustomDataFloats = Params->Get_NumCustomData();
        InIsmActorComp->InstanceStartCullDistance = Params->Get_CullingInfo().Get_InstanceCullDistance_Start();
        InIsmActorComp->InstanceEndCullDistance = Params->Get_CullingInfo().Get_InstanceCullDistance_End();

        const auto& CustomPrimitiveDataDefaults = Params->Get_CustomPrimitiveDataDefaults().Data;
        for (auto Index = 0; Index < CustomPrimitiveDataDefaults.Num(); ++Index)
        {
            InIsmActorComp->SetDefaultCustomPrimitiveDataFloat(Index, CustomPrimitiveDataDefaults[Index]);
            InIsmActorComp->SetCustomPrimitiveDataFloat(Index, CustomPrimitiveDataDefaults[Index]);
        }

        auto MaterialSlotsOverriden = TSet<int32>{};

        for (const auto& MaterialOverride : Params->Get_MaterialsInfo().Get_MaterialOverrides())
        {
            const auto& MaterialSlot = MaterialOverride.Get_MaterialSlot();
            const auto& ReplacementMaterial = MaterialOverride.Get_ReplacementMaterial();

            CK_ENSURE_IF_NOT(NOT MaterialSlotsOverriden.Contains(MaterialSlot),
                TEXT("More than one Material Override target Slot #[{}] on Static Mesh [{}] for Ism Renderer [{}]"),
                MaterialSlot,
                MeshToRender,
                Params)
            { continue; }

            CK_ENSURE_IF_NOT(InIsmActorComp->GetNumMaterials() > MaterialSlot,
                TEXT("Found Material Override that targets Slot #[{}] on Static Mesh [{}] for Ism Renderer [{}]\n"
                     "The Mesh has a maximum of [{}] Material Slots available!"),
                MaterialSlot,
                MeshToRender,
                Params,
                InIsmActorComp->GetNumMaterials())
            { continue; }

            MaterialSlotsOverriden.Add(MaterialSlot);

            InIsmActorComp->SetMaterial(MaterialSlot, ReplacementMaterial);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
