#include "CkEntityReplicationChannel_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Label_EntityReplicationChannel, TEXT("EntityReplicationChannel"));

// --------------------------------------------------------------------------------------------------------------------

ACk_EcsChannel_Actor_UE::
    ACk_EcsChannel_Actor_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    this->PrimaryActorTick.bStartWithTickEnabled = false;
    this->PrimaryActorTick.bCanEverTick = false;
    this->PrimaryActorTick.bTickEvenWhenPaused = false;
}

// --------------------------------------------------------------------------------------------------------------------
