#include "CkAcceleration_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Acceleration_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Acceleration_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_Acceleration_Params>(InParams);
    InHandle.Add<ck::FFragment_Acceleration_Current>(InParams.Get_StartingAcceleration());
    InHandle.Add<ck::FTag_Acceleration_Setup>();

    TryAddReplicatedFragment<UCk_Fragment_Acceleration_Rep>(InHandle);
}

auto
    UCk_Utils_Acceleration_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Acceleration_Current, ck::FFragment_Acceleration_Params>();
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

    return InHandle.Get<ck::FFragment_Acceleration_Current>().Get_CurrentAcceleration();
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

    InHandle.Get<ck::FFragment_Acceleration_Current>()._CurrentAcceleration = InNewAcceleration;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AccelerationModifier_SingleTarget_UE::
    Add(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData& InParams)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_Acceleration_UE::Has(InAccelerationOwnerEntity),
        TEXT("Cannot add Single Target Acceleration Modifier to Entity [{}] because it does NOT have a Acceleration Fragment"),
        InAccelerationOwnerEntity)
    { return; }

    auto newModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAccelerationOwnerEntity);

    newModifierEntity.Add<ck::FTag_AccelerationModifier_SingleTarget>();
    newModifierEntity.Add<ck::FTag_AccelerationModifier_SingleTarget_Setup>();

    UCk_Utils_GameplayLabel_UE::Add(newModifierEntity, InModifierName);

    RecordOfAccelerationModifiers_Utils::AddIfMissing(InAccelerationOwnerEntity);
    RecordOfAccelerationModifiers_Utils::Request_Connect(InAccelerationOwnerEntity, newModifierEntity);

    UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Add(newModifierEntity, InAccelerationOwnerEntity);
    UCk_Utils_Acceleration_UE::Add(newModifierEntity, InParams.Get_AccelerationParams());
}

auto
    UCk_Utils_AccelerationModifier_SingleTarget_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AccelerationModifier_SingleTarget_UE,
        RecordOfAccelerationModifiers_Utils>(InHandle, InModifierName);

    return ck::IsValid(AccelerationModifierEntity);
}

auto
    UCk_Utils_AccelerationModifier_SingleTarget_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle, InModifierName), TEXT("Handle [{}] does NOT have Single Target Acceleration Modifier"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_AccelerationModifier_SingleTarget_UE::
    Remove(
        FCk_Handle   InHandle,
        FGameplayTag InModifierName)
    -> void
{
    const auto& AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AccelerationModifier_SingleTarget_UE,
        RecordOfAccelerationModifiers_Utils>(InHandle, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AccelerationModifierEntity);
}

auto
    UCk_Utils_AccelerationModifier_SingleTarget_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle->Has<ck::FTag_AccelerationModifier_SingleTarget>(InHandle.Get_Entity()) &&
        UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Has(InHandle) &&
        UCk_Utils_Acceleration_UE::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
