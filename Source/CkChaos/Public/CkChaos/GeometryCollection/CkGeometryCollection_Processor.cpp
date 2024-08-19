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
        FProcessor_GeometryCollection_CrumbleNonActiveClusters::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams) const
            -> void
    {
        InHandle.Remove<FTag_GeometryCollection_CrumbleNonAnchoredClusters>();

        const auto GeometryCollectionComponent = InParams.Get_Params().Get_GeometryCollection();

        CK_ENSURE_IF_NOT(ck::IsValid(GeometryCollectionComponent),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the Geometry Collection Component [{}] is INVALID"),
            InHandle, GeometryCollectionComponent)
        { return; }

        // If Geometry Collection is not set
        if (ck::Is_NOT_Valid(GeometryCollectionComponent->RestCollection))
        { return; }

        const auto& PhysProxy = GeometryCollectionComponent->GetPhysicsProxy();

        CK_ENSURE_IF_NOT(ck::IsValid(PhysProxy, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the of Geometry Collection Component [{}] is INVALID"),
            InHandle, GeometryCollectionComponent)
        { return; }

        auto RbdSolver = PhysProxy->GetSolver<Chaos::FPhysicsSolver>();

        CK_ENSURE_IF_NOT(ck::IsValid(RbdSolver, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the RbdSolver of Geometry Collection Component [{}] is INVALID"),
            InHandle, GeometryCollectionComponent)
        { return; }

        RbdSolver->EnqueueCommandImmediate([PhysProxy, RbdSolver]()
        {
            Chaos::FRigidClustering& Clustering = RbdSolver->GetEvolution()->GetRigidClustering();

            // Relevant comment from base code FRigidClustering::BreakClustersByProxy:
            // we should probably have a way to retrieve all the active clusters per proxy instead of having to do this iteration
            for (Chaos::FPBDRigidClusteredParticleHandle* ClusteredHandle : Clustering.GetTopLevelClusterParents())
            {
                if (ck::Is_NOT_Valid(ClusteredHandle, ck::IsValid_Policy_NullptrOnly{}))
                { continue; }

                if (ClusteredHandle->PhysicsProxy() == PhysProxy)
                {
                    if (ClusteredHandle->ObjectState() != Chaos::EObjectStateType::Kinematic)
                    { Clustering.BreakCluster(ClusteredHandle); }
                }
            }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GeometryCollection_RemoveAllAnchors::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams) const
        -> void
    {
        InHandle.Remove<FTag_GeometryCollection_RemoveAllAnchors>();

        const auto GeometryCollectionComponent = InParams.Get_Params().Get_GeometryCollection();

        CK_ENSURE_IF_NOT(ck::IsValid(GeometryCollectionComponent),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the Geometry Collection Component [{}] is INVALID"),
            InHandle, GeometryCollectionComponent)
        { return; }

        GeometryCollectionComponent->RemoveAllAnchors();
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
