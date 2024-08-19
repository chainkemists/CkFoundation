#include "CkGeometryCollection_Utils.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment.h"
#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Fragment.h"

#include <GeometryCollection/GeometryCollectionComponent.h>
#include <PhysicsProxy/GeometryCollectionPhysicsProxy.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GeometryCollection_UE::
    Add(
        FCk_Handle_GeometryCollectionOwner& InOwner,
        const FCk_Fragment_GeometryCollection_ParamsData& InParams)
    -> FCk_Handle_GeometryCollection
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_GeometryCollection()),
        TEXT("Unable to add new GeometryCollection to [{}] because the GeometryCollection [{}] is INVALID"),
        InOwner, InParams.Get_GeometryCollection())
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner, [&](FCk_Handle InNewEntity)
    {
        InNewEntity.Add<ck::FFragment_GeometryCollection_Params>(InParams);
        InNewEntity.Add<ck::FFragment_GeometryCollection_Current>();
    });

    auto NewGeometryCollectionEntity = Cast(NewEntity);

    ck::FUtils_RecordOfGeometryCollections::Request_Connect(InOwner, NewGeometryCollectionEntity);

    return NewGeometryCollectionEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Chaos, UCk_Utils_GeometryCollection_UE, FCk_Handle_GeometryCollection,
    ck::FFragment_GeometryCollection_Params, ck::FFragment_GeometryCollection_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GeometryCollection_UE::
    Get_AreAllClustersAnchored(
        const FCk_Handle_GeometryCollection& InGeometryCollection)
    -> bool
{
    const auto GeometryCollectionComponent = Get_GeometryCollectionComponent(InGeometryCollection);

    CK_ENSURE_IF_NOT(ck::IsValid(GeometryCollectionComponent), TEXT("Unable to iterate over clusters of GeometryCollection [{}] because its"
        " GeometryCollectionComponent [{}] is INVALID"), InGeometryCollection, GeometryCollectionComponent)
    { return {}; }

    if (ck::Is_NOT_Valid(GeometryCollectionComponent->RestCollection))
    { return true; }

    const auto& PhysProxy = GeometryCollectionComponent->GetPhysicsProxy();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysProxy, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to iterate over clusters of GeometryCollection [{}] because its Phys Proxy of the Geometry Collection Component [{}] is INVALID"),
        InGeometryCollection, GeometryCollectionComponent)
    { return true; }

    auto RbdSolver = PhysProxy->GetSolver<Chaos::FPhysicsSolver>();

    CK_ENSURE_IF_NOT(ck::IsValid(RbdSolver, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to iterate over clusters of GeometryCollection [{}] because its RbdSolver of the Geometry Collection Component [{}] is INVALID"),
        InGeometryCollection, GeometryCollectionComponent)
    { return true; }

    // Relevant comment from base code FRigidClustering::BreakClustersByProxy:
    // we should probably have a way to retrieve all the active clusters per proxy instead of having to do this iteration
    for (auto& Clustering = RbdSolver->GetEvolution()->GetRigidClustering();
         const auto ClusteredHandle : Clustering.GetTopLevelClusterParents())
    {
        if (ck::Is_NOT_Valid(ClusteredHandle, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        if (ClusteredHandle->PhysicsProxy() == PhysProxy)
        {
            if (ClusteredHandle->ObjectState() != Chaos::EObjectStateType::Kinematic)
            { return false; }
        }
    }
    return true;
}

auto
    UCk_Utils_GeometryCollection_UE::
    Get_GeometryCollectionComponent(
        const FCk_Handle_GeometryCollection& InGeometryCollection)
    -> UGeometryCollectionComponent*
{
    const auto& Ptr = InGeometryCollection.Get<ck::FFragment_GeometryCollection_Params>().Get_Params().Get_GeometryCollection();

    if (ck::Is_NOT_Valid(Ptr))
    { return nullptr; }

    return Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GeometryCollection_UE::
    Request_ApplyStrainAndVelocity(
        FCk_Handle_GeometryCollection& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyStrain& InRequest)
    -> FCk_Handle_GeometryCollection
{
    InGeometryCollection.AddOrGet<ck::FFragment_GeometryCollection_Requests>()._Requests.Emplace(InRequest);
    return InGeometryCollection;
}

auto
    UCk_Utils_GeometryCollection_UE::
    Request_ApplyAoE(
        FCk_Handle_GeometryCollection& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyAoE& InRequest)
    -> FCk_Handle_GeometryCollection
{
    InGeometryCollection.AddOrGet<ck::FFragment_GeometryCollection_Requests>()._Requests.Emplace(InRequest);
    return InGeometryCollection;
}

auto
    UCk_Utils_GeometryCollection_UE::
    Request_CrumbleNonAnchoredClusters(
        FCk_Handle_GeometryCollection& InGeometryCollection)
    -> FCk_Handle_GeometryCollection
{
    InGeometryCollection.AddOrGet<ck::FTag_GeometryCollection_CrumbleNonAnchoredClusters>();
    return InGeometryCollection;
}

auto
    UCk_Utils_GeometryCollection_UE::
    Request_RemoveAllAnchors(
        FCk_Handle_GeometryCollection& InGeometryCollection)
    -> FCk_Handle_GeometryCollection
{
    InGeometryCollection.AddOrGet<ck::FTag_GeometryCollection_RemoveAllAnchors>();
    return InGeometryCollection;
}

auto
    UCk_Utils_GeometryCollection_UE::
    Request_RemoveAllAnchorsAndCrumbleNonAnchoredClusters(
        FCk_Handle_GeometryCollection& InGeometryCollection)
    -> FCk_Handle_GeometryCollection
{
    Request_CrumbleNonAnchoredClusters(InGeometryCollection);
    Request_RemoveAllAnchors(InGeometryCollection);
    return InGeometryCollection;
}

// --------------------------------------------------------------------------------------------------------------------
