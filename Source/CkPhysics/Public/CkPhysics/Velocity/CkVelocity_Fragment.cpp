#include "CkVelocity_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Velocity_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Velocity, Params);
}

auto
    UCk_Fragment_Velocity_Rep::
    OnRep_Velocity() -> void
{
    if (NOT ck::IsValid(Get_AssociatedEntity())) { return; }
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this) { return; }
    [&]()
    {
        auto VelocityHandle = UCk_Utils_Velocity_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, _Velocity);
    }();
}

auto
    UCk_Fragment_Velocity_Rep::
    Set_Velocity(
        FVector InVelocity)
    -> void
{
    _Velocity = InVelocity;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Velocity, this);
}

// --------------------------------------------------------------------------------------------------------------------
