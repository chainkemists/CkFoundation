#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Entity/CkEntity.h"

#include "CkHandle_Subsystem.generated.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkAbility_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKABILITY_API UCk_Ability_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_Subsystem_UE);

public:
    auto
    Request_TrackAbilityScript(
        class UCk_Ability_Script_PDA* InAbility) -> void;

    auto
    Request_UntrackAbilityScript(
        class UCk_Ability_Script_PDA* InAbility) -> void;

    auto
    Request_TrackAbilityEntityConfig(
        class UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig) -> void;

    auto
    Request_UntrackAbilityEntityConfig(
        class UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _AbilityScripts;

    UPROPERTY(Transient)
    TArray<TObjectPtr<class UCk_Ability_EntityConfig_PDA>> _AbilityEntityConfigs;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_Ability_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_Ability_Subsystem_UE;

public:
    static auto
    Get_Subsystem(const UWorld* InWorld) -> SubsystemType*;
};

// --------------------------------------------------------------------------------------------------------------------
