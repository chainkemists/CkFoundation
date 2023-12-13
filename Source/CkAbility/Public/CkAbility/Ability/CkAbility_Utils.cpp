#include "CkAbility_Utils.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/CkAbility_Subsystem.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams)
    -> void
{
    const auto NewAbilityEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle,
    [&](const FCk_Handle& InNewEntity)
    {
        DoAdd(InNewEntity, InParams);
    });

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
        FCk_Handle InAbilityEntity)
    -> bool
{
    return InAbilityEntity.Has_All<ck::FFragment_Ability_Params, ck::FFragment_Ability_Current>();
}

auto
    UCk_Utils_Ability_UE::
    Ensure(
        FCk_Handle InAbilityEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAbilityEntity), TEXT("Handle [{}] does NOT have an Ability"), InAbilityEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_Ability_UE::
    Get_Info(
        FCk_Handle InAbilityEntity)
    -> FCk_Ability_Info
{
    if (NOT Ensure(InAbilityEntity))
    { return {}; }

    const auto& Label = UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntity);
    const auto& AbilityOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityEntity);

    return FCk_Ability_Info{Label, AbilityOwner};
}

auto
    UCk_Utils_Ability_UE::
    Get_ActivationSettings(
        FCk_Handle InAbilityEntity)
    -> FCk_Ability_ActivationSettings
{
    if (NOT Ensure(InAbilityEntity))
    { return {}; }

    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data().Get_ActivationSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_NetworkSettings(
        FCk_Handle InAbilityEntity)
    -> FCk_Ability_NetworkSettings
{
    if (NOT Ensure(InAbilityEntity))
    { return {}; }

    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data().Get_NetworkSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_Status(
        FCk_Handle InAbilityEntity)
    -> ECk_Ability_Status
{
    if (NOT Ensure(InAbilityEntity))
    { return {}; }

    return InAbilityEntity.Get<ck::FFragment_Ability_Current>().Get_Status();
}

auto
    UCk_Utils_Ability_UE::
    Get_ScriptClass(
        FCk_Handle InAbilityEntity)
    -> TSubclassOf<UCk_Ability_Script_PDA>
{
    if (NOT Ensure(InAbilityEntity))
    { return {}; }

    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_AbilityScriptClass();
}

auto
    UCk_Utils_Ability_UE::
    BindTo_OnAbilityActivated(
        FCk_Handle InAbilityHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Ability_OnActivated& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityHandle))
    { return; }

    ck::UUtils_Signal_OnAbilityActivated::Bind(InAbilityHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_Ability_UE::
    UnbindFrom_OnAbilityActivated(
        FCk_Handle InAbilityHandle,
        const FCk_Delegate_Ability_OnActivated& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityHandle))
    { return; }

    ck::UUtils_Signal_OnAbilityActivated::Unbind(InAbilityHandle, InDelegate);
}

auto
    UCk_Utils_Ability_UE::
    BindTo_OnAbilityDeactivated(
        FCk_Handle InAbilityHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityHandle))
    { return; }

    ck::UUtils_Signal_OnAbilityDeactivated::Bind(InAbilityHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_Ability_UE::
    UnbindFrom_OnAbilityDeactivated(
        FCk_Handle InAbilityHandle,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityHandle))
    { return; }

    ck::UUtils_Signal_OnAbilityDeactivated::Unbind(InAbilityHandle, InDelegate);
}

auto
    UCk_Utils_Ability_UE::
    DoActivate(
        FCk_Handle InAbilityEntity)
    -> void
{
    if (NOT Has(InAbilityEntity))
    { return; }

    auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current>();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityCurrent._AbilityScript),
        TEXT("Cannot Activate Ability with Entity [{}] because its AbilityScript is INVALID"),
        InAbilityEntity)
    { return; }

    AbilityCurrent._Status = ECk_Ability_Status::Active;
    AbilityCurrent._AbilityScript->OnActivateAbility();

    ck::UUtils_Signal_OnAbilityActivated::Broadcast(InAbilityEntity, ck::MakePayload(InAbilityEntity));
}

