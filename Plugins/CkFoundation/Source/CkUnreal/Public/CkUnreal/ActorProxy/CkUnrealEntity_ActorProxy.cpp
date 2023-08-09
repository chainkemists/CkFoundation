#include "CkUnrealEntity_ActorProxy.h"

#include "CkObject/CkObject_Utils.h"

#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"
#include "CkUnreal/Entity/CkUnrealEntity_Utils.h"

#include "Misc/TypeContainer.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_UnrealEntity_ActorProxy_UE::
ACk_UnrealEntity_ActorProxy_UE()
{
#if WITH_EDITORONLY_DATA
    _ChildActorComponent = CreateEditorOnlyDefaultSubobject<UChildActorComponent>(TEXT("Proxy Actor Comp"));
#endif
}

auto ACk_UnrealEntity_ActorProxy_UE::
DoInvokeOnEntityCreated(const FCk_Handle& InCreatedEntity) -> void
{
    OnEntityCreated(InCreatedEntity);
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

    const auto& InstancedUnrealEntity = UCk_Utils_Object_UE::Request_CloneObject(this, _UnrealEntity.Get());

    const auto UnrealEntityWithActor = Cast<UCk_UnrealEntity_ConstructionScript_WithActor_PDA>(
        InstancedUnrealEntity->Get_EntityConstructionScript());

    CK_ENSURE_IF_NOT(ck::IsValid(UnrealEntityWithActor), TEXT("UnrealEntity_ActorProxy REQUIRES the Entity Construction Script "
        "to be of type UnrealEntity_ConstructionScript_WithActor_PDA. Instead, it is of type [{}].[{}]"),
        InstancedUnrealEntity, ck::Context(this))
    { return; }

    UnrealEntityWithActor->_EntityInitialTransform = GetActorTransform();

    const auto& PostSpawnFunc = [this](const FCk_Handle& InCreatedEntity)
    {
        DoInvokeOnEntityCreated(InCreatedEntity);
    };

    UCk_Utils_UnrealEntity_UE::Request_Spawn(Get_TransientHandle(),
        FCk_Request_UnrealEntity_Spawn{InstancedUnrealEntity, PostSpawnFunc});

#if WITH_EDITORONLY_DATA
    _ChildActorComponent->DestroyChildActor();
#endif
}
#endif

// --------------------------------------------------------------------------------------------------------------------
