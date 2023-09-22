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
    UCk_Utils_VelocityChannel_UE::
    AddMultiple(
        FCk_Handle            InVelocityOwnerEntity,
        FGameplayTagContainer InVelocityChannels)
    -> void
{
    TArray<FGameplayTag> outVelocityChannelTags;
    InVelocityChannels.GetGameplayTagArray(outVelocityChannelTags);

    for (const auto& velocityChannel : outVelocityChannelTags)
    {
        Add(InVelocityOwnerEntity, velocityChannel);
    }
}

auto
    UCk_Utils_VelocityChannel_UE::
    Add(
        FCk_Handle   InVelocityOwnerEntity,
        FGameplayTag InVelocityChannel)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::Has(InVelocityOwnerEntity),
        TEXT("Cannot add Velocity Channel to Entity [{}] because it does NOT have a Velocity Fragment"),
        InVelocityOwnerEntity)
    { return; }

    auto newChannelEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InVelocityOwnerEntity);

    newChannelEntity.Add<ck::FTag_VelocityChannel>();

    UCk_Utils_GameplayLabel_UE::Add(newChannelEntity, InVelocityChannel);

    RecordOfVelocityChannels_Utils::AddIfMissing(InVelocityOwnerEntity);
    RecordOfVelocityChannels_Utils::Request_Connect(InVelocityOwnerEntity, newChannelEntity);
}

auto
    UCk_Utils_VelocityChannel_UE::
    Has(
        FCk_Handle   InVelocityOwnerEntity,
        FGameplayTag InVelocityChannel)
    -> bool
{
    const auto& VelocityChannelEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_VelocityModifier_UE,
        RecordOfVelocityChannels_Utils>(InVelocityOwnerEntity, InVelocityChannel);

    return ck::IsValid(VelocityChannelEntity);
}

auto
    UCk_Utils_VelocityChannel_UE::
    Ensure(
        FCk_Handle   InVelocityOwnerEntity,
        FGameplayTag InVelocityChannel)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InVelocityOwnerEntity, InVelocityChannel), TEXT("Handle [{}] does NOT have Velocity Channel [{}]"), InVelocityOwnerEntity, InVelocityChannel)
    { return false; }

    return true;
}

auto
    UCk_Utils_VelocityChannel_UE::
    Get_IsAffectedByOtherChannel(
        FCk_Handle   InVelocityOwnerEntity,
        FGameplayTag InOtherVelocityChannel)
    -> bool
{
    return RecordOfVelocityChannels_Utils::Get_HasRecordEntry(InVelocityOwnerEntity, ck::algo::MatchesGameplayLabel{InOtherVelocityChannel});
}

auto
    UCk_Utils_VelocityChannel_UE::
    Get_IsAffectedByAnyOtherChannel(
        FCk_Handle            InVelocityOwnerEntity,
        FGameplayTagContainer InOtherVelocityChannels)
    -> bool
{
    return RecordOfVelocityChannels_Utils::Get_HasRecordEntry(InVelocityOwnerEntity, ck::algo::MatchesAnyGameplayLabel{InOtherVelocityChannels});
}

auto
    UCk_Utils_VelocityChannel_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_VelocityChannel>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VelocityModifier_UE::
    Add(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_VelocityModifier_ParamsData& InParams)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::Has(InVelocityOwnerEntity),
        TEXT("Cannot add Single Target Velocity Modifier to Entity [{}] because it does NOT have a Velocity Fragment"),
        InVelocityOwnerEntity)
    { return {}; }

    auto newModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InVelocityOwnerEntity);

    newModifierEntity.Add<ck::FTag_VelocityModifier>();
    newModifierEntity.Add<ck::FTag_VelocityModifier_Setup>();

    UCk_Utils_GameplayLabel_UE::Add(newModifierEntity, InModifierName);

    RecordOfVelocityModifiers_Utils::AddIfMissing(InVelocityOwnerEntity);
    RecordOfVelocityModifiers_Utils::Request_Connect(InVelocityOwnerEntity, newModifierEntity);

    UCk_Utils_Velocity_UE::VelocityTarget_Utils::Add(newModifierEntity, InVelocityOwnerEntity);
    UCk_Utils_Velocity_UE::Add(newModifierEntity, InParams.Get_VelocityParams());

    return newModifierEntity;
}

auto
    UCk_Utils_VelocityModifier_UE::
    Has(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_VelocityModifier_UE,
        RecordOfVelocityModifiers_Utils>(InVelocityOwnerEntity, InModifierName);

    return ck::IsValid(VelocityModifierEntity);
}

