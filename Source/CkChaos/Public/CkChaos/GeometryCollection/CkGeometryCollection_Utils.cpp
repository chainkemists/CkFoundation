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
// --------------------------------------------------------------------------------------------------------------------
