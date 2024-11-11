#include "CkEntityExtension_Fragment.h"

#include "CkEntityExtension/CkEntityExtension_Utils.h"

#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_EntityExtension_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _NewExtensionOwner, Params);
}

auto
    UCk_Fragment_EntityExtension_Rep::
    OnRep_ExtensionOwnerChanged()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    auto EntityExtension =  UCk_Utils_EntityExtension_UE::CastChecked(Get_AssociatedEntity());
    auto ExtensionOwner = UCk_Utils_EntityExtension_UE::Get_ExtensionOwner(EntityExtension);

    if (ck::Is_NOT_Valid(_NewExtensionOwner))
    {
        UCk_Utils_EntityExtension_UE::Remove(ExtensionOwner, EntityExtension);
        return;
    }

    UCk_Utils_EntityExtension_UE::Add(ExtensionOwner, EntityExtension, ECk_Replication::DoesNotReplicate);
}
