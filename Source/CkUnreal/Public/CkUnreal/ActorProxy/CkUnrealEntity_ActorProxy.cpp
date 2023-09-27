#include "CkUnrealEntity_ActorProxy.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_UnrealEntity_ActorProxy_UE::
ACk_UnrealEntity_ActorProxy_UE()
{
    bReplicates = true;
    bAlwaysRelevant = false;

#if WITH_EDITORONLY_DATA
    _ChildActorComponent = CreateEditorOnlyDefaultSubobject<UChildActorComponent>(TEXT("Proxy Actor Comp"));
#endif
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

    if (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ACk_UnrealEntity_ActorProxy_UE, _ActorToSpawn))
    {
        [this]() -> void
        {
            _ChildActorComponent->DestroyChildActor();

            if (ck::IsValid(_ActorToSpawn))
            {
                _ChildActorComponent->SetChildActorClass(_ActorToSpawn);
                _ChildActorComponent->GetChildActor()->bIsEditorOnlyActor = true;
            }
        }();
    }
}
#endif

auto
    ACk_UnrealEntity_ActorProxy_UE::
    BeginPlay() -> void
{
    Super::BeginPlay();

    if (ck::Is_NOT_Valid(_ActorToSpawn))
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    const auto ActorCdo = Cast<AActor>(UCk_Utils_Object_UE::Get_ClassDefaultObject(_ActorToSpawn));

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
}

void
    ACk_UnrealEntity_ActorProxy_UE::GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _SpawnedActor);
}

// --------------------------------------------------------------------------------------------------------------------
