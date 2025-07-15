#include "CkAbility_Utils.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"
#include "CkAbility/Subsystem/CkAbility_Subsystem.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcs/EntityHolder/CkEntityHolder_Utils.h"

#include "CkEntityExtension/CkEntityExtension_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Ability_UE, FCk_Handle_Ability, ck::FFragment_Ability_Params, ck::FFragment_Ability_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_UE::
    TryGet_Owner(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Handle_AbilityOwner
{
    const auto& AbilityScript = InAbilityEntity.Get<ck::FFragment_Ability_Current>().Get_AbilityScript();

    if (ck::Is_NOT_Valid(AbilityScript))
    { return {}; }

    CK_ENSURE_IF_NOT(AbilityScript->Get_AbilityOwnerHandle() == UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityEntity),
        TEXT("The Ability [{}] does NOT have the correct Owner Handle"),
        InAbilityEntity)
    {
    }

    return UCk_Utils_AbilityOwner_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityEntity));
}

auto
    UCk_Utils_Ability_UE::
    Get_Source(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Handle
{
    return AbilitySource_Utils::Get_StoredEntity(InAbilityEntity);
}

auto
    UCk_Utils_Ability_UE::
    Get_DisplayName(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FName
{
    const auto& AbilityParams = InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Data();

    if (AbilityParams.Get_HasDisplayName())
    { return AbilityParams.Get_DisplayName(); }

    if (NOT UCk_Utils_GameplayLabel_UE::Get_IsUnnamedLabel(InAbilityEntity))
    { return UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntity).GetTagName(); }

    return UCk_Utils_Debug_UE::Get_DebugName(Get_ScriptClass(InAbilityEntity), ECk_DebugNameVerbosity_Policy::Compact);
}

auto
    UCk_Utils_Ability_UE::
    Get_OnGiveSettings(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Ability_OnGiveSettings
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Data().Get_OnGiveSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_ActivationSettings(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Ability_ActivationSettings
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Data().Get_ActivationSettings();
}

auto
    UCk_Utils_Ability_UE::
    Get_NetworkSettings(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Ability_NetworkSettings
{
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_Data().Get_NetworkSettings();
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
    return InAbilityEntity.Get<ck::FFragment_Ability_Params>().Get_AbilityScriptClass();
}

auto
    UCk_Utils_Ability_UE::
    Get_CanActivate(
        const FCk_Handle_Ability& InAbilityEntity)
    -> ECk_Ability_ActivationRequirementsResult
{
    const auto& AbilityParams             = InAbilityEntity.Get<ck::FFragment_Ability_Params>();
    const auto& AbilityActivationSettings = AbilityParams.Get_Data().Get_ActivationSettings();

    const auto& AbilityCurrent = InAbilityEntity.Get<ck::FFragment_Ability_Current>();
    const auto& Script         = AbilityCurrent.Get_AbilityScript();
    const auto& AbilityStatus  = AbilityCurrent.Get_Status();

    CK_ENSURE_IF_NOT(ck::IsValid(Script),
        TEXT("Cannot check if the Ability [{}] can Activate because it does NOT have a valid Script"),
        InAbilityEntity)
    { return {}; }

    const auto& AbilityOwner = Script->Get_AbilityOwnerHandle();

    auto IsOwnerBlockingAbilities = [](const FCk_Handle_AbilityOwner& InOwner)
    {
        auto Impl = [](const FCk_Handle_AbilityOwner& InOwner, auto& Self) mutable
        {
            if (UCk_Utils_AbilityOwner_UE::Get_IsBlockingSubAbilities(InOwner))
            { return true; }

            const auto MaybeAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InOwner));

            if (ck::Is_NOT_Valid(MaybeAbilityOwner))
            { return false; }

            return Self(MaybeAbilityOwner, Self);
        };

        return Impl(InOwner, Impl);
    };

    if (IsOwnerBlockingAbilities(AbilityOwner))
    { return ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_BlockedByOwner; }

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

    if (NOT AreActivationRequirementsMet_OnOwner)
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
    DoAdd(
        FCk_Handle& InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams)
    -> FCk_Handle_Ability
{
    const auto& AbilityScriptClass = InParams.Get_AbilityScriptClass();
    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass), TEXT("Invalid Ability Script Class"))
    { return {}; }

    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(AbilityScriptClass);
    const auto& AbilityArchetype = InParams.Get_AbilityArchetype().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO), TEXT("Failed to get CDO of Ability Script of Class [{}]"), AbilityScriptClass)
    { return {}; }

    const auto CurrentWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(CurrentWorld), TEXT("Invalid World for Entity [{}]"), InHandle)
    { return {}; }

    const auto& AbilityData      = InParams.Get_Data();
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

        return {};
    }

    const auto& AbilityScriptToUse = InstancingPolicy == ECk_Ability_InstancingPolicy::NotInstanced
                                       ? AbilityScriptCDO
                                       : UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_Script_PDA>(CurrentWorld, AbilityScriptClass, AbilityArchetype, nullptr);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptToUse),
        TEXT("Failed to create instance of Ability Script of Class [{}] with Archetype [{}]"), AbilityScriptClass, AbilityArchetype)
    { return {}; }

    AbilityScriptToUse->Set_CurrentWorld(CurrentWorld);

    InHandle.Add<ck::FFragment_Ability_Params>(InParams);
    auto& AbilityCurrent = InHandle.Add<ck::FFragment_Ability_Current>(AbilityScriptToUse);

    AbilityCurrent._AbilityScript_DefaultInstance = ck::IsValid(AbilityArchetype)
                                                        ? AbilityArchetype
                                                        : AbilityScriptCDO;

    const auto& HandleAsAbility = Cast(InHandle);

    UCk_Utils_GameplayLabel_UE::Add(InHandle, AbilityName);
    UCk_Utils_Handle_UE::Set_DebugName(InHandle, Get_DisplayName(HandleAsAbility));

    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(CurrentWorld)->Request_TrackAbilityScript(AbilityScriptToUse);
    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(CurrentWorld)->Request_TrackAbilityScript(AbilityCurrent.Get_AbilityScript_DefaultInstance().Get());

    return HandleAsAbility;
}

