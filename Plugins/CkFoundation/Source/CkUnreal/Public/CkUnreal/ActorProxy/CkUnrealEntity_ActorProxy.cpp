//#include "CkUnrealEntity_ActorProxy.h"
//
//#include "CkCore/Actor/CkActor_Utils.h"
//#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
//
//#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
//#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"
//
//#include "CkUnreal/CkUnreal_Log.h"
//#include "CkUnreal/EcsBootstrapper/CkEcsBootstrapper.h"
//#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"
//#include "CkUnreal/Entity/CkUnrealEntity_Utils.h"
//
//#include "Net/UnrealNetwork.h"
//
//// --------------------------------------------------------------------------------------------------------------------
//
//ACk_UnrealEntity_ActorProxy_UE::
//ACk_UnrealEntity_ActorProxy_UE()
//{
//    bReplicates = false;
//    bAlwaysRelevant = false;
//
//#if WITH_EDITORONLY_DATA
//    _ChildActorComponent = CreateEditorOnlyDefaultSubobject<UChildActorComponent>(TEXT("Proxy Actor Comp"));
//#endif
//}
//
//auto
//    ACk_UnrealEntity_ActorProxy_UE::
//    DoInvokeOnEntityCreated(
//        const FCk_Handle& InCreatedEntity)
//    -> void
//{
//    OnEntityCreated(InCreatedEntity);
//}
//
//#if WITH_EDITOR
//auto
//    ACk_UnrealEntity_ActorProxy_UE::
//    PostEditChangeProperty(
//        FPropertyChangedEvent& InPropertyChangedEvent)
//    -> void
//{
//    Super::PostEditChangeProperty(InPropertyChangedEvent);
//
//    if (ck::Is_NOT_Valid(InPropertyChangedEvent.Property))
//    {
//        // TODO: does this need to be logged?
//        return;
//    }
//
//    if (InPropertyChangedEvent.Property->GetFName() != GET_MEMBER_NAME_CHECKED(ACk_UnrealEntity_ActorProxy_UE, _ActorToSpawn))
//    { return; }
//
//    _ChildActorComponent->DestroyChildActor();
//    _ChildActorComponent->SetChildActorClass(_ActorToSpawn);
//}
//
//auto
//    ACk_UnrealEntity_ActorProxy_UE::
//    BeginPlay() -> void
//{
//    Super::BeginPlay();
//
//    if (ck::Is_NOT_Valid(_ActorToSpawn))
//    { return; }
//
//    if (HasAuthority())
//    {
//        const auto NewHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Get_TransientHandle());
//        GetComponentByClass<UCk_ActorInfo_ActorComponent_UE>()->_EntityHandle = NewHandle;
//        UCk_Utils_OwningActor_UE::Link(this, NewHandle);
//        UCk_Utils_OwningActor_UE::Add(NewHandle, this->GetOwner(), nullptr);
//
//        UCk_EcsBootstrapper_UE::Create(FCk_Bootstrapper_Construction_Params{this, _UnrealEntity});
//
//    }
//
//#if WITH_EDITORONLY_DATA
//    _ChildActorComponent->DestroyChildActor();
//#endif
//}
//
//#endif
//
//// --------------------------------------------------------------------------------------------------------------------
