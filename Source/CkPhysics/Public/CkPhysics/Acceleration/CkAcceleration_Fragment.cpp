#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Acceleration_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Acceleration, Params);
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

auto
    UCk_Fragment_Acceleration_Rep::
    Set_Acceleration(
        FVector InAcceleration)
    -> void
{
    _Acceleration = InAcceleration;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Acceleration, this);
}

// --------------------------------------------------------------------------------------------------------------------