auto
    UCk_Utils_Ability_UE::
    Request_AddAndGiveAbility(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestAddAndGive& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_TransferExisting_Initiate(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestTransferExisting_Initiate& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_TransferExisting_SwapOwner(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestTransferExisting_SwapOwner& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_TransferExisting_Finalize(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestTransferExisting_Finalize& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_TransferTarget();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_GiveAbility(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestGive& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_RevokeAbility(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestRevoke& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_ActivateAbility(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestActivate& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    Request_DeactivateAbility(
        FCk_Handle_Ability& InAbilityHandle,
        const ck::FFragment_Ability_RequestDeactivate& InRequest,
        ECk_AbilityRequest_PendingSubabilityPolicy AddPendingSubabilityTag)
    -> void
{
    if (AddPendingSubabilityTag == ECk_AbilityRequest_PendingSubabilityPolicy::AddTag)
    {
        auto MutableAbilityOwner = InRequest.Get_AbilityOwner();
        MutableAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
    }
    InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
        InRequest);
}

auto
    UCk_Utils_Ability_UE::
    DoGet_CanBeGiven(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FCk_Handle& InAbilitySource)
    -> bool
{
    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(InAbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO),
        TEXT("Cannot get valid CDO for Ability Script [{}] with AbilityOwner [{}] and AbilitySource [{}]"),
        InAbilityScriptClass, InAbilityOwnerEntity, InAbilitySource)
    { return {}; }

    return AbilityScriptCDO->Get_CanBeGiven(InAbilityOwnerEntity, InAbilitySource);
}

auto
    UCk_Utils_Ability_UE::
    DoOnNotGiven(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FCk_Handle& InAbilitySource)
    -> void
{
    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(InAbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO),
        TEXT("Cannot get valid CDO for Ability Script [{}] with AbilityOwner [{}] and AbilitySource [{}]"),
        InAbilityScriptClass, InAbilityOwnerEntity, InAbilitySource)
    { return; }

    AbilityScriptCDO->DoOnNotGiven(InAbilityOwnerEntity, InAbilitySource);
}

auto
    UCk_Utils_Ability_UE::
    DoCreate_AbilityEntityConfig(
        UObject* InOuter,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        UCk_Ability_Script_PDA* InAbilityArchetype)
    -> UCk_Ability_EntityConfig_PDA*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOuter),
        TEXT("INVALID Outer! Cannot create Ability Entity Config"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityScriptClass),
        TEXT("INVALID Ability Script Class! Cannot create Ability Entity Config"))
    { return {}; }

    const auto& AbilityScript = ck::IsValid(InAbilityArchetype)
                                    ? InAbilityArchetype
                                    : UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(InAbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScript),
        TEXT("Cannot get valid Instance of Ability Script [{}]. Cannot create Ability Entity Config"),
        InAbilityScriptClass)
    { return {}; }

    const auto& AbilityScriptData              = AbilityScript->Get_Data();
    const auto& ConditionSettings              = AbilityScriptData.Get_ConditionSettings();
    const auto& ConditionSettings_Instanced    = AbilityScriptData.Get_ConditionSettings_Instanced();
    const auto& CostSettings                   = AbilityScriptData.Get_CostSettings();
    const auto& CostSettings_Instanced         = AbilityScriptData.Get_CostSettings_Instanced();
    const auto& CooldownSettings               = AbilityScriptData.Get_CooldownSettings();
    const auto& CooldownSettings_Instanced     = AbilityScriptData.Get_CooldownSettings_Instanced();
    const auto& OtherAbilitySettings           = AbilityScriptData.Get_OtherAbilitySettings();
    const auto& OtherAbilitySettings_Instanced = AbilityScriptData.Get_OtherAbilitySettings_Instanced();
    const auto& AbilityCtorScript              = AbilityScriptData.Get_AbilityConstructionScript();
    const auto& NetworkSettings                = AbilityScriptData.Get_NetworkSettings();

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityCtorScript),
        TEXT("Ability Script [{}] specifies an INVALID Ability ConstructionScript. Cannot create Ability Entity Config"),
        InAbilityScriptClass)
    { return {}; }

    const auto& WorldToUse = InOuter->GetWorld();

    const auto& CreateSubAbilityInstances = [&](const TArray<TSubclassOf<UCk_Ability_Script_PDA>>& InSubAbilityClasses)
    {
        return ck::algo::Transform<TArray<UCk_Ability_Script_PDA*>>(InSubAbilityClasses, [&](const TSubclassOf<UCk_Ability_Script_PDA>& InSubAbilityClass)
        {
            CK_ENSURE_IF_NOT(ck::IsValid(InSubAbilityClass), TEXT("Encountered an INVALID sub-ability for Ability [{}]"), InAbilityScriptClass)
            { return static_cast<UCk_Ability_Script_PDA*>(nullptr); }

            const auto& SubAbilityInstance = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_Script_PDA>(WorldToUse, InSubAbilityClass, nullptr, nullptr);

            // We do not need to track this new instance in the ability subsystem since the following will make sure it can't be garbage collected:
            // *  It is initially added to an array UCk_Ability_ConstructionScript_PDA, which itself is tracked by the ability subsystem.
            //      The reference chain prevents the ability instance from being GC'd until FProcessor_AbilityOwner_Setup is called
            // *  In FProcessor_AbilityOwner_Setup, these instanced abilities are given and set as the ability archetype. This then calls DoCreate_AbilityEntityConfig
            //      with this ability instance as the InAbilityArchetype, which is then stored in the UCk_Ability_ConstructionScript_PDA's AbilityParams. This prevents being GC'd
            //      until the ability is added.
            // *  In UCk_Utils_Ability_UE::DoAdd the ability is set as the ability's DefaultInstance and tracked in the ability subsystem. It is untracked when the ability
            //      is revoked.

            return SubAbilityInstance;
        });
    };

    const auto FixupSubAbilityNetworkSettings = [&](const TArray<UCk_Ability_Script_PDA*>& InSubAbilities)
    {
        for (const auto& SubAbility : InSubAbilities)
        {
            CK_ENSURE_IF_NOT(ck::IsValid(SubAbility), TEXT("Encountered INVALID Sub-Ability when trying to instance [{}]"), InAbilityScriptClass)
            { continue; }

            // TODO: We could drive this via (yet another) settings, but the interface is already quite bloated
            if (const auto NeedsFixup = SubAbility->Implements<UCk_Ability_Cost_Interface>() ||
                                        SubAbility->Implements<UCk_Ability_Cooldown_Interface>() ||
                                        SubAbility->Implements<UCk_Ability_Condition_Interface>();
                NOT NeedsFixup)
            { continue; }

            SubAbility->_Data._NetworkSettings = FCk_Ability_NetworkSettings{}
                                                    .Set_ExecutionPolicy(NetworkSettings.Get_ExecutionPolicy())
                                                    .Set_ReplicationType(NetworkSettings.Get_ReplicationType())
                                                    .Set_FeatureReplicationPolicy(ECk_Ability_FeatureReplication_Policy::DoNotReplicateAbilityFeatures);
        }
    };

    auto NewAbilityCtorScript = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_ConstructionScript_PDA>(InOuter, AbilityCtorScript,
    [&](UCk_Ability_ConstructionScript_PDA* InAbilityCtorScript) -> void
    {
        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(CreateSubAbilityInstances(ConditionSettings.Get_ConditionAbilities()));
        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(ConditionSettings_Instanced.Get_ConditionAbilities());

        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(CreateSubAbilityInstances(CostSettings.Get_CostAbilities()));
        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(CostSettings_Instanced.Get_CostAbilities());

        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(CreateSubAbilityInstances(CooldownSettings.Get_CooldownAbilities()));
        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(CooldownSettings_Instanced.Get_CooldownAbilities());

        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(CreateSubAbilityInstances(OtherAbilitySettings.Get_OtherAbilities()));
        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(OtherAbilitySettings_Instanced.Get_OtherAbilities());

        InAbilityCtorScript->_DefaultAbilities_Instanced.Append(AbilityScript->DoGet_AdditionalSubAbilities());

        InAbilityCtorScript->_AbilityParams = FCk_Fragment_Ability_ParamsData{InAbilityScriptClass}.Set_AbilityArchetype(InAbilityArchetype);

        FixupSubAbilityNetworkSettings(InAbilityCtorScript->_DefaultAbilities_Instanced);
    });

    return UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_EntityConfig_PDA>(InOuter, nullptr,
    [&](UCk_Ability_EntityConfig_PDA* InAbilityCtorScript) -> void
    {
        InAbilityCtorScript->_EntityConstructionScript = NewAbilityCtorScript;
    });
}

auto
    UCk_Utils_Ability_UE::
    Request_MarkAbility_AsGiven(
        FCk_Handle_Ability& InAbilityEntity)
    -> void
{
    InAbilityEntity.AddOrGet<ck::FTag_Ability_Given>();
}

auto
    UCk_Utils_Ability_UE::
    Request_MarkAbility_AsNotGiven(
        FCk_Handle_Ability& InAbilityEntity)
    -> void
{
    InAbilityEntity.Remove<ck::FTag_Ability_Given>();
}

auto
    UCk_Utils_Ability_UE::
    Get_IsAbilityGiven(
        FCk_Handle_Ability& InAbilityEntity)
    -> bool
{
    return InAbilityEntity.Has<ck::FTag_Ability_Given>();
}

// --------------------------------------------------------------------------------------------------------------------
