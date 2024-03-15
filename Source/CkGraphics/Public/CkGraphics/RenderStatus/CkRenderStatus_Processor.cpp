#include "CkRenderStatus_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkGraphics/CkGraphics_Utils.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RenderStatus_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_RenderStatus_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderStatus_Requests& InRequestsComp) const
        -> void
    {
        algo::ForEachRequest(InRequestsComp._Requests, [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InRequest);
        });

        InHandle.Remove<MarkedDirtyBy>();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_RenderStatus_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_RenderStatus_QueryRenderedActors& InRequest)
        -> void
    {
        const auto& TimeTolerance = InRequest.Get_TimeTolerance();
        const auto& RenderGroup = InRequest.Get_RenderGroup();

        auto RenderedEntityWithActors = TArray<FCk_EntityOwningActor_BasicDetails>{};

        const auto& TryAddToRenderedActorsList = [&](AActor* InActor, const FCk_Handle& InActorEntity)
        {
            if (ck::Is_NOT_Valid(InActor))
            { return; }

            if (NOT UCk_Utils_Graphics_UE::Get_WasActorRecentlyRendered(InActor, TimeTolerance))
            { return; }

            RenderedEntityWithActors.Add(FCk_EntityOwningActor_BasicDetails{InActor, InActorEntity});
        };

        switch (RenderGroup)
        {
            case ECk_RenderStatus_Group::Unspecified:
            {
                InHandle.View<FFragment_OwningActor_Current, CK_IGNORE_PENDING_KILL>().ForEach(
                [&](EntityType InEntity, const FFragment_OwningActor_Current& InOwningActorComp)
                {
                    const auto Handle = MakeHandle(InEntity, InHandle);
                    TryAddToRenderedActorsList(InOwningActorComp.Get_EntityOwningActor().Get(), Handle);
                });

                break;
            }
            case ECk_RenderStatus_Group::WorldStatic:
            {
                InHandle.View<FFragment_OwningActor_Current, FTag_RenderStatus_Group_WorldStatic, CK_IGNORE_PENDING_KILL>().ForEach(
                [&](EntityType InEntity, const FFragment_OwningActor_Current& InOwningActorComp)
                {
                    const auto Handle = MakeHandle(InEntity, InHandle);
                    TryAddToRenderedActorsList(InOwningActorComp.Get_EntityOwningActor().Get(), Handle);
                });

                break;
            }
            case ECk_RenderStatus_Group::WorldDynamic:
            {
                InHandle.View<FFragment_OwningActor_Current, FTag_RenderStatus_Group_WorldDynamic, CK_IGNORE_PENDING_KILL>().ForEach(
                [&](EntityType InEntity, const FFragment_OwningActor_Current& InOwningActorComp)
                {
                    const auto Handle = MakeHandle(InEntity, InHandle);
                    TryAddToRenderedActorsList(InOwningActorComp.Get_EntityOwningActor().Get(), Handle);
                });

                break;
            }
            case ECk_RenderStatus_Group::Pawn:
            {
                InHandle.View<FFragment_OwningActor_Current, FTag_RenderStatus_Group_Pawn, CK_IGNORE_PENDING_KILL>().ForEach(
                [&](EntityType InEntity, const FFragment_OwningActor_Current& InOwningActorComp)
                {
                    const auto Handle = MakeHandle(InEntity, InHandle);
                    TryAddToRenderedActorsList(InOwningActorComp.Get_EntityOwningActor().Get(), Handle);
                });

                break;
            }
            default:
            {
                CK_INVALID_ENUM(RenderGroup);
                break;
            }
        }

        const auto RenderedActorsList = FCk_RenderedActorsList{}
                                          .Set_RenderGroup(RenderGroup)
                                          .Set_RenderedEntityWithActors(RenderedEntityWithActors);

        UUtils_Signal_OnRenderedActorsQueried::Broadcast(InHandle, MakePayload(RenderedActorsList,
            UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag)));
    }
}

// --------------------------------------------------------------------------------------------------------------------
