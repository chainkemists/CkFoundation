#include "CkActorProxy_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "Components/BillboardComponent.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_ActorProxy_UE::
    ACk_ActorProxy_UE()
{
    if (ck::Is_NOT_Valid(GetWorld()))
    { return; }

    bAlwaysRelevant = false;
}

#if WITH_EDITOR
auto
    ACk_ActorProxy_UE::
    OnConstruction(
        const FTransform& Transform)
    -> void
{
    Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
    if (ck::IsValid(_Icon))
    { GetSpriteComponent()->Sprite = _Icon; }
#endif

    DoSpawnActor();
}

auto
    ACk_ActorProxy_UE::
    BeginDestroy()
    -> void
{
    Super::BeginDestroy();

    if (ck::IsValid(_SpawnedActor))
    { _SpawnedActor->Destroy(); }
}

auto
    ACk_ActorProxy_UE::
    Destroyed()
    -> void
{
    Super::Destroyed();

    if (ck::IsValid(_SpawnedActor))
    { _SpawnedActor->Destroy(); }
}

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
        if (ck::IsValid(_SpawnedActor))
        { _SpawnedActor->Destroy(); }

        DoSpawnActor();
    }
}
#endif

auto
    ACk_ActorProxy_UE::
    BeginPlay()
    -> void
{
    const auto World = GetWorld();

    if (GetWorld()->IsGameWorld())
    { DoSpawnActor(); }

    Super::BeginPlay();
}

auto
    ACk_ActorProxy_UE::
    DoSpawnActor()
    -> void
{
    if (ck::IsValid(_SpawnedActor))
    {
        _SpawnedActor->SetActorTransform(this->GetActorTransform());
    }
    else
    {
        if (GetWorld()->IsNetMode(NM_Client) && _ActorToSpawnIsReplicated)
        { return; }

        if (ck::Is_NOT_Valid(_ActorToSpawn))
        { return; }

        auto Params = FActorSpawnParameters{};
        Params.bHideFromSceneOutliner = true;
        Params.ObjectFlags = RF_Transient;

        _SpawnedActor = GetWorld()->SpawnActor(_ActorToSpawn, &GetActorTransform(), Params);
        _ActorToSpawnIsReplicated = _SpawnedActor->GetIsReplicated();

#if WITH_EDITOR
        // the following disables the Actor from being deleted OR from being moved directly (the ActorProxy can still move the Actor)
        SaveToTransactionBuffer(_SpawnedActor.Get(), false);
        _SpawnedActor->SetLockLocation(true);
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
