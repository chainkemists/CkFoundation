#include "CkTransform_Fragment.h"

#include "CkTransform_Utils.h"


#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Transform_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Location);
    DOREPLIFETIME(ThisType, _Rotation);
    DOREPLIFETIME(ThisType, _Scale);
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Transform() -> void
{
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_Transform_UE::Request_SetLocation
    (
        Get_AssociatedEntity(),
        FCk_Request_Transform_SetLocation{_Location, ECk_RelativeAbsolute::Absolute}
    );
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Rotation() -> void
{
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_Transform_UE::Request_SetRotation
    (
        Get_AssociatedEntity(),
        FCk_Request_Transform_SetRotation{FRotator{_Rotation}, ECk_RelativeAbsolute::Absolute}
    );
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Scale() -> void
{
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_Transform_UE::Request_SetScale
    (
        Get_AssociatedEntity(),
        FCk_Request_Transform_SetScale{_Scale, ECk_RelativeAbsolute::Absolute}
    );
}

// --------------------------------------------------------------------------------------------------------------------
