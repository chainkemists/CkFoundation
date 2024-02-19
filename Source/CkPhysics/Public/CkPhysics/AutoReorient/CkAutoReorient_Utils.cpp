#include "CkAutoReorient_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AutoReorient_UE::
    Add(
        FCk_Handle_UnderConstruction& InHandle,
        const FCk_Fragment_AutoReorient_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_AutoReorient_Params>(InParams);
}

auto
    UCk_Utils_AutoReorient_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_AutoReorient_Params>();
}

auto
    UCk_Utils_AutoReorient_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have AutoReorient"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_AutoReorient_UE::
    Get_AutoReorientPolicy(
        FCk_Handle InHandle)
    -> ECk_AutoReorient_Policy
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_AutoReorient_Params>().Get_Params().Get_ReorientPolicy();
}

auto
    UCk_Utils_AutoReorient_UE::
    Request_Start(
        FCk_Handle InHandle)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }


    if (const auto& ReorientPolicy = InHandle.Get<ck::FFragment_AutoReorient_Params>().Get_Params().Get_ReorientPolicy() ==
        ECk_AutoReorient_Policy::OrientTowardsVelocity)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::Has(InHandle),
            TEXT("AutoReorient for Entity [{}] is set to [{}] but does NOT have a Velocity fragment!"),
            InHandle,
            ReorientPolicy)
        { return; }

        InHandle.Add<ck::FTag_AutoReorient_OrientTowardsVelocity>();
    }
}

auto
    UCk_Utils_AutoReorient_UE::
    Request_Stop(
        FCk_Handle InHandle)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    if (InHandle.Get<ck::FFragment_AutoReorient_Params>().Get_Params().Get_ReorientPolicy() == ECk_AutoReorient_Policy::OrientTowardsVelocity)
    {
        InHandle.Remove<ck::FTag_AutoReorient_OrientTowardsVelocity>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
