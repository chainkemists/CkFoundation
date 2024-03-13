#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "Net/UnrealNetwork.h"

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
        auto AccelerationHandle = UCk_Utils_Acceleration_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, _Acceleration);
    });
}

// --------------------------------------------------------------------------------------------------------------------
