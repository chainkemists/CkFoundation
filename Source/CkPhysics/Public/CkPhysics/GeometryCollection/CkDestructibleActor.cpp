#include "CkDestructibleActor.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_Destructible::
    ACk_Destructible(
        const FObjectInitializer& InObjectInitializer)
{
    // The following settings are crucial for the replication of the geometry collection
    // TODO: We definitely do NOT want destructible Actors to always be relevant. For now, we just want things to work.
    bAlwaysRelevant = true;
    bReplicates = true;
    bNetLoadOnClient = true;
    SetReplicatingMovement(true);

    // Optimized network settings to reduce the network load of replicated destruction
    NetUpdateFrequency = 250.0f;
    MinNetUpdateFrequency = 10.0f;
    NetPriority = 0.01f; // destruction should be the lowest priority

    bReplicateUsingRegisteredSubObjectList = true;

    _GeometryCollection = CreateDefaultSubobject<UCk_GeometryCollectionComponent>(TEXT("CkGeometryCollection"));
    _GeometryCollection->Request_EnableAsyncPhysics();
}
