#include "CkAntSquad_Processor.h"

#include "CkIsmRenderer/CkIsmRenderer_Fragment.h"
#include "CkIsmRenderer/CkIsmRenderer_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "Components/InstancedStaticMeshComponent.h"

#include <AntSubsystem.h>

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

    FProcessor_AntSquad_UpdateInstances::
        FProcessor_AntSquad_UpdateInstances(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld)
        : _Registry(InRegistry)
    {
        _AntSubsystem = InWorld->GetSubsystem<UAntSubsystem>();
    }

    auto
        FProcessor_AntSquad_UpdateInstances::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        for (const auto &Agent : _AntSubsystem->GetUnderlyingAgentsList())
        {
            const auto Center = FVector{Agent.GetLocationLerped()};

            const auto EntityId = Agent.GetFlag();
            const auto RendererEntity = _Registry.Get_ValidEntity(static_cast<FCk_Entity::IdType>(EntityId));
            auto RendererHandle = UCk_Utils_IsmRenderer_UE::Cast(ck::MakeHandle(RendererEntity, _Registry));

            UCk_Utils_IsmRenderer_UE::Request_RenderInstance(RendererHandle,
                FCk_Request_IsmRenderer_NewInstance{}.Set_Transform(FTransform{Center}));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
