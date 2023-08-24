//#include "CkEcsBootstrapper.h"
//
//#include "CkActor/ActorInfo/CkActorInfo_Utils.h"
//
//#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
//
//#include "CkObject/CkObject_Utils.h"
//
//#include "CkUnreal/CkUnreal_Log.h"
//#include "CkUnreal/Entity/CkUnrealEntity_Utils.h"
//
//#include "Net/UnrealNetwork.h"
//
//// --------------------------------------------------------------------------------------------------------------------
//
//FCk_Bootstrapper_Construction_Params::
//FCk_Bootstrapper_Construction_Params(
//    TObjectPtr<AActor> InOuter,
//    TObjectPtr<UCk_UnrealEntity_Base_PDA> InUnrealEntity)
//    : _Outer(InOuter)
//    , _UnrealEntity(InUnrealEntity)
//{
//}
//
//// --------------------------------------------------------------------------------------------------------------------
//
//auto UCk_EcsBootstrapper_UE::
//Create(
//    FCk_Bootstrapper_Construction_Params InParams)
//-> UCk_EcsBootstrapper_UE*
//{
//    auto* ObjectReplicator = InParams.Get_Outer()->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>();
//
//    CK_ENSURE_IF_NOT(ck::IsValid(ObjectReplicator),
//        TEXT("DOESNT EXIST"))
//    { return {}; }
//
//    if (NOT InParams.Get_Outer()->HasAuthority())
//    { return nullptr; }
//
//    auto* NewBootstrapper = NewObject<UCk_EcsBootstrapper_UE>(InParams.Get_Outer());
//    ObjectReplicator->Request_RegisterObjectForReplication(NewBootstrapper);
//
//    NewBootstrapper->_ConstructionParams = InParams;
//    NewBootstrapper->DoInvoke_ConstructionScript(InParams);
//
//    return NewBootstrapper;
//}
//
//auto UCk_EcsBootstrapper_UE::DoInvoke_ConstructionScript(
//    FCk_Bootstrapper_Construction_Params InParams) -> void
//{
//    if (_IsConstructed)
//    { return; }
//
//    _IsConstructed = true;
//
//    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_UnrealEntity()->Get_EntityConstructionScript()),
//        TEXT("UnrealEntity [{}] does NOT have a ConstructionScript assigned.[{}]"), InParams.Get_UnrealEntity(), ck::Context(this))
//    { return; }
//
//    // TODO: check if ConstructionScript is also cloned OR does it use the existing pointer
//    // TODO: if it's the latter, then we need to clone the ConstructionScript as well
//    // TODO: This was TESTED and it seems we get a different pointer but the same name - investigate further
//    const auto& InstancedUnrealEntity = UCk_Utils_Object_UE::Request_CloneObject(this, InParams.Get_UnrealEntity().Get());
//
//    const auto UnrealEntity = InstancedUnrealEntity->Get_EntityConstructionScript();
//
//    CK_ENSURE_IF_NOT(ck::IsValid(UnrealEntity), TEXT("UnrealEntity REQUIRES the Entity Construction Script "
//        "to be of type UnrealEntity_ConstructionScript_WithActor_PDA. Instead, it is of type [{}].[{}]"),
//        InstancedUnrealEntity, ck::Context(this))
//    { return; }
//
//    const auto& PreBuildFunc = [this, InParams](const FCk_Handle& InCreatedEntity)
//    {
//        ck::unreal::Log(TEXT("PreBuildFunction"));
//        UCk_Utils_OwningActor_UE::Add(InCreatedEntity, InParams.Get_Outer(), this);
//        _AssociatedEntity = InCreatedEntity;
//
//        if (ck::IsValid(this->Get_ReplicatedActor()))
//        { UCk_Utils_OwningActor_UE::Link(this->Get_ReplicatedActor(), _AssociatedEntity); }
//    };
//
//    const auto ActorInfo = UCk_Utils_OwningActor_UE::Get_ActorInfoBasicDetails_FromActor(InParams.Get_Outer());
//    UCk_Utils_UnrealEntity_UE::Request_Spawn(ActorInfo.Get_Handle(),
//        FCk_Request_UnrealEntity_Spawn{InstancedUnrealEntity, PreBuildFunc, [](auto){}});
//}
//
//void UCk_EcsBootstrapper_UE::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//    DOREPLIFETIME(ThisType, _ConstructionParams);
//}
//
//auto UCk_EcsBootstrapper_UE::
//OnRep_ConstructionParams(const FCk_Bootstrapper_Construction_Params& InConstructionScript) -> void
//{
//    CK_ENSURE_IF_NOT(ck::IsValid(_ConstructionParams.Get_Outer()), TEXT("INVALID"))
//    { return; }
//
//    if (ck::Is_NOT_Valid(Get_ReplicatedActor()))
//    { return; }
//
//    DoInvoke_ConstructionScript(_ConstructionParams);
//}
//
//void UCk_EcsBootstrapper_UE::OnRep_ReplicatedActor(AActor* InActor)
//{
//    if (ck::Is_NOT_Valid(_ConstructionParams.Get_Outer()))
//    { return; }
//
//    DoInvoke_ConstructionScript(_ConstructionParams);
//}
//
//// --------------------------------------------------------------------------------------------------------------------