auto
    UCk_Utils_VelocityModifier_UE::
    Ensure(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InVelocityOwnerEntity, InModifierName), TEXT("Handle [{}] does NOT have Velocity Modifier [{}]"), InVelocityOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_VelocityModifier_UE::
    Remove(
        FCk_Handle   InVelocityOwnerEntity,
        FGameplayTag InModifierName)
    -> void
{
    if (NOT Ensure(InVelocityOwnerEntity, InModifierName))
    { return; }

    const auto& VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_VelocityModifier_UE,
        RecordOfVelocityModifiers_Utils>(InVelocityOwnerEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(VelocityModifierEntity);
}

auto
    UCk_Utils_VelocityModifier_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle->Has<ck::FTag_VelocityModifier>(InHandle.Get_Entity()) &&
        UCk_Utils_Velocity_UE::VelocityTarget_Utils::Has(InHandle) &&
        UCk_Utils_Velocity_UE::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Add(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Fragment_BulkVelocityModifier_ParamsData& InParams)
    -> FCk_Handle
{
    auto newModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    newModifierEntity.Add<ck::FFragment_BulkVelocityModifier_Params>(InParams);
    newModifierEntity.Add<ck::FTag_BulkVelocityModifier_Setup>();

    if (InParams.Get_ModifierScope() == ECk_ExtentScope::Global)
    {
        newModifierEntity.Add<ck::FTag_BulkVelocityModifier_GlobalScope>();
    }

    UCk_Utils_GameplayLabel_UE::Add(newModifierEntity, InModifierName);

    RecordOfBulkVelocityModifiers_Utils::AddIfMissing(InHandle);
    RecordOfBulkVelocityModifiers_Utils::Request_Connect(InHandle, newModifierEntity);

    return newModifierEntity;
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_BulkVelocityModifier_UE,
        RecordOfBulkVelocityModifiers_Utils>(InHandle, InModifierName);

    return ck::IsValid(VelocityModifierEntity);
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle, InModifierName), TEXT("Handle [{}] does NOT have Bulk Velocity Modifer"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Remove(
        FCk_Handle   InHandle,
        FGameplayTag InModifierName)
    -> void
{
    if (NOT Ensure(InHandle, InModifierName))
    { return; }

    const auto& VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_VelocityModifier_UE,
        RecordOfBulkVelocityModifiers_Utils>(InHandle, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(VelocityModifierEntity);
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_BulkVelocityModifier_Params>();
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Request_AddTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkVelocityModifier_AddTarget& InRequest)
    -> void
{
    if (NOT Ensure(InHandle, InModifierName))
    { return; }

    auto VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_BulkVelocityModifier_UE,
        RecordOfBulkVelocityModifiers_Utils>(InHandle, InModifierName);

    CK_ENSURE_IF_NOT(NOT VelocityModifierEntity.Has<ck::FTag_BulkVelocityModifier_GlobalScope>(),
        TEXT("Cannot Start Affecting Entity [{}] Velocity because the Bulk Velocity Modifer [{}] operates on a GLOBAL scope"), InRequest.Get_TargetEntity(), VelocityModifierEntity)
    { return; }

    DoRequest_AddTarget(VelocityModifierEntity, InRequest);
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    Request_RemoveTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkVelocityModifier_RemoveTarget& InRequest)
    -> void
{
    if (NOT Ensure(InHandle, InModifierName))
    { return; }

    auto VelocityModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_BulkVelocityModifier_UE,
        RecordOfBulkVelocityModifiers_Utils>(InHandle, InModifierName);

    CK_ENSURE_IF_NOT(NOT VelocityModifierEntity.Has<ck::FTag_BulkVelocityModifier_GlobalScope>(),
        TEXT("Cannot Stop Affecting Entity [{}] Velocity because the Bulk Velocity Modifer [{}] operates on a GLOBAL scope"), InRequest.Get_TargetEntity(), VelocityModifierEntity)
    { return; }

    DoRequest_RemoveTarget(VelocityModifierEntity, InRequest);
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    DoRequest_AddTarget(
        FCk_Handle VelocityModifierEntity,
        const FCk_Request_BulkVelocityModifier_AddTarget& InRequest)
    -> void
{
    VelocityModifierEntity.AddOrGet<ck::FFragment_BulkVelocityModifier_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_BulkVelocityModifier_UE::
    DoRequest_RemoveTarget(
        FCk_Handle VelocityModifierEntity,
        const FCk_Request_BulkVelocityModifier_RemoveTarget& InRequest)
    -> void
{
    VelocityModifierEntity.AddOrGet<ck::FFragment_BulkVelocityModifier_Requests>()._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------
