#include "CkDestructibleAnchor.h"

UCk_DestructibleAnchorField_ActorComponent_UE::
    UCk_DestructibleAnchorField_ActorComponent_UE()
        : Super()
{
}

// --------------------------------------------------------------------------------------------------------------------

UCk_DestructibleAnchor_ActorComponent_UE::
    UCk_DestructibleAnchor_ActorComponent_UE()
        : Super()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
    bHiddenInGame = true;
    bUseDefaultCollision = false;
}
