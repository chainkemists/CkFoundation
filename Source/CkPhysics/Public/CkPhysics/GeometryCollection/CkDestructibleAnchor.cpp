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

// --------------------------------------------------------------------------------------------------------------------

UCk_UniformKinematic::
    UCk_UniformKinematic()
        : Super()
{
    Magnitude = 2.0f; //EObjectStateTypeEnum::Chaos_Object_Kinematic
}
