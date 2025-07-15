#include "CkGeometryCollection_Processor.h"

#include <PhysicsProxy/GeometryCollectionPhysicsProxy.h>

#include "CkChaos/GeometryCollection/CkGeometryCollection_Utils.h"
#include "CkChaos/Settings/CkChaos_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Math/FloatCurve/CkFloatCurve_Utils.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcs/Transform/CkTransform_Utils.h"

#include "GeometryCollection/GeometryCollectionComponent.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_geometrycollection_processor
{
    auto
    Get_ParticlesInRadialShell(
        const FGeometryCollectionPhysicsProxy* Proxy,
        const FVector& InLocation,
        const float InRadiusMin,
        const float InRadiusMax)
    -> decltype(Proxy->GetUnorderedParticles_Internal())
    {
        using ClusterHandleType = FGeometryCollectionPhysicsProxy::FClusterHandle;

        // TODO: Do we need to call `_External` for external strains?
        const auto& Particles = Proxy->GetUnorderedParticles_Internal();

        auto ParticlesInRadius = ck::algo::Filter(Particles,
        [&](ClusterHandleType* InValidHandle)
        {
            if (ck::Is_NOT_Valid(InValidHandle, ck::IsValid_Policy_NullptrOnly{}))
            { return false; }

            const auto InMinRadius = UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(InRadiusMin, 0.0f) ? false :
                UCk_Utils_Vector3_UE::Get_IsPointInRadius(InValidHandle->GetX(), InLocation, InRadiusMin);
            const auto InMaxRadius = UCk_Utils_Vector3_UE::Get_IsPointInRadius(InValidHandle->GetX(), InLocation, InRadiusMax);

            return InMaxRadius && NOT InMinRadius;
        });

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

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_GeometryCollection_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FCk_Request_GeometryCollection_ApplyRadialStrain& InRequest)
        -> void
    {
        const auto& Proxy = InParams.Get_Params().Get_GeometryCollection()->GetPhysicsProxy();

        CK_ENSURE_IF_NOT(ck::IsValid(Proxy, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Unable to ApplyRadialStrain to GeometryCollection [{}] because the PhysProxy of Geometry Collection Component [{}] is INVALID"),
            InHandle, InParams.Get_Params().Get_GeometryCollection())
        { return; }

        CK_ENSURE_IF_NOT(InRequest.Get_Radius() > 0.0f,
            TEXT("Unable to ApplyRadialStrain to GeometryCollection [{}] because the radius is zero"),
            InHandle)
        { return; }

        const auto MaximumRadiusToUse = UCk_Utils_Chaos_Settings_UE::Get_MaximumRadialDamageDeltaRadiusPerFrame();
        auto MaybeNewRequest = InRequest;

        const auto MinRadius = MaybeNewRequest.Get_IncrementalRadius();
        const auto FinalRadius = InRequest.Get_Radius();
        MaybeNewRequest.Set_IncrementalRadius(MaybeNewRequest.Get_IncrementalRadius() + MaximumRadiusToUse);
        const auto& CurrentRadius = MaybeNewRequest.Get_IncrementalRadius() < FinalRadius ? MaybeNewRequest.Get_IncrementalRadius() : FinalRadius;
        const auto& ParticlesInRadius = ck_geometrycollection_processor::Get_ParticlesInRadialShell(Proxy, InRequest.Get_Location(), MinRadius, CurrentRadius);

        const auto RbdSolver = Proxy->GetSolver<Chaos::FPhysicsSolver>();

        using ClusterHandleType = FGeometryCollectionPhysicsProxy::FClusterHandle;

        ck::algo::ForEach(ParticlesInRadius, [&](ClusterHandleType* Particle)
        {
            RbdSolver->EnqueueCommandImmediate([=]()
            {
                Chaos::FRigidClustering& RigidClustering = RbdSolver->GetEvolution()->GetRigidClustering();

                const auto& ClusteredParticle = Particle->CastToClustered();

                if (ck::Is_NOT_Valid(ClusteredParticle, ck::IsValid_Policy_NullptrOnly{}))
                { return; }

                switch(InRequest.Get_ChangeParticleStateTo())
                {
                    case ECk_GeometryCollection_ObjectState::Uninitialized: Particle->SetObjectStateLowLevel(Chaos::EObjectStateType::Uninitialized); break;
                    case ECk_GeometryCollection_ObjectState::Sleeping:      Particle->SetObjectStateLowLevel(Chaos::EObjectStateType::Sleeping); break;
                    case ECk_GeometryCollection_ObjectState::Kinematic:     Particle->SetObjectStateLowLevel(Chaos::EObjectStateType::Kinematic); break;
                    case ECk_GeometryCollection_ObjectState::Static:        Particle->SetObjectStateLowLevel(Chaos::EObjectStateType::Static); break;
                    case ECk_GeometryCollection_ObjectState::Dynamic:       Particle->SetObjectStateLowLevel(Chaos::EObjectStateType::Dynamic); break;
                    case ECk_GeometryCollection_ObjectState::NoChange: break;
                }

                auto Direction = Particle->GetTransformPQ().GetLocation() - InRequest.Get_Location();
                const auto NormalizedDistance = Direction.Length() / FinalRadius;
                Direction.Normalize();

                const auto IsValidCurve = InRequest.Get_NormalizedFalloffCurve().GetRichCurveConst() &&
                    NOT InRequest.Get_NormalizedFalloffCurve().GetRichCurveConst()->Keys.IsEmpty();

                const auto FalloffScale = IsValidCurve ?
                    UCk_Utils_RuntimeFloatCurve_UE::Get_ValueAtTime(InRequest.Get_NormalizedFalloffCurve(), FCk_Time(NormalizedDistance)) : 1.f;

                if (const auto& Strain = InRequest.Get_ExternalStrain() * FalloffScale;
                    Strain > 0.0f)
                {
                    const auto& NewStrain = Strain;
                    RigidClustering.SetExternalStrain(ClusteredParticle, FMath::Max(ClusteredParticle->GetExternalStrain(), NewStrain));
                }

                if (const auto& Strain = InRequest.Get_InternalStrain() * FalloffScale;
                    Strain > 0.0f)
                {
                    const Chaos::FRealSingle NewInternalStrain = ClusteredParticle->GetInternalStrains() - Strain;
                    RigidClustering.SetInternalStrain(ClusteredParticle, FMath::Max(0.0f, NewInternalStrain));
                }

                if (const auto& Speed = InRequest.Get_LinearSpeed() * FalloffScale;
                    Speed > 0.0f)
                { Particle->SetV(Direction * Speed); }

                if (const auto& Speed = InRequest.Get_AngularSpeed() * FalloffScale;
                    Speed > 0.0f)
                { Particle->SetW(Direction * Speed); }
            });
        });

        if (MaybeNewRequest.Get_IncrementalRadius() < InRequest.Get_Radius())
        {
            UCk_Utils_GeometryCollection_UE::Request_ApplyRadialStrain(InHandle, MaybeNewRequest);
        }
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

        CK_ENSURE_IF_NOT(ck::IsValid(GeometryCollectionComponent->RestCollection),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the Geometry Collection Component [{}] RestCollection [{}] is INVALID"),
            InHandle, GeometryCollectionComponent, GeometryCollectionComponent->RestCollection)
        { return; }

        const auto& PhysProxy = GeometryCollectionComponent->GetPhysicsProxy();

        CK_ENSURE_IF_NOT(ck::IsValid(PhysProxy, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the Physics Proxy of the Geometry Collection Component [{}]"),
            InHandle, GeometryCollectionComponent)
        { return; }

        auto RbdSolver = PhysProxy->GetSolver<Chaos::FPhysicsSolver>();

        CK_ENSURE_IF_NOT(ck::IsValid(RbdSolver, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Unable to CrumbleNonActiveClusters of GeometryCollection [{}] because the RbdSolver of the Geometry Collection Component [{}] is INVALID"),
            InHandle, GeometryCollectionComponent)
        { return; }

        RbdSolver->EnqueueCommandImmediate([PhysProxy, RbdSolver]()
        {
            Chaos::FRigidClustering& Clustering = RbdSolver->GetEvolution()->GetRigidClustering();

            // Relevant comment from base code FRigidClustering::BreakClustersByProxy:
            // we should probably have a way to retrieve all the active clusters per proxy instead of having to do this iteration
            ck::algo::ForEachIsValid(Clustering.GetTopLevelClusterParents(), [&](Chaos::FPBDRigidClusteredParticleHandle* ClusteredHandle)
            {
                if (ClusteredHandle->PhysicsProxy() == PhysProxy)
                {
                    if (ClusteredHandle->ObjectState() != Chaos::EObjectStateType::Kinematic)
                    { Clustering.BreakCluster(ClusteredHandle); }
                }
            }, ck::IsValid_Policy_NullptrOnly{});
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
        InHandle.Remove<MarkedDirtyBy>();

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