#pragma once

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkAbility_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Ability"))
class CKABILITY_API UCk_Ability_ProjectSettings_UE : public UCk_Game_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Recycling",
              meta = (AllowPrivateAccess = true, ValidEnumValues="Recycle"))
    ECk_Ability_RecyclingPolicy _AbilityRecyclingPolicy = ECk_Ability_RecyclingPolicy::Recycle;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Default Tags",
              meta = (AllowPrivateAccess = true))
    FGameplayTag _ApplyCooldownTag;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Default Tags",
              meta = (AllowPrivateAccess = true))
    FGameplayTag _ApplyCostTag;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Default Tags",
              meta = (AllowPrivateAccess = true))
    FGameplayTag _ConditionsNotMetTag;

public:
    CK_PROPERTY_GET(_AbilityRecyclingPolicy);

    CK_PROPERTY_GET(_ApplyCooldownTag);
    CK_PROPERTY_GET(_ApplyCostTag);
    CK_PROPERTY_GET(_ConditionsNotMetTag);
};

// --------------------------------------------------------------------------------------------------------------------

class CKABILITY_API UCk_Utils_Ability_ProjectSettings_UE
{
public:
    static auto Get_AbilityRecyclingPolicy() -> ECk_Ability_RecyclingPolicy;

    static auto Get_Default_ApplyCooldownTag() -> FGameplayTag;
    static auto Get_Default_ApplyCostTag() -> FGameplayTag;
    static auto Get_Default_ConditionsNotMetTag() -> FGameplayTag;
};

// --------------------------------------------------------------------------------------------------------------------
