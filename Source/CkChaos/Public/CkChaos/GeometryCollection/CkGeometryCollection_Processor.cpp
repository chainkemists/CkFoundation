#include "CkGeometryCollection_Processor.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "GeometryCollection/GeometryCollectionComponent.h"

#include "PhysicsProxy/GeometryCollectionPhysicsProxy.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_geometrycollection_processor
{
    auto
    Get_ParticlesInRadius(
        const FGeometryCollectionPhysicsProxy* Proxy,
        FVector InLocation,
        float InRadius)
    -> decltype(Proxy->GetUnorderedParticles_Internal())
    {
        using ClusterHandleType = FGeometryCollectionPhysicsProxy::FClusterHandle;

        // TODO: Do we need to call `_External` for external strains?
        const auto& Particles = Proxy->GetUnorderedParticles_Internal();

        auto ParticlesInRadius = decltype(Particles){};

        ck::algo::ForEachIsValid(Particles, [&](ClusterHandleType* InValidHandle)
        {
            if (UCk_Utils_Vector3_UE::Get_IsPointInRadius(InValidHandle->GetX(), InLocation, InRadius))
            {
                ParticlesInRadius.Emplace(InValidHandle);
            }
        }, ck::IsValid_Policy_NullptrOnly{});

        return ParticlesInRadius;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_GeometryCollection_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_GeometryCollection_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FFragment_GeometryCollection_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_GeometryCollection_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InRequest);
            }));
        });
    }

    auto
        FProcessor_GeometryCollection_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FCk_Request_GeometryCollection_ApplyStrain& InRequest)
        -> void
    {
        const auto& Proxy = InParams.Get_Params().Get_GeometryCollection()->GetPhysicsProxy();

        CK_ENSURE_IF_NOT(ck::IsValid(Proxy, ck::IsValid_Policy_NullptrOnly{}), TEXT("TODO MESSAGE"))
        { return; }

        const auto& ParticlesInRadius = ck_geometrycollection_processor::Get_ParticlesInRadius(Proxy,
            InRequest.Get_Location(), InRequest.Get_Radius());

        const auto RbdSolver = Proxy->GetSolver<Chaos::FPhysicsSolver>();
        using ClusterHandleType = FGeometryCollectionPhysicsProxy::FClusterHandle;

        ck::algo::ForEach(ParticlesInRadius, [&](ClusterHandleType* InValidHandle)
        {
            RbdSolver->EnqueueCommandImmediate([=]
            {
                Chaos::FRigidClustering& RigidClustering = RbdSolver->GetEvolution()->GetRigidClustering();

                if (const auto& ClusteredParticle = InValidHandle->CastToClustered();
                    ck::IsValid(ClusteredParticle, ck::IsValid_Policy_NullptrOnly{}))
                {
                    if (const auto& Strain = InRequest.Get_InternalStrain(); Strain > 0)
                    {
                        const auto& NewInternalStrain = ClusteredParticle->GetInternalStrains() - Strain;
                        RigidClustering.SetInternalStrain(ClusteredParticle, FMath::Max(0, NewInternalStrain));
                    }

                    if (const auto& Strain = InRequest.Get_ExternalStrain(); Strain > 0)
                    {
                        const auto& NewExternalStrain = Strain;
                        RigidClustering.SetExternalStrain(ClusteredParticle, FMath::Max(0, NewExternalStrain));
                    }

                    if (const auto& Velocity = InRequest.Get_LinearVelocity(); Velocity.SquaredLength() > 0)
                    { InValidHandle->SetV(InValidHandle->GetV() + Velocity); }

                    if (const auto& AngularVelocity = InRequest.Get_AngularVelocity(); AngularVelocity.SquaredLength() > 0)
                    { InValidHandle->SetW(InValidHandle->GetW() + AngularVelocity); }
                }
            });
        });
    }

    auto
        FProcessor_GeometryCollection_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FCk_Request_GeometryCollection_ApplyAoE& InRequest)
        -> void
    {
        const auto& Proxy = InParams.Get_Params().Get_GeometryCollection()->GetPhysicsProxy();

        CK_ENSURE_IF_NOT(ck::IsValid(Proxy, ck::IsValid_Policy_NullptrOnly{}), TEXT("TODO MESSAGE"))
        { return; }

        const auto& ParticlesInRadius = ck_geometrycollection_processor::Get_ParticlesInRadius(Proxy, InRequest.Get_Location(), InRequest.Get_Radius());

        const auto RbdSolver = Proxy->GetSolver<Chaos::FPhysicsSolver>();

        using ClusterHandleType = FGeometryCollectionPhysicsProxy::FClusterHandle;

        ck::algo::ForEach(ParticlesInRadius, [&](ClusterHandleType* Particle)
        {
            RbdSolver->EnqueueCommandImmediate([=]()
            {
                Chaos::FRigidClustering& RigidClustering = RbdSolver->GetEvolution()->GetRigidClustering();

                if (const auto& ClusteredParticle = Particle->CastToClustered())
                {
                    auto Direction = Particle->GetTransformPQ().GetLocation() - InRequest.Get_Location();
                    const auto& Distance = Direction.Length();
                    Direction.Normalize();

                    if (const auto& Strain = InRequest.Get_ExternalStrain(); Strain > 0)
                    {
                        const auto& NewStrain = ClusteredParticle->GetExternalStrain() - Strain;
                        RigidClustering.SetExternalStrain(ClusteredParticle,
                            FMath::Max(ClusteredParticle->GetExternalStrain(), NewStrain));
                    }

                    if (const auto& Strain = InRequest.Get_InternalStrain(); Strain > 0)
                    {
                        const Chaos::FRealSingle NewInternalStrain = ClusteredParticle->GetInternalStrains() - Strain;
                        RigidClustering.SetInternalStrain(ClusteredParticle,
                            FMath::Max(0, NewInternalStrain));
                    }

                    if (const auto& Speed = InRequest.Get_LinearSpeed(); Speed.SquaredLength() > 0)
                    { Particle->SetV(Particle->GetV() + Direction * Speed); }

                    if (const auto& Speed = InRequest.Get_AngularSpeed(); Speed.SquaredLength() > 0)
                    { Particle->SetW(Particle->GetW() + Direction * Speed); }
                }
            });
        });
    }

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
