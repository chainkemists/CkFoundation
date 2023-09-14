#include "CkVelocity_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Velocity_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Velocity_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_Velocity_Params>(InParams);
    InHandle.Add<ck::FFragment_Velocity_Current>(InParams.Get_StartingVelocity());
    InHandle.Add<ck::FTag_Velocity_Setup>();

    TryAddReplicatedFragment<UCk_Fragment_Velocity_Rep>(InHandle);
}

auto
    UCk_Utils_Velocity_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Velocity_Current, ck::FFragment_Velocity_Params>();
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

    return InHandle.Get<ck::FFragment_Velocity_Current>().Get_CurrentVelocity();
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

    InHandle.Get<ck::FFragment_Velocity_Current>()._CurrentVelocity = InNewVelocity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VelocityModifier_SingleTarget_UE::
    Add(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_VelocityModifier_SingleTarget_ParamsData& InParams)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::Has(InVelocityOwnerEntity),
        TEXT("Cannot add Single Target Velocity Modifier to Entity [{}] because it does NOT have a Velocity Fragment"),
        InVelocityOwnerEntity)
    { return; }

    auto newModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InVelocityOwnerEntity);

    newModifierEntity.Add<ck::FTag_VelocityModifier_SingleTarget>();
    newModifierEntity.Add<ck::FTag_VelocityModifier_SingleTarget_Setup>();

    UCk_Utils_GameplayLabel_UE::Add(newModifierEntity, InModifierName);

    RecordOfVelocityModifiers_Utils::AddIfMissing(InVelocityOwnerEntity);
    RecordOfVelocityModifiers_Utils::Request_Connect(InVelocityOwnerEntity, newModifierEntity);

    UCk_Utils_Velocity_UE::VelocityTarget_Utils::Add(newModifierEntity, InVelocityOwnerEntity);
    UCk_Utils_Velocity_UE::Add(newModifierEntity, InParams.Get_VelocityParams());
}

auto
    UCk_Utils_VelocityModifier_SingleTarget_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_VelocityModifier_SingleTarget_UE,
        RecordOfVelocityModifiers_Utils>(InHandle, InModifierName);

    return ck::IsValid(VelocityModifierEntity);
}

auto
    UCk_Utils_VelocityModifier_SingleTarget_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle, InModifierName), TEXT("Handle [{}] does NOT have Single Target Velocity Modifier"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_VelocityModifier_SingleTarget_UE::
    Remove(
        FCk_Handle   InHandle,
        FGameplayTag InModifierName)
    -> void
{
    const auto& VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_VelocityModifier_SingleTarget_UE,
        RecordOfVelocityModifiers_Utils>(InHandle, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(VelocityModifierEntity);
}

auto
    UCk_Utils_VelocityModifier_SingleTarget_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle->Has<ck::FTag_VelocityModifier_SingleTarget>(InHandle.Get_Entity()) &&
        UCk_Utils_Velocity_UE::VelocityTarget_Utils::Has(InHandle) &&
        UCk_Utils_Velocity_UE::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
