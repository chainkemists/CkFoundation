#include "CkAcceleration_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
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
    InHandle.Add<ck::FTag_Acceleration_NeedsSetup>();

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
    UCk_Utils_AccelerationChannel_UE::
    AddMultiple(
        FCk_Handle            InAccelerationOwnerEntity,
        FGameplayTagContainer InAccelerationChannels)
    -> void
{
    TArray<FGameplayTag> outAccelerationChannelTags;
    InAccelerationChannels.GetGameplayTagArray(outAccelerationChannelTags);

    for (const auto& velocityChannel : outAccelerationChannelTags)
    {
        Add(InAccelerationOwnerEntity, velocityChannel);
    }
}

auto
    UCk_Utils_AccelerationChannel_UE::
    Add(
        FCk_Handle   InAccelerationOwnerEntity,
        FGameplayTag InAccelerationChannel)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_Acceleration_UE::Has(InAccelerationOwnerEntity),
        TEXT("Cannot add Acceleration Channel to Entity [{}] because it does NOT have a Acceleration Fragment"),
        InAccelerationOwnerEntity)
    { return; }

    const auto NewChannelEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAccelerationOwnerEntity, [&](FCk_Handle InChannelEntity)
    {
        InChannelEntity.Add<ck::FTag_AccelerationChannel>();
        UCk_Utils_GameplayLabel_UE::Add(InChannelEntity, InAccelerationChannel);
    });

    RecordOfAccelerationChannels_Utils::AddIfMissing(InAccelerationOwnerEntity);
    RecordOfAccelerationChannels_Utils::Request_Connect(InAccelerationOwnerEntity, NewChannelEntity);
}

auto
    UCk_Utils_AccelerationChannel_UE::
    Has(
        FCk_Handle   InAccelerationOwnerEntity,
        FGameplayTag InAccelerationChannel)
    -> bool
{
    const auto& AccelerationChannelEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AccelerationModifier_UE,
        RecordOfAccelerationChannels_Utils>(InAccelerationOwnerEntity, InAccelerationChannel);

    return ck::IsValid(AccelerationChannelEntity);
}

auto
    UCk_Utils_AccelerationChannel_UE::
    Ensure(
        FCk_Handle   InAccelerationOwnerEntity,
        FGameplayTag InAccelerationChannel)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAccelerationOwnerEntity, InAccelerationChannel), TEXT("Handle [{}] does NOT have Acceleration Channel [{}]"), InAccelerationOwnerEntity, InAccelerationChannel)
    { return false; }

    return true;
}

auto
    UCk_Utils_AccelerationChannel_UE::
    Get_IsAffectedByOtherChannel(
        FCk_Handle   InAccelerationOwnerEntity,
        FGameplayTag InOtherAccelerationChannel)
    -> bool
{
    return RecordOfAccelerationChannels_Utils::Get_HasRecordEntry(InAccelerationOwnerEntity, ck::algo::MatchesGameplayLabel{InOtherAccelerationChannel});
}

auto
    UCk_Utils_AccelerationChannel_UE::
    Get_IsAffectedByAnyOtherChannel(
        FCk_Handle            InAccelerationOwnerEntity,
        FGameplayTagContainer InOtherAccelerationChannels)
    -> bool
{
    return RecordOfAccelerationChannels_Utils::Get_HasRecordEntry(InAccelerationOwnerEntity, ck::algo::MatchesAnyGameplayLabel{InOtherAccelerationChannels});
}

auto
    UCk_Utils_AccelerationChannel_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_AccelerationChannel>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AccelerationModifier_UE::
    Add(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_AccelerationModifier_ParamsData& InParams)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(UCk_Utils_Acceleration_UE::Has(InAccelerationOwnerEntity),
        TEXT("Cannot add Single Target Acceleration Modifier to Entity [{}] because it does NOT have a Acceleration Fragment"),
        InAccelerationOwnerEntity)
    { return {}; }

    const auto NewModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAccelerationOwnerEntity, [&](FCk_Handle InModifierEntity)
    {
        InModifierEntity.Add<ck::FTag_AccelerationModifier>();
        InModifierEntity.Add<ck::FTag_AccelerationModifier_NeedsSetup>();

        UCk_Utils_GameplayLabel_UE::Add(InModifierEntity, InModifierName);
        UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Add(InModifierEntity, InAccelerationOwnerEntity);
        UCk_Utils_Acceleration_UE::Add(InModifierEntity, InParams.Get_AccelerationParams());
    });

    RecordOfAccelerationModifiers_Utils::AddIfMissing(InAccelerationOwnerEntity);
    RecordOfAccelerationModifiers_Utils::Request_Connect(InAccelerationOwnerEntity, NewModifierEntity);

    return NewModifierEntity;
}

auto
    UCk_Utils_AccelerationModifier_UE::
    Has(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AccelerationModifier_UE,
        RecordOfAccelerationModifiers_Utils>(InAccelerationOwnerEntity, InModifierName);

    return ck::IsValid(AccelerationModifierEntity);
}

