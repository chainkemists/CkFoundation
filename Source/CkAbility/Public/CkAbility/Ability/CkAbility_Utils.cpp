#include "CkAbility_Utils.h"

#include "CkAbility/CkAbility_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_UE::
    Add(
        FCk_Handle                             InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams)
    -> void
{
    const auto& AbilityScriptClass = InParams.Get_AbilityScriptClass();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass), TEXT("Invalid Ability Script Class"))
    { return; }

    const auto& AbilityScriptCDO = Cast<UCk_Ability_Script_PDA>(UCk_Utils_Object_UE::Get_ClassDefaultObject(AbilityScriptClass));

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO), TEXT("Failed to create CDO of Ability Script of Class [{}]"), AbilityScriptClass)
    { return; }

    const auto& AbilityData = AbilityScriptCDO->Get_Data();
    const auto& AbilityName = AbilityData.Get_AbilityName();
    const auto& NetworkSettings = AbilityData.Get_NetworkSettings();
    const auto& ReplicationType = NetworkSettings.Get_ReplicationType();

    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InHandle, ReplicationType))
    {
        ck::ability::VeryVerbose
        (
            TEXT("Skipping creation of Ability [{}] because it's Replication Type [{}] does NOT match"),
            AbilityName,
            ReplicationType
        );

        return;
    }

    const auto& AbilityScriptInstance = UCk_Utils_Object_UE::Request_CreateNewObject_TransientPackage(AbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptInstance), TEXT("Failed to create instance of Ability Script of Class [{}]"), AbilityScriptClass)
    { return; }

    // TODO: Replace with PostEntityCreated func once available
    const auto NewAbilityEntity = [&]()
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
        auto ParamsToUse = InParams;
        ParamsToUse.Set_Data(AbilityData);

        ck::UCk_Utils_OwningEntity::Add(NewEntity, InHandle);
        UCk_Utils_GameplayLabel_UE::Add(NewEntity, AbilityName);

        NewEntity.Add<ck::FFragment_Ability_Params>(ParamsToUse);
        NewEntity.Add<ck::FFragment_Ability_Current>(AbilityScriptInstance);

        // TODO: Add Rep Fragment
        return NewEntity;
    }();

    RecordOfAbilities_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfAbilities_Utils::Request_Connect(InHandle, NewAbilityEntity);
}

auto
    UCk_Utils_Ability_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleAbility_ParamsData& InParams)
    -> void
{
    for (const auto& Params : InParams.Get_AbilityParams())
    {
        Add(InHandle, Params);
    }
}

auto
    UCk_Utils_Ability_UE::
    Has(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> bool
{
    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return ck::IsValid(AbilityEntity);
}

auto
    UCk_Utils_Ability_UE::
    Has_Any(
        FCk_Handle InAbilityOwnerEntity)
    -> bool
{
    return RecordOfAbilities_Utils::Has(InAbilityOwnerEntity);
}

auto
    UCk_Utils_Ability_UE::
    Ensure(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAbilityOwnerEntity, InAbilityName), TEXT("Handle [{}] does NOT have Ability [{}]"), InAbilityOwnerEntity, InAbilityName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Ability_UE::
    Ensure_Any(
        FCk_Handle InAbilityOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAbilityOwnerEntity), TEXT("Handle [{}] does NOT have any Ability [{}]"), InAbilityOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_Ability_UE::
    Get_All(
        FCk_Handle InAbilityOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT Has_Any(InAbilityOwnerEntity))
    { return {}; }

    const auto& AbilityEntities = RecordOfAbilities_Utils::Get_AllRecordEntries(InAbilityOwnerEntity);

    return ck::algo::Transform<TArray<FGameplayTag>>(AbilityEntities, [&](FCk_Handle InAbilityEntity)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntity);
    });
}

auto
    UCk_Utils_Ability_UE::
    Get_ActivationSettings(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> FCk_Ability_ActivationSettings
{
    if (NOT Ensure(InAbilityOwnerEntity, InAbilityName))
    { return {}; }

    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return AbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data().Get_ActivationSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_NetworkSettings(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> FCk_Ability_NetworkSettings
{
    if (NOT Ensure(InAbilityOwnerEntity, InAbilityName))
    { return {}; }

    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return AbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data().Get_NetworkSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_Status(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> ECk_Ability_Status
{
    if (NOT Ensure(InAbilityOwnerEntity, InAbilityName))
    { return {}; }

    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return AbilityEntity.Get<ck::FFragment_Ability_Current>().Get_Status();
}

auto
    UCk_Utils_Ability_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Ability_Params, ck::FFragment_Ability_Current>();
}

auto
    UCk_Utils_Ability_UE::
    Activate(
        FCk_Handle InHandle)
    -> void
{
    if (NOT Has(InHandle))
    { return; }

    auto& AbilityCurrent = InHandle.Get<ck::FFragment_Ability_Current>();

    AbilityCurrent._Status = ECk_Ability_Status::Active;
    AbilityCurrent._AbilityScript->OnActivateAbility();

    // TODO: Fire Signal
}

auto
    UCk_Utils_Ability_UE::
    End(
        FCk_Handle InHandle)
    -> void
{
    if (NOT Has(InHandle))
    { return; }

    auto& AbilityCurrent = InHandle.Get<ck::FFragment_Ability_Current>();

    AbilityCurrent._Status = ECk_Ability_Status::NotActive;
    AbilityCurrent._AbilityScript->OnEndAbility();

    // TODO: Fire Signal
}

// --------------------------------------------------------------------------------------------------------------------
