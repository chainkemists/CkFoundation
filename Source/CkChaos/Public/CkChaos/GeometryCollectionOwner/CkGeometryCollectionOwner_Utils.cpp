#include "CkGeometryCollectionOwner_Utils.h"

#include "CkChaos/CkChaos_Log.h"
#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment.h"
#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_GeometryCollectionOwner_UE::
    Add(
        FCk_Handle InHandle,
        ECk_Replication InReplicates)
    -> FCk_Handle_GeometryCollectionOwner
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Unable to add [{}] as a GeometryCollectionOwner since it does NOT have an owning Actor"), InHandle)
    { return {}; }

    ck::FUtils_RecordOfGeometryCollections::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
    InHandle.Add<ck::FTag_GeometryCollectionOwner_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::chaos::VeryVerbose
        (
            TEXT("Skipping creation of Ability Owner Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_GeometryCollectionOwner_Rep>(InHandle);
    }

    return Cast(InHandle);
}

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(GeometryCollectionOwner,
    UCk_Utils_GeometryCollectionOwner_UE,
    FCk_Handle_GeometryCollectionOwner,
    ck::FFragment_RecordOfGeometryCollections)

auto
    UCk_Utils_GeometryCollectionOwner_UE::
    ForEach_GeometryCollection(
        FCk_Handle_GeometryCollectionOwner& InOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_GeometryCollection>
{
    auto GCs = TArray<FCk_Handle_GeometryCollection>{};

    ForEach_GeometryCollection(InOwner, [&](FCk_Handle_GeometryCollection InGeometryCollection)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InGeometryCollection, InOptionalPayload); }
        else
        { GCs.Emplace(InGeometryCollection); }
    });

    return GCs;
}

auto
    UCk_Utils_GeometryCollectionOwner_UE::
    ForEach_GeometryCollection(
        FCk_Handle_GeometryCollectionOwner& InOwner,
        const TFunction<void(FCk_Handle_GeometryCollection)>& InFunc)
    -> void
{
    ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(InOwner, InFunc);
}

auto
    UCk_Utils_GeometryCollectionOwner_UE::
    Request_ApplyStrainAndVelocity(
        FCk_Handle_GeometryCollectionOwner& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyStrain_Replicated& InRequest)
    -> FCk_Handle_GeometryCollectionOwner
{
    InGeometryCollection.AddOrGet<ck::FFragment_GeometryCollectionOwner_Requests>()._Requests.Emplace(InRequest);
    return InGeometryCollection;
}

auto
    UCk_Utils_GeometryCollectionOwner_UE::
    Request_ApplyAoE(
        FCk_Handle_GeometryCollectionOwner& InGeometryCollection,
        const FCk_Request_GeometryCollection_ApplyAoE_Replicated& InRequest)
    -> FCk_Handle_GeometryCollectionOwner
{
    InGeometryCollection.AddOrGet<ck::FFragment_GeometryCollectionOwner_Requests>()._Requests.Emplace(InRequest);
    return InGeometryCollection;
}

// --------------------------------------------------------------------------------------------------------------------
