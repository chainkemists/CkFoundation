#include "CkUnrealEntity_ActorProxy_Subsystem.h"

#include "CkUnreal/ActorProxy/CkUnrealEntity_ActorProxy.h"

#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

/*
 * Our ActorProxies create temporary Actors only for the Editor and only when not in PIE. We want to destroy these Actors
 * before any side-effects happen. Unfortunately, we do not know when these Actors will be available for destruction. If
 * the `DestroyChildActor` is called too late in the Actor life-cycle, then the Actor will call BeginPlay and result
 * in code being run on an Actor that was supposed to be Editor only.
 *
 * Thus, we call the destruction code in ALL identified paths (i.e. the paths that we have identified as needed during
 * the Actor construction to intercept the construction and destroy it) in the World SubSystem to make sure that the
 * temporary Actors do not have a chance to call their respective BeginPlay.
 *
 * Extra notes:
 * - It's interesting to note that OnWorldBeginPlay, in Epic's code, has the comment (see header) where the function is
 * guaranteed to be called _before_ BeginPlay of all Actors is called. It's possible that it's too late at this point
 * for _some_ Actors as we have no guarantees on Actor ordering.
 */

auto
    UCk_ActorProxy_Subsystem_UE::
    DoDestroyAllActorProxyChildActors() const
    -> void
{
#if WITH_EDITORONLY_DATA
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACk_UnrealEntity_ActorProxy_UE::StaticClass(), FoundActors);

    for (const auto Actor : FoundActors)
    {
        const auto ActorProxy = Cast<ACk_UnrealEntity_ActorProxy_UE>(Actor);
        ActorProxy->_ChildActorComponent->DestroyChildActor();
    }
#endif
}

void
    UCk_ActorProxy_Subsystem_UE::PostInitialize()
{
    // see notes above on why this is called multiple times
    DoDestroyAllActorProxyChildActors();
    Super::PostInitialize();
}

void
    UCk_ActorProxy_Subsystem_UE::Initialize(
        FSubsystemCollectionBase& Collection)
{
    // see notes above on why this is called multiple times
    DoDestroyAllActorProxyChildActors();
    Super::Initialize(Collection);
}

auto
    UCk_ActorProxy_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    // see notes above on why this is called multiple times
    DoDestroyAllActorProxyChildActors();
    Super::OnWorldBeginPlay(InWorld);
}

// --------------------------------------------------------------------------------------------------------------------
