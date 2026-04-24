#include "CkEntitySpawner_Actor.h"

#include "CkEntitySpawner/CkEntitySpawner_Fragment.h"
#include "CkEntitySpawner/CkEntitySpawner_Log.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include <Components/BillboardComponent.h>
#include <Components/SceneComponent.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_EntitySpawner_UE::
    ACk_EntitySpawner_UE()
{
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = false;
    bAlwaysRelevant = false;
    SetCanBeDamaged(false);

    _ReplicatedChannelGroup = UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Generic"));

    auto SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
    if (auto Sprite = GetSpriteComponent();
        ck::IsValid(Sprite))
    {
        Sprite->SetupAttachment(RootComponent);
    }
#endif
}

auto
    ACk_EntitySpawner_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    DoSpawnEntity();
}

#if WITH_EDITOR
auto
    ACk_EntitySpawner_UE::
    EditorOnly_InitializeEntityScript(
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass)
    -> void
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return; }

    _EntityScript = NewObject<UCk_EntityScript_UE>(this, InEntityScriptClass, NAME_None, RF_Transactional);
}
#endif

auto
    ACk_EntitySpawner_UE::
    DoSpawnEntity()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_EntityScript),
        TEXT("EntitySpawner [{}] has no EntityScript assigned."), this)
    { return; }

    const auto Replication = _EntityScript->Get_Replication();
    const auto IsReplicated = Replication == ECk_Replication::Replicates;

    if (IsReplicated)
    {
        if (NOT HasAuthority())
        { return; }

        CK_ENSURE_IF_NOT(_ReplicatedChannelGroup.IsValid(),
            TEXT("EntitySpawner [{}] has an invalid ReplicatedChannelGroup tag."), this)
        { return; }
    }

    auto PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(this);

    CK_ENSURE_IF_NOT(ck::IsValid(PendingEntity),
        TEXT("EntitySpawner [{}] could not create a pending-spawn entity."), this)
    { return; }

    PendingEntity.Add<ck::FFragment_EntitySpawner_PendingSpawn>(
        _EntityScript,
        IsReplicated ? _ReplicatedChannelGroup : FGameplayTag::EmptyTag);
}

// --------------------------------------------------------------------------------------------------------------------
