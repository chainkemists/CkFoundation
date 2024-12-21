#include "CkIsmRenderer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "Components/InstancedStaticMeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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
            TEXT("OwningActor [{}] for Handle [{}] is INVALID. Unable to Setup the AntAgent Renderer"), Actor, InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(Params.Get_Mesh()),
            TEXT("The Mesh [{}] is INVALID. Unable to Setup the AntAgent Renderer"), InParams.Get_Params().Get_Mesh())
        { return; }

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UInstancedStaticMeshComponent>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UInstancedStaticMeshComponent>{Actor.Get()}.Set_IsUnique(false),
            [=, this](UInstancedStaticMeshComponent* InISM) mutable
            {
                if (ck::Is_NOT_Valid(InHandle))
                { return; }

                const auto& MeshPtr = Params.Get_Mesh();

                CK_ENSURE_IF_NOT(ck::IsValid(MeshPtr),
                    TEXT("The StaticMesh [{}] is INVALID. Unable to Setup the AntAgent Renderer"), MeshPtr)
                { return; }

                InHandle.Get<FFragment_IsmRenderer_Current>()._IsmComponent_Static = InISM;

                InISM->SetCollisionEnabled(UCk_Utils_Enum_UE::ConvertToECollisionEnabled(Params.Get_Collision()));
                InISM->CastShadow = Params.Get_CastShadows() == ECk_EnableDisable::Enable;
                InISM->SetStaticMesh(MeshPtr);
            }
        );

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UInstancedStaticMeshComponent>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UInstancedStaticMeshComponent>{Actor.Get()}.Set_IsUnique(false),
            [=, this](UInstancedStaticMeshComponent* InISM) mutable
            {
                if (ck::Is_NOT_Valid(InHandle))
                { return; }

                const auto& MeshPtr = Params.Get_Mesh();

                CK_ENSURE_IF_NOT(ck::IsValid(MeshPtr),
                    TEXT("The StaticMesh [{}] is INVALID. Unable to Setup the AntAgent Renderer"), MeshPtr)
                { return; }

                InHandle.Get<FFragment_IsmRenderer_Current>()._IsmComponent_Movable = InISM;

                InISM->SetCollisionEnabled(UCk_Utils_Enum_UE::ConvertToECollisionEnabled(Params.Get_Collision()));
                InISM->CastShadow = Params.Get_CastShadows() == ECk_EnableDisable::Enable;
                InISM->SetStaticMesh(MeshPtr);
            }
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
        if (ck::Is_NOT_Valid(InCurrent.Get_IsmComponent_Static()))
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
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InCurrent, InRequestVariant);
        }));
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

        InCurrent.Get_IsmComponent_Static()->AddInstance(InRequest.Get_Transform());
    }
}

// --------------------------------------------------------------------------------------------------------------------
