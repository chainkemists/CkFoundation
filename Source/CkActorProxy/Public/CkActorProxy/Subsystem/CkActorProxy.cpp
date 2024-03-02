#include "CkActorProxy.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

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
    {
        _SpawnedActor->Destroy();
        _SpawnedActor = nullptr;
    }
}

auto
    ACk_ActorProxy_UE::
    Destroyed()
    -> void
{
    Super::Destroyed();

    if (ck::IsValid(_SpawnedActor))
    {
        _SpawnedActor->Destroy();
        _SpawnedActor = nullptr;
    }
}

#if WITH_EDITOR
auto
    ACk_ActorProxy_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    if (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ACk_ActorProxy_UE, _ActorToSpawn))
    {
        auto ChangeLabel = false;

        DoSpawnActor();

        if (ChangeLabel && ck::IsValid(_SpawnedActor))
        {
            UCk_Utils_Actor_UE::Request_SetActorLabel(this, _SpawnedActor->GetActorNameOrLabel(), false);
        }
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
    auto DoChangeLabel = [this](const AActor* InActor)
    {
        UCk_Utils_Actor_UE::Request_SetActorLabel(this, InActor->GetActorNameOrLabel() + TEXT("__PROXY"), false);
    };

    if (ck::IsValid(_SpawnedActor))
    {
        if (_ActorToSpawn != _SpawnedActor->GetClass())
        {
            _SpawnedActor->Destroy();
            _SpawnedActor = nullptr;
            DoSpawnActor();

            if (ck::IsValid(_SpawnedActor))
            { DoChangeLabel(_SpawnedActor.Get()); }
            else
            {
                UCk_Utils_Actor_UE::Request_SetActorLabel(this, this->GetActorNameOrLabel() + TEXT("_INVALID"), false);
            }

            return;
        }

        _SpawnedActor->SetActorTransform(this->GetActorTransform());
    }
    else
    {
        if (GetWorld()->IsNetMode(NM_Client) && _ActorToSpawnIsReplicated)
        { return; }

        if (ck::Is_NOT_Valid(_ActorToSpawn))
        { return; }

        auto Params = FActorSpawnParameters{};
#if WITH_EDITOR
        Params.bHideFromSceneOutliner = true;
#endif
        Params.ObjectFlags = RF_Transient;

        _SpawnedActor = GetWorld()->SpawnActor(_ActorToSpawn, &GetActorTransform(), Params);
        _ActorToSpawnIsReplicated = _SpawnedActor->GetIsReplicated();

#if WITH_EDITOR
        // the following disables the Actor from being deleted OR from being moved directly (the ActorProxy can still move the Actor)
        SaveToTransactionBuffer(_SpawnedActor.Get(), false);
        _SpawnedActor->SetLockLocation(true);
        DoChangeLabel(_SpawnedActor.Get());
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
