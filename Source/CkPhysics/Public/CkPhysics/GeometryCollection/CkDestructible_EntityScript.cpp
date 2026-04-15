#include "CkDestructible_EntityScript.h"

#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_EntityScript_Destructible_UE::
    UCk_EntityScript_Destructible_UE()
{
    _Replication = ECk_Replication::DoesNotReplicate;
}

auto
    UCk_EntityScript_Destructible_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Flow = Super::Construct(InHandle, InSpawnParams);

    UCk_Utils_GeometryCollectionOwner_UE::Add(InHandle);

    return Flow;
}

// --------------------------------------------------------------------------------------------------------------------
