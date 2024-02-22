#include "CkAbility_Utils.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Subsystem/CkAbility_Subsystem.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Ability, UCk_Utils_Ability_UE, FCk_Handle_Ability, ck::FFragment_Ability_Params, ck::FFragment_Ability_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_UE::
    Get_DisplayName(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FName
{
    const auto& AbilityParams = InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data();

    if (AbilityParams.Get_HasDisplayName())
    { return AbilityParams.Get_DisplayName(); }

    return AbilityParams.Get_AbilityName().GetTagName();
}

auto
    UCk_Utils_Ability_UE::
    Get_ActivationSettings(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Ability_ActivationSettings
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data().Get_ActivationSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_NetworkSettings(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Ability_NetworkSettings
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_Data().Get_NetworkSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_Status(
        const FCk_Handle_Ability& InAbilityEntity)
    -> ECk_Ability_Status
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Current>().Get_Status();
}

auto
    UCk_Utils_Ability_UE::
    Get_ScriptClass(
        const FCk_Handle_Ability& InAbilityEntity)
    -> TSubclassOf<UCk_Ability_Script_PDA>
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params().Get_AbilityScriptClass();
}

auto
    UCk_Utils_Ability_UE::
    Get_CanActivate(
        const FCk_Handle_Ability& InAbilityEntity)
    -> ECk_Ability_ActivationRequirementsResult
{
    const auto& AbilityParams             = InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params();
    const auto& AbilityActivationSettings = AbilityParams.Get_Data().Get_ActivationSettings();

    const auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current>();
    const auto& Script         = AbilityCurrent.Get_AbilityScript();
    const auto& AbilityStatus  = AbilityCurrent.Get_Status();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("Cannot check if the Ability [{}] can Activate because it does NOT have a valid Script"),
        InAbilityEntity)
    { return {}; }

    const auto& AbilityOwner = Script->Get_AbilityOwnerHandle();

    const auto& AbilityOwnerActiveTags = UCk_Utils_AbilityOwner_UE::Get_ActiveTags(AbilityOwner);

    const auto& AreActivationRequirementsMet_OnOwner = [&]() -> bool
    {
        const auto ActivationSettingsOnOwner = AbilityActivationSettings.Get_ActivationSettingsOnOwner();
        const auto ActivationRequirements    = ActivationSettingsOnOwner.Get_RequiredTagsOnAbilityOwner();
        const auto ActivationBlockers        = ActivationSettingsOnOwner.Get_BlockedByTagsOnAbilityOwner();


        return AbilityOwnerActiveTags.HasAllExact(ActivationRequirements) && NOT AbilityOwnerActiveTags.HasAnyExact(ActivationBlockers);
    }();

    const auto& AreActivationRequirementsMet_OnSelf = [&]()
    {
        if (NOT UCk_Utils_AbilityOwner_UE::Has(InAbilityEntity))
        { return true; }

        const auto ActivationSettingsOnSelf = AbilityActivationSettings.Get_ActivationSettingsOnSelf();
        const auto ActivationBlockers = ActivationSettingsOnSelf.Get_BlockedByTagsOnSelf();

        const auto AbilityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::CastChecked(InAbilityEntity);
        return NOT UCk_Utils_AbilityOwner_UE::Get_ActiveTags(AbilityAsAbilityOwner).HasAnyExact(ActivationBlockers);
    }();

    // We could be clever, but this is more readable
    if (AreActivationRequirementsMet_OnOwner && AreActivationRequirementsMet_OnSelf)
    {
        if (AbilityStatus == ECk_Ability_Status::NotActive)
        { return ECk_Ability_ActivationRequirementsResult::RequirementsMet; }

        return ECk_Ability_ActivationRequirementsResult::RequirementsMet_ButAlreadyActive;
    }

    if (NOT AreActivationRequirementsMet_OnOwner && NOT AreActivationRequirementsMet_OnSelf)
    { return ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnOwnerAndSelf; }

    if (AreActivationRequirementsMet_OnOwner)
    { return ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnOwner; }

    return ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnSelf;
}

auto
    UCk_Utils_Ability_UE::
    BindTo_OnAbilityActivated(
        FCk_Handle_Ability& InAbilityHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Ability_OnActivated& InDelegate)
    -> FCk_Handle_Ability
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAbilityActivated, InAbilityHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAbilityHandle;
}

