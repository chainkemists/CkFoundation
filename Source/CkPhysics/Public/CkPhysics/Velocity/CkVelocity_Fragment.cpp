#include "CkVelocity_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"
#include "Net/UnrealNetwork.h"

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
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_Velocity_UE::Request_OverrideVelocity(Get_AssociatedEntity(), _Velocity);
    });
}

// --------------------------------------------------------------------------------------------------------------------
