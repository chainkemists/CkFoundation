#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_Acceleration_Params::
        FCk_Fragment_Acceleration_Params(
            ParamsType InParams)
        : _Params(InParams)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_Acceleration_Current::
        FCk_Fragment_Acceleration_Current(
            FVector InAcceleration)
        : _CurrentAcceleration(InAcceleration)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Acceleration_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Acceleration);
}

auto
    UCk_Fragment_Acceleration_Rep::
    OnRep_Acceleration() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(Get_AssociatedEntity(), _Acceleration);
    });
}

// --------------------------------------------------------------------------------------------------------------------