#include "CkAbility_Fragment_Data.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

#include <Misc/DataValidation.h>
#include <UObject/ObjectSaveContext.h>

// --------------------------------------------------------------------------------------------------------------------

FCk_Ability_NotActivated_Info::
    FCk_Ability_NotActivated_Info(
        const FCk_Handle_Ability& InAbility,
        ECk_Ability_ActivationRequirementsResult InActivationRequirementsResult)
    : _ActivationRequirementResult(InActivationRequirementsResult)
{
    const auto& AbilityCurrent = InAbility.Get<ck::FFragment_Ability_Current>();
    const auto& Script         = AbilityCurrent.Get_AbilityScript();

    CK_LOG_ERROR_IF_NOT(ck::ability, ck::IsValid(Script),
        TEXT("Cannot check if the Ability [{}] can Activate because it does NOT have a valid Script"),
        InAbility)
    { return; }

    const auto& AbilityOwner = Script->Get_AbilityOwnerHandle();
    _ActiveTagsOnOwner = UCk_Utils_AbilityOwner_UE::Get_ActiveTags(AbilityOwner);

    const auto AbilityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(InAbility);

    if (ck::Is_NOT_Valid(AbilityAsAbilityOwner))
    { return; }

    _ActiveTagsOnSelf = UCk_Utils_AbilityOwner_UE::Get_ActiveTags(AbilityAsAbilityOwner);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_Ability_ParamsData::
    Get_Data() const
    -> const FCk_Ability_Script_Data&
{
    if (ck::IsValid(_AbilityArchetype))
    { return _AbilityArchetype->Get_Data(); }

    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(_AbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO), TEXT("Could not retrieve valid CDO for Ability Script Class [{}]"), _AbilityScriptClass)
    {
        static FCk_Ability_Script_Data Invalid;
        return Invalid;
    }

    return AbilityScriptCDO->Get_Data();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_ConstructionScript_PDA::
    DoConstruct_Implementation(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const
    -> void
{
    UCk_Utils_Ability_UE::DoAdd(InHandle, Get_AbilityParams());

    if (_DefaultAbilities_Instanced.IsEmpty())
    { return; }

    const auto AbilityOwnerParamsData = FCk_Fragment_AbilityOwner_ParamsData{}
                                        .Set_DefaultAbilities_Instanced(_DefaultAbilities_Instanced);

    const auto& AbilityNetworkSettings = Get_AbilityParams().Get_Data().Get_NetworkSettings();
    const auto& AbilityFeatureReplicationPolicy = AbilityNetworkSettings.Get_FeatureReplicationPolicy();


    const auto& AbilityOwnerFeatureReplication = AbilityFeatureReplicationPolicy == ECk_Ability_FeatureReplication_Policy::ReplicateAbilityFeatures
                                                    ? ECk_Replication::Replicates
                                                    : ECk_Replication::DoesNotReplicate;

    if (UCk_Utils_AbilityOwner_UE::Has(InHandle))
    {
        UCk_Utils_AbilityOwner_UE::Append_DefaultAbilities_Instanced(InHandle, _DefaultAbilities_Instanced);
    }
    else
    {
        UCk_Utils_AbilityOwner_UE::Add(InHandle, AbilityOwnerParamsData, AbilityOwnerFeatureReplication);
    }
}

auto
    UCk_Ability_EntityConfig_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

#if WITH_EDITOR
auto
    UCk_Ability_EntityConfig_PDA::
    IsDataValid(
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(Context);

    if (IsTemplate())
    { return Result; }

    if (ck::Is_NOT_Valid(_EntityConstructionScript))
    {
        Context.AddError(FText::FromString(ck::Format_UE(TEXT("Ability Entity Bridge Config [{}] is missing an Entity Construction Script"), this)));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
