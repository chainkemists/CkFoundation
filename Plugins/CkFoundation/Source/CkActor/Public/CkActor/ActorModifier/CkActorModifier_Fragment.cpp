#include "CkActorModifier_Fragment.h"

#include "CkActor/ActorModifier/CkActorModifier_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_ActorModifier_Rep::
    Request_SetLocation_Implementation(
        const FCk_Request_ActorModifier_SetLocation& InRequest)
    -> void
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_ActorModifier_UE::Request_SetLocation(Get_AssociatedEntity(), InRequest, {});
}

auto
    UCk_Fragment_ActorModifier_Rep::
    Request_AddLocationOffset_Implementation(
        const FCk_Request_ActorModifier_AddLocationOffset& InRequest)
    -> void
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_ActorModifier_UE::Request_AddLocationOffset(Get_AssociatedEntity(), InRequest, {});
}

auto
    UCk_Fragment_ActorModifier_Rep::
    Request_SetRotation_Implementation(
        const FCk_Request_ActorModifier_SetRotation& InRequest)
    -> void
{
    UCk_Utils_ActorModifier_UE::Request_SetRotation(Get_AssociatedEntity(), InRequest, {});
}

auto
    UCk_Fragment_ActorModifier_Rep::
    Request_AddRotationOffset_Implementation(
        const FCk_Request_ActorModifier_AddRotationOffset & InRequest)
    -> void
{
    UCk_Utils_ActorModifier_UE::Request_AddRotationOffset(Get_AssociatedEntity(), InRequest, {});
}

auto
    UCk_Fragment_ActorModifier_Rep::
    Request_SetScale_Implementation(
        const FCk_Request_ActorModifier_SetScale& InRequest)
    -> void
{
    UCk_Utils_ActorModifier_UE::Request_SetScale(Get_AssociatedEntity(), InRequest, {});
}

auto
    UCk_Fragment_ActorModifier_Rep::
    Request_SetTransform_Implementation(
        const FCk_Request_ActorModifier_SetTransform& InRequest)
    -> void
{
    UCk_Utils_ActorModifier_UE::Request_SetTransform(Get_AssociatedEntity(), InRequest, {});
}

// --------------------------------------------------------------------------------------------------------------------
