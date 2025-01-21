#pragma once

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkAbility_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Ability"))
class CKABILITY_API UCk_Ability_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Recycling",
              meta = (AllowPrivateAccess = true, ValidEnumValues="Recycle"))
    ECk_Ability_RecyclingPolicy _AbilityRecyclingPolicy = ECk_Ability_RecyclingPolicy::Recycle;

    // Cue types for the Cue Aggregators to discover
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Cues",
              meta = (AllowPrivateAccess = true, AllowAbstract))
    TArray<TSubclassOf<UCk_Ability_Script_PDA>> _CueTypes;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Cues",
              meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1))
    int32 _NumberOfCueReplicators = 4;

public:
    CK_PROPERTY_GET(_AbilityRecyclingPolicy);
    CK_PROPERTY_GET(_CueTypes);
    CK_PROPERTY_GET(_NumberOfCueReplicators);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Ability"))
class CKABILITY_API UCk_Ability_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_UserSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AbilityNotActivatedDebug = ECk_EnableDisable::Disable;

    // Disable/Enable the Debug processor that logs Abilities that have the PendingOperation Tag
    // every pump
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug",
              meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1))
    ECk_EnableDisable _LogResolvePendingOperationTags = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_AbilityNotActivatedDebug);
    CK_PROPERTY_GET(_LogResolvePendingOperationTags);
};

// --------------------------------------------------------------------------------------------------------------------

class CKABILITY_API UCk_Utils_Ability_Settings_UE
{
public:
    static auto Get_AbilityRecyclingPolicy() -> ECk_Ability_RecyclingPolicy;
    static auto Get_CueTypes() -> const TArray<TSubclassOf<UCk_Ability_Script_PDA>>&;
    static auto Get_NumberOfCueReplicators() -> int32;

    static auto Get_AbilityNotActivatedDebug() -> const ECk_EnableDisable;
    static auto Get_LogResolvePendingOperationTags() -> ECk_EnableDisable;
};

// --------------------------------------------------------------------------------------------------------------------