auto
    UCk_Utils_Ability_UE::
    UnbindFrom_OnAbilityActivated(
        FCk_Handle_Ability& InAbilityHandle,
        const FCk_Delegate_Ability_OnActivated& InDelegate)
    -> FCk_Handle_Ability
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAbilityActivated, InAbilityHandle, InDelegate);
    return InAbilityHandle;
}

auto
    UCk_Utils_Ability_UE::
    BindTo_OnAbilityDeactivated(
        FCk_Handle_Ability& InAbilityHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate)
    -> FCk_Handle_Ability
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAbilityDeactivated, InAbilityHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAbilityHandle;
}

auto
    UCk_Utils_Ability_UE::
    UnbindFrom_OnAbilityDeactivated(
        FCk_Handle_Ability& InAbilityHandle,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate)
    -> FCk_Handle_Ability
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAbilityDeactivated, InAbilityHandle, InDelegate);
    return InAbilityHandle;
}

auto
    UCk_Utils_Ability_UE::
    DoActivate(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FCk_Handle_Ability& InAbilityEntity,
        const FCk_Ability_Payload_OnActivate& InOptionalPayload)
    -> void
{
    auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current>();
    const auto Script = AbilityCurrent.Get_AbilityScript();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("Cannot Activate Ability with Entity [{}] because its AbilityScript is INVALID"),
        InAbilityEntity)
    { return; }

    const auto& AbilityParams = InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Params();
    const auto& AbilityInstancingPolicy = AbilityParams.Get_Data().Get_InstancingPolicy();
    AbilityCurrent._Status = ECk_Ability_Status::Active;

    CK_ENSURE_IF_NOT([&]
    {
        if (AbilityInstancingPolicy == ECk_Ability_InstancingPolicy::NotInstanced)
        { return true; }

        const auto IsSameAbilityEntity      = Script->_AbilityHandle == InAbilityEntity;
        const auto IsSameAbilityOwnerEntity = Script->_AbilityOwnerHandle == InAbilityOwnerEntity;

        return IsSameAbilityEntity && IsSameAbilityOwnerEntity;
    }(),
    TEXT("Ability Script [{}] that is NOT a CDO was GIVEN with {} [{}] and {} [{}] but ACTIVATED with Ability [{}] and Ability Owner [{}]\n"
         "This is supported/expected on a CDO, but NOT for instanced Abilities"),
         Script,
         ck::IsValid(Script->_AbilityHandle) ? TEXT("VALID Ability") : TEXT("INVALID Ability"),
         Script->_AbilityHandle,
         ck::IsValid(Script->_AbilityOwnerHandle) ? TEXT("VALID Ability Owner") : TEXT("INVALID Ability Owner"),
         Script->_AbilityOwnerHandle,
         InAbilityEntity,
         InAbilityOwnerEntity)
    {}

    // It is possible that between Give and Activate, the Ability Owning & Handle were changed
    // if the Ability is a CDO AND multiple activate/deactivate requests happened in the same frame.
    // This is partly due to the fact that we process deactivation requests immediately after activation
    // IFF activation + deactivation happened in the same frame
    Script->_AbilityHandle = InAbilityEntity;
    Script->_AbilityOwnerHandle = InAbilityOwnerEntity;

    Script->OnActivateAbility(InOptionalPayload);

    ck::UUtils_Signal_OnAbilityActivated::Broadcast(InAbilityEntity, ck::MakePayload(InAbilityEntity));
}

