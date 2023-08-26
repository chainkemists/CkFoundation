#include "CkVelocity_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"
#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_Velocity_Params::
        FCk_Fragment_Velocity_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_Velocity_Current::
        FCk_Fragment_Velocity_Current(
            FVector InVelocity)
        : _CurrentVelocity(InVelocity)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Velocity_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Velocity);
}

auto
    UCk_Fragment_Velocity_Rep::
    OnRep_Velocity() -> void
{
    if (NOT Get_AssociatedEntity().IsValid())
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_Velocity_UE::Request_OverrideVelocity(Get_AssociatedEntity(), _Velocity);
}

// --------------------------------------------------------------------------------------------------------------------
