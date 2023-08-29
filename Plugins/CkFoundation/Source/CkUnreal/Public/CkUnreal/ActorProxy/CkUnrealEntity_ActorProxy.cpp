#include "CkUnrealEntity_ActorProxy.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"

#include "CkUnreal/CkUnreal_Log.h"
#include "CkUnreal/EcsBootstrapper/CkEcsBootstrapper.h"
#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"
#include "CkUnreal/Entity/CkUnrealEntity_Utils.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_UnrealEntity_ActorProxy_UE::
ACk_UnrealEntity_ActorProxy_UE()
{
    bReplicates = false;
    bAlwaysRelevant = false;
}

#if WITH_EDITOR
auto
    ACk_UnrealEntity_ActorProxy_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    if (ck::Is_NOT_Valid(InPropertyChangedEvent.Property))
    {
        // TODO: does this need to be logged?
        return;
    }

    if (InPropertyChangedEvent.Property->GetFName() != GET_MEMBER_NAME_CHECKED(ACk_UnrealEntity_ActorProxy_UE, _ActorToSpawn))
    { return; }

    _TransientActor = UCk_Utils_Actor_UE::Request_SpawnActor
    (
        UCk_Utils_Actor_UE::SpawnActorParamsType{this, _ActorToSpawn}.Set_SpawnTransform(GetActorTransform())
    );
    _TransientActor->bIsEditorOnlyActor = true;
}

auto
    ACk_UnrealEntity_ActorProxy_UE::
    BeginPlay() -> void
{
    Super::BeginPlay();

    if (ck::Is_NOT_Valid(_ActorToSpawn))
    { return; }

    const auto NewActor = UCk_Utils_Actor_UE::Request_SpawnActor
    (
        UCk_Utils_Actor_UE::SpawnActorParamsType{this, _ActorToSpawn}.Set_SpawnTransform(GetActorTransform())
    );
}

#endif

// --------------------------------------------------------------------------------------------------------------------
