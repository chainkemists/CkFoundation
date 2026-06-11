#include "CkEntityScript_WithActor_ActorComponent.h"

#include "CkEntityScript_WithActor.h"
#include "CkEntityScript_WithActor_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_WithActor_ActorComponent_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (ck::Is_NOT_Valid(_EntityScript))
    { return; }

    UCk_Utils_EntityScript_WithActor_UE::Request_SpawnEntityScript_OnActor(GetOwner(), _EntityScript);
}

// --------------------------------------------------------------------------------------------------------------------
