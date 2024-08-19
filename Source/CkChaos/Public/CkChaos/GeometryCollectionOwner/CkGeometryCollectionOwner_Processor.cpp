#include "CkGeometryCollectionOwner_Processor.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment.h"
#include "CkChaos/GeometryCollection/CkGeometryCollection_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "GeometryCollection/GeometryCollectionComponent.h"

#include "PhysicsProxy/GeometryCollectionPhysicsProxy.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GeometryCollectionOwner_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        const auto& BasicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(BasicDetails.Get_Actor()),
            TEXT("OwningActor [{}] of [{}] is NOT valid"), BasicDetails.Get_Actor(), InHandle)
        { return; }

        const auto& GCs = [&]
        {
            auto Ret = TArray<UGeometryCollectionComponent*>{};
            BasicDetails.Get_Actor()->GetComponents<UGeometryCollectionComponent>(Ret);

            return Ret;
        }();

        algo::ForEach(GCs, [&](UGeometryCollectionComponent* InGeometryCollection)
        {
            UCk_Utils_GeometryCollection_UE::Add(InHandle, FCk_Fragment_GeometryCollection_ParamsData{InGeometryCollection});
        });

        InHandle.Remove<FTag_GeometryCollectionOwner_RequiresSetup>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GeometryCollectionOwner_HandleRequests::DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_GeometryCollectionOwner_HandleRequests::ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollectionOwner_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_GeometryCollectionOwner_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InRequest);
            }));
        });
    }

    auto
        FProcessor_GeometryCollectionOwner_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GeometryCollection_ApplyStrain_Replicated& InRequest)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Request()),
            TEXT("Unable to ApplyStrain on [{}] GeometryCollection because the Request DataAsset [{}] is INVALID"),
            InHandle, InRequest.Get_Request())
        { return; }

        ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(InHandle, [&](FCk_Handle_GeometryCollection InGc)
        {
            const auto& Settings = InRequest.Get_Request();
            const auto& Request = FCk_Request_GeometryCollection_ApplyStrain{InRequest.Get_Location(), Settings->Get_Radius()}
            .Set_LinearVelocity(Settings->Get_LinearVelocity())
            .Set_AngularVelocity(Settings->Get_AngularVelocity())
            .Set_InternalStrain(Settings->Get_InternalStrain())
            .Set_ExternalStrain(Settings->Get_ExternalStrain());

            UCk_Utils_GeometryCollection_UE::Request_ApplyStrainAndVelocity(InGc, Request);
        });

        InHandle.AddOrGet<FFragment_GeometryCollection_ReplicationRequests>()._Requests.Emplace(InRequest);
    }

    auto
        FProcessor_GeometryCollectionOwner_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GeometryCollection_ApplyAoE_Replicated& InRequest)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Request()),
            TEXT("Unable to ApplyAoE on [{}] GeometryCollection because the Request DataAsset [{}] is INVALID"),
            InHandle, InRequest.Get_Request())
        { return; }

        ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(InHandle, [&](FCk_Handle_GeometryCollection InGc)
        {
            const auto& Settings = InRequest.Get_Request();
            const auto& Request = FCk_Request_GeometryCollection_ApplyAoE{InRequest.Get_Location(), Settings->Get_Radius()}
            .Set_LinearSpeed(Settings->Get_LinearSpeed())
            .Set_AngularSpeed(Settings->Get_AngularSpeed())
            .Set_InternalStrain(Settings->Get_InternalStrain())
            .Set_ExternalStrain(Settings->Get_ExternalStrain());

            UCk_Utils_GeometryCollection_UE::Request_ApplyAoE(InGc, Request);
        });

        InHandle.AddOrGet<FFragment_GeometryCollection_ReplicationRequests>()._Requests.Emplace(InRequest);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };

    auto
        FProcessor_GeometryCollectionOwner_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_GeometryCollectionOwner_Rep>& InComp,
            const FFragment_GeometryCollection_ReplicationRequests& InRequestComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestComp, [&](FFragment_GeometryCollection_ReplicationRequests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor(overload
            {
                [&](const FCk_Request_GeometryCollection_ApplyStrain_Replicated& InRequest)
                {
                    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_GeometryCollectionOwner_Rep>(InHandle,
                    [&](UCk_Fragment_GeometryCollectionOwner_Rep* InRepComp)
                    {
                        InRepComp->Broadcast_ApplyStrain(InRequest);
                    });
                },
                [&](const FCk_Request_GeometryCollection_ApplyAoE_Replicated& InRequest)
                {
                    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_GeometryCollectionOwner_Rep>(InHandle,
                    [&](UCk_Fragment_GeometryCollectionOwner_Rep* InRepComp)
                    {
                        InRepComp->Broadcast_ApplyAoE(InRequest);
                    });
                },
            }));
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