auto
    UCk_Utils_Ability_UE::
    DoDeactivate(
        FCk_Handle InAbilityEntity)
    -> void
{
    if (NOT Has(InAbilityEntity))
    { return; }

    auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current>();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityCurrent._AbilityScript),
        TEXT("Cannot Deactivate Ability with Entity [{}] because its AbilityScript is INVALID"),
        InAbilityEntity)
    { return; }

    AbilityCurrent._Status = ECk_Ability_Status::NotActive;
    AbilityCurrent._AbilityScript->OnDeactivateAbility();

    ck::UUtils_Signal_OnAbilityDeactivated::Broadcast(InAbilityEntity, ck::MakePayload(InAbilityEntity));
}

auto
    UCk_Utils_Ability_UE::
    DoGet_CanActivate(
        FCk_Handle InAbilityEntity)
    -> bool
{
    if (NOT Ensure(InAbilityEntity))
    { return {}; }

    const auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current>();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityCurrent._AbilityScript),
        TEXT("Cannot check if Ability with Entity [{}] can be Activated because its AbilityScript is INVALID"),
        InAbilityEntity)
    { return {}; }

    return AbilityCurrent._AbilityScript->Get_CanActivateAbility();
}

auto
    UCk_Utils_Ability_UE::
    DoAdd(
        FCk_Handle InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams)
    -> void
{
    const auto& AbilityScriptClass = InParams.Get_AbilityScriptClass();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass), TEXT("Invalid Ability Script Class"))
    { return; }

    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(AbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO), TEXT("Failed to create CDO of Ability Script of Class [{}]"), AbilityScriptClass)
    { return; }

    const auto AbilityData = AbilityScriptCDO->Get_Data();
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

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(AbilityScriptInstance)
    { return; }

    UCk_Utils_GameplayLabel_UE::Add(InHandle, AbilityName);

    InHandle.Add<ck::FFragment_Ability_Params>(InParams);
    InHandle.Add<ck::FFragment_Ability_Current>(AbilityScriptInstance);

    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityScriptInstance->GetWorld())->Request_TrackAbilityScript(AbilityScriptInstance);

    // TODO: Add Rep Fragment
}

auto
    UCk_Utils_Ability_UE::
    DoGive(
        FCk_Handle InAbilityOwner,
        FCk_Handle InAbility)
    -> void
{
    if (NOT Ensure(InAbility))
    { return; }

    RecordOfAbilities_Utils::Request_Connect(InAbilityOwner, InAbility);
    const auto Script = InAbility.Get<ck::FFragment_Ability_Current>().Get_AbilityScript();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to GIVE the Ability properly"),
        InAbility,
        InAbilityOwner)
    { return; }

    Script->_AbilityOwnerHandle = InAbilityOwner;
    Script->_AbilityHandle = InAbility;

    Script->OnGiveAbility();
}

auto
    UCk_Utils_Ability_UE::
    DoRevoke(
        FCk_Handle InAbilityOwner,
        FCk_Handle InAbility)
    -> void
{
    if (NOT Ensure(InAbility))
    { return; }

    const auto& Current = InAbility.Get<ck::FFragment_Ability_Current>();
    if (Current.Get_Status() == ECk_Ability_Status::Active)
    {
        UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InAbilityOwner, FCk_Request_AbilityOwner_DeactivateAbility{InAbility});
        UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(InAbilityOwner, FCk_Request_AbilityOwner_RevokeAbility{InAbility});

        return;
    }

    RecordOfAbilities_Utils::Request_Disconnect(InAbilityOwner, InAbility);
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAbility);

    const auto Script = Current.Get_AbilityScript();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to REVOKE the Ability properly"),
        InAbility,
        InAbilityOwner)
    { return; }

    Script->OnRevokeAbility();

    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(Script->GetWorld())->Request_RemoveAbilityScript(Script.Get());
}

// --------------------------------------------------------------------------------------------------------------------
