#include "CkIntent_Fragment_Params.h"

#include "CkIntent_Fragment.h"

#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Intent_NewIntent::FCk_Request_Intent_NewIntent(FGameplayTag InIntent)
    : _Intent(InIntent)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Intent_ReplicatedObject_UE::Create(AActor* InOwningActor, FCk_Handle InAssociatedEntity) -> UCk_Intent_ReplicatedObject_UE*
{
    auto* Obj = NewObject<UCk_Intent_ReplicatedObject_UE>(InOwningActor);
    Obj->_AssociatedEntity = InAssociatedEntity;
    Obj->_AssociatedActor = InOwningActor;

    // TODO: this should be hidden in the base class
    auto* ObjectReplicator = InOwningActor->GetComponentByClass<UCk_ObjectReplicator_Component>();
    ObjectReplicator->Request_RegisterObjectForReplication(Obj);

    return Obj;
}

void UCk_Intent_ReplicatedObject_UE::OnRep_AssociatedActor(AActor* InActor)
{
    Super::OnRep_AssociatedActor(InActor);

    const auto& BasicDetails = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails_FromActor(GetOwningActor());
    _AssociatedEntity = BasicDetails.Get_Handle();
    _AssociatedEntity.Add<ck::FCk_Fragment_Intent_Params>(this);
}

void UCk_Intent_ReplicatedObject_UE::Request_AddIntent_Implementation(FGameplayTag InIntent)
{
}
