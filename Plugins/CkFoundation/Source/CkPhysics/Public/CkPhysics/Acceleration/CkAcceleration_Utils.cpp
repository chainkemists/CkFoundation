#include "CkAcceleration_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Acceleration_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Acceleration_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_Acceleration_Params>(InParams);
    InHandle.Add<ck::FCk_Fragment_Acceleration_Current>(InParams.Get_StartingAcceleration());
    InHandle.Add<ck::FCk_Tag_Acceleration_Setup>();
}

auto
    UCk_Utils_Acceleration_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FCk_Fragment_Acceleration_Current, ck::FCk_Fragment_Acceleration_Params>();
}

auto
    UCk_Utils_Acceleration_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Acceleration"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_Acceleration_UE::
    Get_CurrentAcceleration(
        FCk_Handle InHandle)
    -> FVector
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FCk_Fragment_Acceleration_Current>().Get_CurrentAcceleration();
}

// --------------------------------------------------------------------------------------------------------------------
