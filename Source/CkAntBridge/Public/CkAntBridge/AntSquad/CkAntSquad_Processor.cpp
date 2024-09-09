#include "CkAntSquad_Processor.h"

#include "CkNet/CkNet_Utils.h"

#include "Components/InstancedStaticMeshComponent.h"

#include <AntSubsystem.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ant_squad_processor
{

    enum class ECk_AntAgent_Renderer_MeshType
    {
        MeshType0 = 0,
        MeshType1 = 1,
        MeshType2 = MeshType1 << 2,
        MeshType3 = MeshType1 << 3,
        MeshType4 = MeshType1 << 4,
        MeshType5 = MeshType1 << 5,
        MeshType6 = MeshType1 << 6,
        MeshType7 = MeshType1 << 7,
        MeshType8 = MeshType1 << 8,
        MeshType9 = MeshType1 << 9,
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <ECk_AntAgent_Renderer_MeshType T_MeshType>
    struct TTag_MeshType : ck::TTag<TTag_MeshType<T_MeshType>> { };

    auto
        Add_MeshTypeTag(
            FCk_Handle& InHandle,
            ECk_AntAgent_Renderer_MeshType MeshType)
        -> void
    {
        switch (MeshType)
        {
            case ECk_AntAgent_Renderer_MeshType::MeshType0: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType0>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType1: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType1>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType2: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType2>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType3: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType3>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType4: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType4>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType5: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType5>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType6: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType6>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType7: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType7>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType8: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType8>>(); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType9: InHandle.Add<TTag_MeshType<ECk_AntAgent_Renderer_MeshType::MeshType9>>(); break;
            default:
            {
                break;
            }
        }
    }

    template <ECk_AntAgent_Renderer_MeshType T>
    auto
    Add_Instance(
        FCk_Registry& InRegistry,
        const FTransform& InTransform)
    {
        InRegistry.View<TTag_MeshType<T>, ck::FFragment_AntAgent_Renderer_Current>().ForEach(
        [&](FCk_Entity InEntity, const ck::FFragment_AntAgent_Renderer_Current& InCurrent)
        {
            if (ck::Is_NOT_Valid(InCurrent.Get_IsmComponent()))
            { return; }

            InCurrent.Get_IsmComponent()->AddInstance(InTransform);
        });
    }

    auto
        Add_MeshInstance(
            FCk_Registry& InRegistry,
            int32 InFlag,
            const FTransform& InTransform)
        -> void
    {
        switch (static_cast<ECk_AntAgent_Renderer_MeshType>(InFlag))
        {
            case ECk_AntAgent_Renderer_MeshType::MeshType0: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType0>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType1: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType1>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType2: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType2>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType3: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType3>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType4: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType4>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType5: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType5>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType6: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType6>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType7: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType7>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType8: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType8>(InRegistry, InTransform); break;
            case ECk_AntAgent_Renderer_MeshType::MeshType9: Add_Instance<ECk_AntAgent_Renderer_MeshType::MeshType9>(InRegistry, InTransform); break;

        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_AntSquad_HandleRequests::
        FProcessor_AntSquad_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld)
        : TProcessor(InRegistry)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
            TEXT("The World [{}] is INVALID. AntSquad will not work correctly."), InWorld)
        { return; }

        _AntSubsystem = InWorld->GetSubsystem<UAntSubsystem>();
    }

    auto
        FProcessor_AntSquad_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AntSquad_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AntSquad_Current& InCurrent,
            FFragment_AntSquad_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InCurrent, InRequestVariant);

            // TODO: Add formatter for each request and track which one was responsible for destroying entity
        }));
    }

    auto
        FProcessor_AntSquad_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_AntSquad_Current& InCurrent,
            const FCk_Request_AntSquad_AddAgent& InRequestAgent) const
        -> void
    {
        const auto EntityId = InRequestAgent.Get_AgentData().Get_Entity().Get_ID();

        const auto Agent = _AntSubsystem->AddAgent(
            InRequestAgent.Get_RelativeLocation(),
            InRequestAgent.Get_Radius(),
            InRequestAgent.Get_Height(),
            InRequestAgent.Get_FaceAngle(),
            static_cast<int32>(EntityId));

        // TODO: Figure out how to cleanly validate the Agents

        InCurrent._AntHandles.Emplace(Agent);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AntAgent_Renderer_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AntAgent_Renderer_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Params& InParams,
            const FFragment_OwningActor_Current& InOwningActorCurrent)
        -> void
    {
        const auto Actor = InOwningActorCurrent.Get_EntityOwningActor();

        CK_ENSURE_IF_NOT(ck::IsValid(Actor),
            TEXT("OwningActor [{}] for Handle [{}] is INVALID. Unable to Setup the AntAgent Renderer"), Actor, InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Params().Get_Mesh()),
            TEXT("The Mesh [{}] is INVALID. Unable to Setup the AntAgent Renderer"), InParams.Get_Params().Get_Mesh())
        { return; }

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UInstancedStaticMeshComponent>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UInstancedStaticMeshComponent>{Actor.Get()}.Set_IsUnique(false),
            [=, this](UInstancedStaticMeshComponent* InISM) mutable
            {
                if (ck::Is_NOT_Valid(InHandle))
                { return; }

                const auto& MeshPtr = InParams.Get_Params().Get_Mesh();

                CK_ENSURE_IF_NOT(ck::IsValid(MeshPtr),
                    TEXT("The StaticMesh [{}] is INVALID. Unable to Setup the AntAgent Renderer"), MeshPtr)
                { return; }

                InHandle.Get<FFragment_AntAgent_Renderer_Current>()._IsmComponent = InISM;

                using namespace ck_ant_squad_processor;

                const auto MeshType = [&, this]()
                {
                    const auto Found = _MeshToType.Find(MeshPtr);
                    if (ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}))
                    { return static_cast<ECk_AntAgent_Renderer_MeshType>(*Found); }

                    _MeshToType.Add(MeshPtr, ++_NextAvailableMeshType);
                    return static_cast<ECk_AntAgent_Renderer_MeshType>(_NextAvailableMeshType);
                }();

                Add_MeshTypeTag(InHandle, MeshType);

                InISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                InISM->CastShadow = false;
                InISM->SetStaticMesh(MeshPtr);
            }
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AntAgent_Renderer_ClearInstances::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent) const
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_IsmComponent()))
        { return; }

        InCurrent.Get_IsmComponent()->ClearInstances();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InstancedStaticMeshRenderer_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_InstancedStaticMeshRenderer_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent,
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
        FProcessor_InstancedStaticMeshRenderer_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent,
            const FCk_Request_InstancedStaticMeshRenderer_NewInstance& InRequest)
            -> void
    {
        const auto IsmComponent = InCurrent.Get_IsmComponent();

        if (ck::Is_NOT_Valid(IsmComponent))
        { return; }

        InCurrent.Get_IsmComponent()->AddInstance(InRequest.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_AntAgent_Renderer_Update::
        FProcessor_AntAgent_Renderer_Update(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld)
        : _Registry(InRegistry)
    {
        _AntSubsystem = InWorld->GetSubsystem<UAntSubsystem>();
    }

    auto
        FProcessor_AntAgent_Renderer_Update::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        for (const auto &Agent : _AntSubsystem->GetUnderlyingAgentsList())
        {
            const auto Center = FVector{Agent.GetLocationLerped()};

            const auto EntityId = Agent.GetFlag();
            const auto RendererEntity = _Registry.Get_ValidEntity(static_cast<FCk_Entity::IdType>(EntityId));
            auto RendererHandle = UCk_Utils_AntAgent_Renderer_UE::Cast(ck::MakeHandle(RendererEntity, _Registry));

            UCk_Utils_AntAgent_Renderer_UE::Request_RenderInstance(RendererHandle,
                FCk_Request_InstancedStaticMeshRenderer_NewInstance{}.Set_Transform(FTransform{Center}));

            ck_ant_squad_processor::Add_MeshInstance(_Registry, Agent.GetFlag(), FTransform(FRotator::ZeroRotator, Center, FVector::One()));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