auto
    UCk_Utils_Ability_UE::
    DoDeactivate(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FCk_Handle_Ability& InAbilityEntity)
    -> void
{
    auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current, ck::IsValid_Policy_IncludePendingKill>();
    const auto& AbilityParams = InAbilityEntity.Get<ck::FFragment_Ability_Params, ck::IsValid_Policy_IncludePendingKill>().Get_Params();
    auto Script = AbilityCurrent.Get_AbilityScript();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("Cannot Deactivate Ability with Entity [{}] because its AbilityScript is INVALID"),
        InAbilityEntity)
    { return; }

    if (AbilityCurrent.Get_Status() == ECk_Ability_Status::NotActive)
    { return; }

    AbilityCurrent._Status = ECk_Ability_Status::NotActive;
    Script->OnDeactivateAbility();

    ck::UUtils_Signal_OnAbilityDeactivated::Broadcast(InAbilityEntity, ck::MakePayload(InAbilityEntity));

    if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InAbilityEntity))
    { return; }

    // NOTE: If we reset the ability script properties on DEACTIVATE, we are potentially doing a cleanup for nothing
    // if the ability is not activated again. If we do it on ACTIVATE then we are going to perform a cleanup for nothing
    // the very first time it is activated
    if (AbilityParams.Get_Data().Get_InstancingPolicy() == ECk_Ability_InstancingPolicy::InstancedPerAbilityActivation)
    {
        switch (const auto& RecyclingPolicy = UCk_Utils_Ability_Settings_UE::Get_AbilityRecyclingPolicy())
        {
            case ECk_Ability_RecyclingPolicy::Recycle:
            {
                ck::ability::VeryVerbose
                (
                    TEXT("Recycling Ability Script [{}] with Entity [{}] for Ability Owner [{}]"),
                    Script,
                    InAbilityEntity,
                    InAbilityOwnerEntity
                );

                UCk_Utils_Object_UE::Request_ResetAllPropertiesToDefault(Script.Get());

                break;
            }
            case ECk_Ability_RecyclingPolicy::DoNotRecycle:
            {
                ck::ability::VeryVerbose
                (
                    TEXT("Instancing new Ability Script [{}] with Entity [{}] for Ability Owner [{}]"),
                    Script,
                    InAbilityEntity,
                    InAbilityOwnerEntity
                );

                const auto& AbilityScriptClass = AbilityParams.Get_AbilityScriptClass();

                UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityCurrent.Get_AbilityScript()->GetWorld())->
                    Request_RemoveAbilityScript(AbilityCurrent.Get_AbilityScript().Get());

                Script = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_Script_PDA>
                (
                    UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAbilityOwnerEntity),
                    AbilityScriptClass,
                    nullptr
                );

                UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityCurrent.Get_AbilityScript()->GetWorld())->
                    Request_TrackAbilityScript(AbilityCurrent.Get_AbilityScript().Get());

                break;
            }
            default:
            {
                CK_INVALID_ENUM(RecyclingPolicy);
                return;
            }
        }

        if (ck::IsValid(Script))
        {
            Script->_AbilityHandle = InAbilityEntity;
            Script->_AbilityOwnerHandle = InAbilityOwnerEntity;
        }
    }
}

auto
    UCk_Utils_Ability_UE::
    DoAdd(
        FCk_Handle& InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams)
    -> void
{
    const auto& AbilityScriptClass = InParams.Get_AbilityScriptClass();
    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass), TEXT("Invalid Ability Script Class"))
    { return; }

    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(AbilityScriptClass);

    const auto CurrentWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

    AbilityScriptCDO->Set_CurrentWorld(CurrentWorld);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO), TEXT("Failed to create CDO of Ability Script of Class [{}]"), AbilityScriptClass)
    { return; }

    const auto& AbilityData      = AbilityScriptCDO->Get_Data();
    const auto& AbilityName      = AbilityData.Get_AbilityName();
    const auto& NetworkSettings  = AbilityData.Get_NetworkSettings();
    const auto& ReplicationType  = NetworkSettings.Get_ReplicationType();
    const auto& InstancingPolicy = AbilityData.Get_InstancingPolicy();

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

    const auto& AbilityScriptToUse = InstancingPolicy == ECk_Ability_InstancingPolicy::NotInstanced
                                       ? AbilityScriptCDO
                                       : UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_Script_PDA>(CurrentWorld, AbilityScriptClass, nullptr);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptToUse), TEXT("Failed to create instance of Ability Script of Class [{}]"), AbilityScriptClass)
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(AbilityScriptToUse)
    { return; }

    UCk_Utils_GameplayLabel_UE::Add(InHandle, AbilityName);

    InHandle.Add<ck::FFragment_Ability_Params>(InParams);
    InHandle.Add<ck::FFragment_Ability_Current>(AbilityScriptToUse);

    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityScriptToUse->GetWorld())->Request_TrackAbilityScript(AbilityScriptToUse);
}

