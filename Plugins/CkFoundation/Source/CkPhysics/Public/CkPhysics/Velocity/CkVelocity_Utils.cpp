#include "CkVelocity_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Velocity_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Velocity_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_Velocity_Params>(InParams);
    InHandle.Add<ck::FCk_Fragment_Velocity_Current>(InParams.Get_StartingVelocity());
    InHandle.Add<ck::FCk_Tag_Velocity_Setup>();

    TryAddReplicatedFragment<UCk_Fragment_Velocity_Rep>(InHandle);
}

auto
    UCk_Utils_Velocity_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FCk_Fragment_Velocity_Current, ck::FCk_Fragment_Velocity_Params>();
}

auto
    UCk_Utils_Velocity_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Velocity"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_Velocity_UE::
    Get_CurrentVelocity(
        FCk_Handle InHandle)
    -> FVector
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FCk_Fragment_Velocity_Current>().Get_CurrentVelocity();
}

auto
    UCk_Utils_Velocity_UE::
    Request_OverrideVelocity(
        FCk_Handle InHandle,
        const FVector& InNewVelocity)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.Get<ck::FCk_Fragment_Velocity_Current>()._CurrentVelocity = InNewVelocity;
}

// --------------------------------------------------------------------------------------------------------------------
