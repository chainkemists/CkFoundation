#include "CkBallistics_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkBallistics/CkBallistics_Log.h"

#include <RealisticProjectileComponent.h>

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Ballistics_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Ballistics_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent) const
        -> void
    {
        auto OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(OwningActor),
            TEXT("Entity [{}] MUST have a valid owning actor"), InHandle)
        { return; }

        const auto& NewComponentParams = UCk_Utils_Actor_UE::AddNewActorComponent_Params<URealisticProjectileComponent>{
            OwningActor, OwningActor->GetRootComponent()};

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<URealisticProjectileComponent>(NewComponentParams,
        [InParams](URealisticProjectileComponent* InComp)
        {
            auto Behavior = FRealisticProjectileBehavior{};
            Behavior.InitialSpeed = InParams.Get_Params().Get_InitialSpeed();
            Behavior.Vterminal = InParams.Get_Params().Get_TerminalVelocity();
            Behavior.Gravity = InParams.Get_Params().Get_Gravity();
            InComp->SetBehaviorSettings(Behavior);
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Ballistics_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_Ballistics_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Ballistics_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent,
            FFragment_Ballistics_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Ballistics_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_Ballistics_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent,
            const FCk_Request_Ballistics_ExampleRequest& InRequest)
        -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Ballistics_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Ballistics_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Ballistics_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Ballistics_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Ballistics_Rep>(InHandle, [&](UCk_Fragment_Ballistics_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------