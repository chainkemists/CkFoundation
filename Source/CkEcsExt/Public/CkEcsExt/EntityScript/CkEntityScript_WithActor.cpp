#include "CkEntityScript_WithActor.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_WithActor_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    // _OwningActor is injected by TryInjectEntityScriptSpawnParams before Construct is called

    CK_ENSURE_IF_NOT(ck::IsValid(_OwningActor),
        TEXT("EntityScript_WithActor [{}] has no valid OwningActor. "
             "Did you pass FCk_EntityScript_WithActor_SpawnParams with the Actor?"),
        GetClass()->GetName())
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    // Order matters: OwningActor MUST be added before Transform so that Transform
    // auto-detects the actor's root component and attaches to it
    UCk_Utils_OwningActor_UE::Add(InHandle, _OwningActor);
    UCk_Utils_Transform_UE::Add(InHandle, _OwningActor->GetActorTransform(), Get_Replication());

    // Set up the reverse link: Actor -> Entity
    UCk_Utils_OwningActor_UE::SetupActorEntityLink(InHandle, _OwningActor);

    return ConstructWithActor(InHandle, _OwningActor);
}

auto
    UCk_EntityScript_WithActor_UE::
    ConstructWithActor(
        FCk_Handle& InHandle,
        AActor* InOwningActor)
    -> ECk_EntityScript_ConstructionFlow
{
    return DoConstructWithActor(InHandle, InOwningActor);
}

auto
    UCk_EntityScript_WithActor_UE::
    EndPlay()
    -> void
{
    Super::EndPlay();
}

auto
    UCk_EntityScript_WithActor_UE::
    DoGet_OwningActor() const
    -> AActor*
{
    return _OwningActor;
}

auto
    UCk_EntityScript_WithActor_UE::
    ShowReplicationInEditor() const
    -> bool
{
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
