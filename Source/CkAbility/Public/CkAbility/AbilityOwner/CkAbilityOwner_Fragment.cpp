#include "CkAbilityOwner_Fragment.h"

#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_AbilityOwner_Events_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Events);
}

auto
    UCk_Fragment_AbilityOwner_Events_Rep::
    OnRep_NewEvents()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    for (auto Index = _NextEventToProcess; Index < _Events.Num(); ++Index)
    {
        // Fire the event here
    }
}
