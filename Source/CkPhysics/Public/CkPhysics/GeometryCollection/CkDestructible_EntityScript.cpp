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
    ConstructWithActor(
        FCk_Handle& InHandle,
        AActor* InOwningActor)
    -> ECk_EntityScript_ConstructionFlow
{
    UCk_Utils_GeometryCollectionOwner_UE::Add(InHandle);

    return ECk_EntityScript_ConstructionFlow::Finished;
}

// --------------------------------------------------------------------------------------------------------------------