auto
    UCk_Utils_AccelerationModifier_UE::
    Ensure(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAccelerationOwnerEntity, InModifierName), TEXT("Handle [{}] does NOT have Acceleration Modifier [{}]"), InAccelerationOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_AccelerationModifier_UE::
    Remove(
        FCk_Handle   InAccelerationOwnerEntity,
        FGameplayTag InModifierName)
    -> void
{
    if (NOT Ensure(InAccelerationOwnerEntity, InModifierName))
    { return; }

    const auto& AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AccelerationModifier_UE,
        RecordOfAccelerationModifiers_Utils>(InAccelerationOwnerEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AccelerationModifierEntity);
}

auto
    UCk_Utils_AccelerationModifier_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle->Has<ck::FTag_AccelerationModifier>(InHandle.Get_Entity()) &&
        UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Has(InHandle) &&
        UCk_Utils_Acceleration_UE::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Add(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Fragment_BulkAccelerationModifier_ParamsData& InParams)
    -> FCk_Handle
{
    const auto NewModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InModifierEntity)
    {
        InModifierEntity.Add<ck::FFragment_BulkAccelerationModifier_Params>(InParams);
        InModifierEntity.Add<ck::FTag_BulkAccelerationModifier_NeedsSetup>();

        if (InParams.Get_ModifierScope() == ECk_ExtentScope::Global)
        {
            InModifierEntity.Add<ck::FTag_BulkAccelerationModifier_GlobalScope>();
        }

        UCk_Utils_GameplayLabel_UE::Add(InModifierEntity, InModifierName);
    });

    RecordOfBulkAccelerationModifiers_Utils::AddIfMissing(InHandle);
    RecordOfBulkAccelerationModifiers_Utils::Request_Connect(InHandle, NewModifierEntity);

    return NewModifierEntity;
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_BulkAccelerationModifier_UE,
        RecordOfBulkAccelerationModifiers_Utils>(InHandle, InModifierName);

    return ck::IsValid(AccelerationModifierEntity);
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle, InModifierName), TEXT("Handle [{}] does NOT have Bulk Acceleration Modifer"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Remove(
        FCk_Handle   InHandle,
        FGameplayTag InModifierName)
    -> void
{
    if (NOT Ensure(InHandle, InModifierName))
    { return; }

    const auto& AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AccelerationModifier_UE,
        RecordOfBulkAccelerationModifiers_Utils>(InHandle, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AccelerationModifierEntity);
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_BulkAccelerationModifier_Params>();
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Request_AddTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkAccelerationModifier_AddTarget& InRequest)
    -> void
{
    if (NOT Ensure(InHandle, InModifierName))
    { return; }

    auto AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_BulkAccelerationModifier_UE,
        RecordOfBulkAccelerationModifiers_Utils>(InHandle, InModifierName);

    CK_ENSURE_IF_NOT(NOT AccelerationModifierEntity.Has<ck::FTag_BulkAccelerationModifier_GlobalScope>(),
        TEXT("Cannot Start Affecting Entity [{}] Acceleration because the Bulk Acceleration Modifer [{}] operates on a GLOBAL scope"), InRequest.Get_TargetEntity(), AccelerationModifierEntity)
    { return; }

    DoRequest_AddTarget(AccelerationModifierEntity, InRequest);
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    Request_RemoveTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkAccelerationModifier_RemoveTarget& InRequest)
    -> void
{
    if (NOT Ensure(InHandle, InModifierName))
    { return; }

    auto AccelerationModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_BulkAccelerationModifier_UE,
        RecordOfBulkAccelerationModifiers_Utils>(InHandle, InModifierName);

    CK_ENSURE_IF_NOT(NOT AccelerationModifierEntity.Has<ck::FTag_BulkAccelerationModifier_GlobalScope>(),
        TEXT("Cannot Stop Affecting Entity [{}] Acceleration because the Bulk Acceleration Modifer [{}] operates on a GLOBAL scope"), InRequest.Get_TargetEntity(), AccelerationModifierEntity)
    { return; }

    DoRequest_RemoveTarget(AccelerationModifierEntity, InRequest);
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    DoRequest_AddTarget(
        FCk_Handle AccelerationModifierEntity,
        const FCk_Request_BulkAccelerationModifier_AddTarget& InRequest)
    -> void
{
    AccelerationModifierEntity.AddOrGet<ck::FFragment_BulkAccelerationModifier_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_BulkAccelerationModifier_UE::
    DoRequest_RemoveTarget(
        FCk_Handle AccelerationModifierEntity,
        const FCk_Request_BulkAccelerationModifier_RemoveTarget& InRequest)
    -> void
{
    AccelerationModifierEntity.AddOrGet<ck::FFragment_BulkAccelerationModifier_Requests>()._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------
