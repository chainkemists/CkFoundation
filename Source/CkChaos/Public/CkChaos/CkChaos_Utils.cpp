#include "CkChaos_Utils.h"

#include <GeometryCollection/GeometryCollectionComponent.h>
#include <PhysicsProxy/GeometryCollectionPhysicsProxy.h>

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Chaos_UE::
    Get_ClosestParticleIndexToLocation(
        const UGeometryCollectionComponent* InGeometryCollection,
        const FVector& InLocation)
    -> int32
{
    const auto GeometryCollectionComponent = InGeometryCollection;

    CK_ENSURE_IF_NOT(ck::IsValid(GeometryCollectionComponent), TEXT("Unable to iterate over clusters of GeometryCollection [{}] because its"
        " GeometryCollectionComponent [{}] is INVALID"), InGeometryCollection, GeometryCollectionComponent)
    { return {}; }

    if (ck::Is_NOT_Valid(GeometryCollectionComponent->RestCollection))
    { return {}; }

    const auto& PhysProxy = GeometryCollectionComponent->GetPhysicsProxy();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysProxy, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to iterate over clusters of GeometryCollection [{}] because its Phys Proxy of the Geometry Collection Component [{}] is INVALID"),
        InGeometryCollection, GeometryCollectionComponent)
    { return {}; }

    const auto& Particles = static_cast<TArray<Chaos::FPBDRigidParticleHandle*>>(PhysProxy->GetUnorderedParticles_Internal());

    const auto* ClosestParticle = [&]()
    {
        using namespace Chaos;

        auto ClosestChildHandle = static_cast<FPBDRigidParticleHandle*>(nullptr);

        auto ClosestSquaredDist = TNumericLimits<FReal>::Max();
        for (auto ChildHandle: Particles)
        {
            if (ck::Is_NOT_Valid(ChildHandle, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            const FReal SquaredDist = (ChildHandle->GetX() - InLocation).SizeSquared();
            if (SquaredDist < ClosestSquaredDist)
            {
                ClosestSquaredDist = SquaredDist;
                ClosestChildHandle = ChildHandle;
            }
        }

        return ClosestChildHandle;
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(ClosestParticle, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("FAILED to find any valid closest particle in Geometry Collection [{}]"),
        InGeometryCollection)
    { return {}; }

    return Particles.IndexOfByKey(ClosestParticle);
}