auto
    UCk_Utils_Ability_UE::
    DoGive(
        FCk_Handle_AbilityOwner& InAbilityOwner,
        FCk_Handle_Ability& InAbility,
        const FCk_Ability_Payload_OnGranted& InOptionalPayload)
    -> void
{
    RecordOfAbilities_Utils::Request_Connect(InAbilityOwner, InAbility);
    const auto Script = InAbility.Get<ck::FFragment_Ability_Current>().Get_AbilityScript();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to GIVE the Ability properly"),
        InAbility,
        InAbilityOwner)
    { return; }

    Script->_AbilityOwnerHandle = InAbilityOwner;
    Script->_AbilityHandle = InAbility;

    Script->OnGiveAbility(InOptionalPayload);
}

auto
    UCk_Utils_Ability_UE::
    DoRevoke(
        FCk_Handle_AbilityOwner& InAbilityOwner,
        FCk_Handle_Ability& InAbility)
    -> void
{
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

auto
    UCk_Utils_Ability_UE::
    DoGet_CanBeGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass)
    -> bool
{
    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(InAbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO),
        TEXT("Cannot get valid CDO for Ability Script [{}]"),
        InAbilityScriptClass)
    { return {}; }

    return AbilityScriptCDO->Get_CanBeGiven(InAbilityOwnerEntity);
}

auto
    UCk_Utils_Ability_UE::
    DoCreate_AbilityEntityConfig(
        UObject* InOuter,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass)
    -> UCk_Ability_EntityConfig_PDA*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOuter),
        TEXT("INVALID Outer! Cannot create Ability Entity Config"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityScriptClass),
        TEXT("INVALID Ability Script Class! Cannot create Ability Entity Config"))
    { return {}; }

    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(InAbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO),
        TEXT("Cannot get valid CDO for Ability Script [{}]. Cannot create Ability Entity Config"),
        InAbilityScriptClass)
    { return {}; }

    const auto& AbilityScriptData = AbilityScriptCDO->Get_Data();
    const auto& ConditionSettings = AbilityScriptData.Get_ConditionSettings();
    const auto& CostSettings      = AbilityScriptData.Get_CostSettings();
    const auto& CooldownSettings  = AbilityScriptData.Get_CooldownSettings();
    const auto& AbilityCtorScript = AbilityScriptData.Get_AbilityConstructionScript();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityCtorScript),
        TEXT("Ability Script [{}] specifies an INVALID Ability ConstructionScript. Cannot create Ability Entity Config"),
        InAbilityScriptClass)
    { return {}; }

    auto NewAbilityCtorScript = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_ConstructionScript_PDA>(InOuter, AbilityCtorScript,
    [&](UCk_Ability_ConstructionScript_PDA* InAbilityCtorScript) -> void
    {
        InAbilityCtorScript->_DefaultAbilities.Append(ConditionSettings.Get_ConditionAbilities());
        InAbilityCtorScript->_DefaultAbilities.Append(CostSettings.Get_CostAbilities());
        InAbilityCtorScript->_DefaultAbilities.Append(CooldownSettings.Get_CooldownAbilities());

        InAbilityCtorScript->_AbilityParams = FCk_Fragment_Ability_ParamsData{InAbilityScriptClass};
    });

    return UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_EntityConfig_PDA>(InOuter, nullptr,
    [&](UCk_Ability_EntityConfig_PDA* InAbilityCtorScript) -> void
    {
        InAbilityCtorScript->_EntityConstructionScript = NewAbilityCtorScript;
    });
}

auto
    UCk_Utils_Ability_UE::
    DoCreate_MultipleAbilityEntityConfigs(
        UObject* InOuter,
        const TArray<TSubclassOf<UCk_Ability_Script_PDA>> InAbilityScriptClasses)
    -> TArray<UCk_Ability_EntityConfig_PDA*>
{
    return ck::algo::Transform<TArray<UCk_Ability_EntityConfig_PDA*>>(InAbilityScriptClasses,
    [&](TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass)
    {
        return DoCreate_AbilityEntityConfig(InOuter, InAbilityScriptClass);
    });
}

// --------------------------------------------------------------------------------------------------------------------
