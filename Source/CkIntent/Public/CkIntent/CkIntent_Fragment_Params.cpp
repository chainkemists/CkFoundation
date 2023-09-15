#include "CkIntent_Fragment_Params.h"

#include "CkIntent_Fragment.h"
#include "CkIntent/CkIntent_Log.h"

#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "Net/UnrealNetwork.h"

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
    Obj->_ReplicatedActor = InOwningActor;

    // TODO: this should be hidden in the base class
    auto* ObjectReplicator = InOwningActor->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>();
    ObjectReplicator->Request_RegisterObjectForReplication(Obj);

    return Obj;
}

void UCk_Intent_ReplicatedObject_UE::OnRep_IntentReady(bool InReady)
{
    const auto& BasicDetails = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(GetOwningActor());
    _AssociatedEntity = BasicDetails.Get_Handle();
    _AssociatedEntity.Add<ck::FFragment_Intent_Params>(this);
}

void UCk_Intent_ReplicatedObject_UE::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisType, _Ready, COND_None, REPNOTIFY_Always);
}

void UCk_Intent_ReplicatedObject_UE::AddIntent_Implementation(FGameplayTag InIntent)
{
    ck::intent::Warning(TEXT("Syncing intent to CLIENT: [{}]"), InIntent);
}

// --------------------------------------------------------------------------------------------------------------------
