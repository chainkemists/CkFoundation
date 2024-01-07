#include "CkActorProxy_Subsystem.h"

#include "CkActorProxy/CkActorProxy_Log.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include <Engine/World.h>
#include "Net/UnrealNetwork.h"
#include <EngineUtils.h>
#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_ActorProxy_UE::
ACk_ActorProxy_UE()
{
    bReplicates = true;
    bAlwaysRelevant = false;

#if WITH_EDITORONLY_DATA
    _ChildActorComponent = CreateEditorOnlyDefaultSubobject<UChildActorComponent>(TEXT("Proxy Actor Comp"));
#endif
}

#if WITH_EDITOR
auto
    ACk_ActorProxy_UE::
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

    if (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ACk_ActorProxy_UE, _ActorToSpawn))
    {
        [this]() -> void
        {
            _ChildActorComponent->DestroyChildActor();

            if (ck::IsValid(_ActorToSpawn))
            {
                _ChildActorComponent->SetChildActorClass(_ActorToSpawn);
            }
        }();
    }
}
#endif

auto
    ACk_ActorProxy_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (ck::Is_NOT_Valid(_ActorToSpawn))
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    const auto ActorCdo = UCk_Utils_Object_UE::Get_ClassDefaultObject<AActor>(_ActorToSpawn);

    CK_ENSURE_IF_NOT(ck::IsValid(ActorCdo),
        TEXT("Could not get a valid Class Default Object for [{}] OR could not cast it to AActor.[{}]"),
        _ActorToSpawn, ck::Context(this))
    { return; }

    _SpawnedActor = UCk_Utils_Actor_UE::Request_SpawnActor
    (
        UCk_Utils_Actor_UE::SpawnActorParamsType{this, _ActorToSpawn}
        .Set_NetworkingType(ActorCdo->GetIsReplicated()
            ? ECk_Actor_NetworkingType::Replicated
            : ECk_Actor_NetworkingType::Local)
        .Set_SpawnTransform(GetActorTransform())
    );

    ck::actor_proxy::VeryVerboseIf(ck::IsValid(_SpawnedActor), TEXT("Successfully Spawned Actor [{}].[{}]"), _SpawnedActor, ck::Context(this));
    ck::actor_proxy::VeryVerboseIf(ck::Is_NOT_Valid(_SpawnedActor), TEXT("Could NOT Spawn Actor of Class [{}].[{}]"), _ActorToSpawn, ck::Context(this));
}

auto
    ACk_ActorProxy_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _SpawnedActor);
}

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
    for (auto ActorItr = TActorIterator<ACk_ActorProxy_UE>{GetWorld()}; ActorItr; ++ActorItr)
    {
        ActorItr->_ChildActorComponent->DestroyChildActor();
        ActorItr->_ChildActorComponent->SetChildActorClass(nullptr);

        ck::actor_proxy::VeryVerbose(TEXT("Destroying ChildActorComponent [{}] of ActorProxy [{}].[{}]"),
            ActorItr->_ChildActorComponent->GetChildActor(), *ActorItr, ck::Context(this));
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
