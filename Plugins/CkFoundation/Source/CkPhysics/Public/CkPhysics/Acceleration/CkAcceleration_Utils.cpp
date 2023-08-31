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

    TryAddReplicatedFragment<UCk_Fragment_Acceleration_Rep>(InHandle);
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

auto
    UCk_Utils_Acceleration_UE::
    Request_OverrideAcceleration(
        FCk_Handle InHandle,
        const FVector& InNewAcceleration)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.Get<ck::FCk_Fragment_Acceleration_Current>()._CurrentAcceleration = InNewAcceleration;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Acceleration_SingleTarget_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData& InParams) -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Target()),
        TEXT("Target Entity [{}] is NOT a valid Entity when adding Single Target Acceleration Modifier to Handle [{}]"),
        InParams.Get_Target(),
        InHandle)
    { return; }

    InHandle.Add<ck::FCk_Tag_AccelerationModifier_SingleTarget>();
    InHandle.Add<ck::FCk_Tag_AccelerationModifier_SingleTarget_Setup>();

    UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Add(InHandle, InParams.Get_Target());
    UCk_Utils_Acceleration_UE::Add(InHandle, InParams.Get_AccelerationParams());
}

auto
    UCk_Utils_Acceleration_SingleTarget_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle->Has<ck::FCk_Tag_AccelerationModifier_SingleTarget>(InHandle.Get_Entity()) &&
           UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Has(InHandle);
}

auto
    UCk_Utils_Acceleration_SingleTarget_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Single Target Acceleration Modifier"), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
