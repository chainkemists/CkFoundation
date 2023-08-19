#include "CkUnrealEntity_ActorProxy.h"

#include "Actor/CkActor_Utils.h"

#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkObject/CkObject_Utils.h"

#include "CkUnreal/CkUnreal_Log.h"
#include "CkUnreal/EcsBootstrapper/CkEcsBootstrapper.h"
#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"
#include "CkUnreal/Entity/CkUnrealEntity_Utils.h"

#include "Misc/TypeContainer.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_UnrealEntity_ActorProxy_UE::
ACk_UnrealEntity_ActorProxy_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;

#if WITH_EDITORONLY_DATA
    _ChildActorComponent = CreateEditorOnlyDefaultSubobject<UChildActorComponent>(TEXT("Proxy Actor Comp"));
#endif

    if (IsTemplate())
    { return; }

	_ObjectReplicator = CreateDefaultSubobject<UCk_ObjectReplicator_Component>(TEXT("Object Replicator"));
    const auto ActorInfoComp = CreateDefaultSubobject<UCk_ActorInfo_ActorComponent_UE>(TEXT("ActorInfo Component"));
}

auto ACk_UnrealEntity_ActorProxy_UE::
DoInvokeOnEntityCreated(const FCk_Handle& InCreatedEntity) -> void
{
    OnEntityCreated(InCreatedEntity);
}

void ACk_UnrealEntity_ActorProxy_UE::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisType, _ObjectReplicator, COND_None, REPNOTIFY_Always);
}

#if WITH_EDITOR
auto ACk_UnrealEntity_ActorProxy_UE::
PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    if (ck::Is_NOT_Valid(InPropertyChangedEvent.Property))
    {
        // TODO: does this need to be logged?
        return;
    }

    if (InPropertyChangedEvent.Property->GetFName() != GET_MEMBER_NAME_CHECKED(ACk_UnrealEntity_ActorProxy_UE, _UnrealEntity))
    { return; }

    _ChildActorComponent->DestroyChildActor();

    if (ck::Is_NOT_Valid(_UnrealEntity))
    {
        // TODO: veryverbose logging?
        return;
    }

    const auto UnrealEntityWithActor = Cast<UCk_UnrealEntity_ConstructionScript_WithActor_PDA>(
        _UnrealEntity->Get_EntityConstructionScript());

    CK_ENSURE_IF_NOT(ck::IsValid(UnrealEntityWithActor),
        TEXT("UnrealEntity_ActorProxy REQUIRES the Entity Construction Script to be of type "
            "UnrealEntity_ConstructionScript_WithActor.[{}]"), ck::Context(this))
    {
        _UnrealEntity = nullptr;
        return;
    }

    if (const auto& ActorClass = UnrealEntityWithActor->Get_EntityActor(); ck::IsValid(ActorClass))
    {
        _ChildActorComponent->SetChildActorClass(ActorClass);
    }
}

auto ACk_UnrealEntity_ActorProxy_UE::
BeginPlay() -> void
{
    Super::BeginPlay();

    if (ck::Is_NOT_Valid(_UnrealEntity))
    { return; }

    if (HasAuthority())
    {
        const auto NewHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Get_TransientHandle());
        GetComponentByClass<UCk_ActorInfo_ActorComponent_UE>()->_EntityHandle = NewHandle;
        UCk_Utils_ActorInfo_UE::Link(this, NewHandle);
        UCk_Utils_OwningActor_UE::Add(NewHandle, this->GetOwner(), nullptr);

        UCk_EcsBootstrapper_UE::Create(FCk_Bootstrapper_Construction_Params{this, _UnrealEntity});

    }

#if WITH_EDITORONLY_DATA
    _ChildActorComponent->DestroyChildActor();
#endif
}

auto ACk_UnrealEntity_ActorProxy_UE::
OnRep_ObjectReplicator(UCk_ObjectReplicator_Component* InObjectReplicator) -> void
{
    ck::unreal::Log(TEXT("ObjectReplicator replicated"));

    const auto NewHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Get_TransientHandle());
    GetComponentByClass<UCk_ActorInfo_ActorComponent_UE>()->_EntityHandle = NewHandle;
    UCk_Utils_ActorInfo_UE::Link(this, NewHandle);
    UCk_Utils_OwningActor_UE::Add(NewHandle, this->GetOwner(), nullptr);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
