#include "CkActorComponent.h"

#include <GameFramework/Actor.h>

// ----------------------------------------------------------------------------------------------------------------

FCk_ActorComponent_DoConstruct_Params::
FCk_ActorComponent_DoConstruct_Params(TObjectPtr<AActor> InActor, const FTransform& InTransform)
    : _Actor(InActor)
    , _Transform(InTransform)
{ }

// ----------------------------------------------------------------------------------------------------------------

auto
    UCk_ActorComponent_UE::
    BeginPlay()
    -> void
{
    Do_Construct_Implementation
    (
        FCk_ActorComponent_DoConstruct_Params
        {
            GetOwner(),
            GetOwner() == nullptr ? FTransform::Identity : GetOwner()->GetTransform()
        }
    );

    Super::BeginPlay();
}

auto
    UCk_ActorComponent_UE::
    Do_Construct_Implementation(const FCk_ActorComponent_DoConstruct_Params& InParams)
    -> void
{
    if (IsTemplate())
    { return; }

    Do_Construct(InParams);
}

// ----------------------------------------------------------------------------------------------------------------
