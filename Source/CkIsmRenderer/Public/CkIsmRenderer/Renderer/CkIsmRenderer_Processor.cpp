#include "CkIsmRenderer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "Components/InstancedStaticMeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_IsmRenderer_Setup::IsmActorComponentInitFunctor::
        IsmActorComponentInitFunctor(
            FCk_Handle_IsmRenderer& InRendererEntity,
            const FFragment_IsmRenderer_Params& InRendererParams,
            ECk_Mobility InIsmMobility)
        : _RendererEntity(InRendererEntity)
        , _RendererParams(InRendererParams)
        , _IsmMobility(InIsmMobility)
    {
    }

    auto
        FProcessor_IsmRenderer_Setup::IsmActorComponentInitFunctor::
        operator()(
                UInstancedStaticMeshComponent* InIsmActorComp)
        -> void
    {
        if (ck::Is_NOT_Valid(_RendererEntity))
        { return; }

        const auto& Params = _RendererParams.Get_Params();
        const auto& MeshPtr = Params.Get_Mesh();

        switch (_IsmMobility)
        {
            case ECk_Mobility::Static:
            {
                _RendererEntity.Get<FFragment_IsmRenderer_Current>()._IsmComponent_Static = InIsmActorComp;
                InIsmActorComp->SetMobility(EComponentMobility::Static);
                break;
            }
            case ECk_Mobility::Movable:
            {
                _RendererEntity.Get<FFragment_IsmRenderer_Current>()._IsmComponent_Movable = InIsmActorComp;
                InIsmActorComp->SetMobility(EComponentMobility::Movable);
                break;
            }
            case ECk_Mobility::Stationary:
            {
                CK_TRIGGER_ENSURE(TEXT("Ism does not support mobility of type [{}]"), _IsmMobility);
                break;
            }
            case ECk_Mobility::Count:
            default:
            {
                CK_INVALID_ENUM(_IsmMobility);
                break;
            }
        }

        InIsmActorComp->SetCollisionEnabled(UCk_Utils_Enum_UE::ConvertToECollisionEnabled(Params.Get_Collision()));
        InIsmActorComp->CastShadow = Params.Get_CastShadows() == ECk_EnableDisable::Enable;
        InIsmActorComp->SetStaticMesh(MeshPtr);
        InIsmActorComp->NumCustomDataFloats = Params.Get_NumCustomData();
    }

    auto
        FProcessor_IsmRenderer_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmRenderer_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Params& InParams,
            const FFragment_OwningActor_Current& InOwningActorCurrent) const
        -> void
    {
        const auto Actor = InOwningActorCurrent.Get_EntityOwningActor();
        const auto& Params = InParams.Get_Params();

        CK_ENSURE_IF_NOT(ck::IsValid(Actor),
            TEXT("OwningActor [{}] for Handle [{}] is INVALID. Unable to Setup the IsmRenderer"), Actor, InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(Params.Get_Mesh()),
            TEXT("The Mesh [{}] is INVALID. Unable to Setup the IsmRenderer"), InParams.Get_Params().Get_Mesh())
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(Params.Get_RendererName()),
            TEXT("The Renderer Name [{}] is INVALID. Unable to Setup the IsmRenderer"), Params.Get_RendererName())
        { return; }

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UInstancedStaticMeshComponent>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UInstancedStaticMeshComponent>{Actor.Get()}.Set_IsUnique(false),
            IsmActorComponentInitFunctor{InHandle, InParams, ECk_Mobility::Movable}
        );

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UInstancedStaticMeshComponent>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UInstancedStaticMeshComponent>{Actor.Get()}.Set_IsUnique(false),
            IsmActorComponentInitFunctor{InHandle, InParams, ECk_Mobility::Static}
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmRenderer_ClearInstances::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Current& InCurrent) const
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_IsmComponent_Movable()))
        { return; }

        InCurrent.Get_IsmComponent_Movable()->ClearInstances();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmRenderer_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmRenderer_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Current& InCurrent,
            FFragment_InstancedStaticMeshRenderer_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_InstancedStaticMeshRenderer_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequestVariant) -> void
            {
                DoHandleRequest(InHandle, InCurrent, InRequestVariant);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_IsmRenderer_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmRenderer_Current& InCurrent,
            const FCk_Request_IsmRenderer_NewInstance& InRequest)
        -> void
    {
        const auto IsmComponent_Static = InCurrent.Get_IsmComponent_Static();
        const auto IsmComponent_Movable = InCurrent.Get_IsmComponent_Movable();

        if (ck::Is_NOT_Valid(IsmComponent_Static) || ck::Is_NOT_Valid(IsmComponent_Movable))
        { return; }

        IsmComponent_Static->AddInstance(InRequest.Get_Transform());
    }
}

// --------------------------------------------------------------------------------------------------------------------
