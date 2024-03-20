#include "CkAbility_Subsystem.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Subsystem_UE::
    Request_TrackAbilityScript(
        UCk_Ability_Script_PDA* InAbility)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAbility),
        TEXT("Unable to TrackAbility. Ability is [{}].{}"), InAbility, ck::Context(this))
    { return; }

    _AbilityScripts.Add(InAbility);
}

auto
    UCk_Ability_Subsystem_UE::
    Request_RemoveAbilityScript(
        UCk_Ability_Script_PDA* InAbility)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAbility),
        TEXT("Unable to RemoveAbilityScript. Ability is [{}].{}"), InAbility, ck::Context(this))
    { return; }

    ck::ability::WarningIf(_AbilityScripts.Remove(InAbility) == 0,
        TEXT("Attempted to Remove Ability [{}] but it was never tracked.{}"), InAbility, ck::Context(this));
}

auto
    UCk_Ability_Subsystem_UE::
    Request_TrackAbilityEntityConfig(
        UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityEntityConfig),
        TEXT("Unable to TrackAbilityEntityConfig. Config is [{}].{}"), InAbilityEntityConfig, ck::Context(this))
    { return; }

    _AbilityEntityConfigs.Add(InAbilityEntityConfig);
}

auto
    UCk_Ability_Subsystem_UE::
    Request_RemoveAbilityEntityConfig(
        UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityEntityConfig),
        TEXT("Unable to RemoveAbilityEntityConfig. Config is [{}].{}"), InAbilityEntityConfig, ck::Context(this))
    { return; }

    ck::ability::WarningIf(_AbilityEntityConfigs.Remove(InAbilityEntityConfig) == 0,
        TEXT("Attempted to Remove Ability Entity Config [{}] but it was never tracked.{}"), InAbilityEntityConfig, ck::Context(this));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_Subsystem_UE::
    Get_Subsystem(const UWorld* InWorld)
    -> SubsystemType*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Unable to get the AbilitySubsystem. InWorld is [{}]"),
        InWorld)
    { return nullptr; }

    const auto Subsystem = InWorld->GetSubsystem<SubsystemType>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("World* is Valid but could NOT get a valid [{}]. Is this being called from another WorldSubsystem resulting in an ordering issue?"),
        ck::TypeToString<SubsystemType>)
    { return nullptr; }

    return Subsystem;
}

// --------------------------------------------------------------------------------------------------------------------
