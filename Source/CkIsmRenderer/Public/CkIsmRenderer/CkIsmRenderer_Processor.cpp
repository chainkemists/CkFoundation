#include "CkIsmRenderer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "Components/InstancedStaticMeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_sim_renderer_processor
{
    using IsmAcArray = std::array<TWeakObjectPtr<UInstancedStaticMeshComponent>,  static_cast<int32>(ECk_Mobility::Count)>;
    static TMap<FGameplayTag, IsmAcArray> Renderers;
}

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

    // --------------------------------------------------------------------------------------------------------------------
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Static::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        using ck_sim_renderer_processor::Renderers;

        _TransientEntity.View<
            FFragment_IsmRenderer_Params,
            FFragment_IsmRenderer_Current,
            TExclude<FTag_IsmRenderer_NeedsSetup>>().ForEach(
            [&](FCk_Entity InHandle, const FFragment_IsmRenderer_Params& InParams, const FFragment_IsmRenderer_Current& InCurrent)
            {
                using ck_sim_renderer_processor::IsmAcArray;

                const auto IsmArray = IsmAcArray{
                    InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Movable()};

                Renderers.Add(InParams.Get_Params().Get_RendererName(), IsmArray);
            });

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmProxy_Static::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams) const
        -> void
    {
        using ck_sim_renderer_processor::Renderers;

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto MeshScale = [&]
        {
            const auto& Scale = Params.Get_MeshScale();
            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(Scale),
                TEXT("MeshScale has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), Scale)
            { return FVector::OneVector; }

            return Scale;
        }();

        const auto& MaybeFound = Renderers.Find(RendererName);

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("No Renderer with Name [{}] found. Unable to Setup the IsmProxy"), RendererName)
        { return; }

        const auto& RendererToUse = *MaybeFound;

        CK_ENSURE_IF_NOT(ck::IsValid(RendererToUse[0]) && ck::IsValid(RendererToUse[1]) && ck::IsValid(RendererToUse[2]),
            TEXT("(One of the) Renderer(s) with Name [{}] is INVALID. Unable to Setup the IsmProxy"), RendererName)
        { return; }

        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to Setup the IsmProxy"), InHandle)
        { return; }

        auto CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        CurrentTransform.SetScale3D(MeshScale);

        if (InHandle.Has<FTag_IsmProxy_Dynamic>())
        { RendererToUse[static_cast<int32>(ECk_Mobility::Movable)]->AddInstance(CurrentTransform); }
        else
        { RendererToUse[static_cast<int32>(ECk_Mobility::Static)]->AddInstance(CurrentTransform); }

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Dynamic::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams) const
        -> void
    {
        using ck_sim_renderer_processor::Renderers;

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto MeshScale = [&]
        {
            const auto& Scale = Params.Get_MeshScale();
            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(Scale),
                TEXT("MeshScale has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), Scale)
            { return FVector::OneVector; }

            return Scale;
        }();

        const auto& MaybeFound = Renderers.Find(RendererName);

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("No Renderer with Name [{}] found. Unable to Setup the IsmProxy"), RendererName)
        { return; }

        const auto& RendererToUse = *MaybeFound;

        CK_ENSURE_IF_NOT(ck::IsValid(RendererToUse[static_cast<int32>(ECk_Mobility::Movable)]),
            TEXT("The MOVABLE Renderer with Name [{}] is INVALID. Unable to Render the MOVABLE IsmProxy"), RendererName)
        { return; }

        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to Setup the IsmProxy"), InHandle)
        { return; }

        auto CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        CurrentTransform.SetScale3D(MeshScale);

        RendererToUse[static_cast<int32>(ECk_Mobility::Movable)]->AddInstance(CurrentTransform);
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
