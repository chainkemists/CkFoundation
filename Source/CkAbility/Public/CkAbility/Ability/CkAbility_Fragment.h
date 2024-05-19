#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Ability_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKABILITY_API FFragment_Ability_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_Params);

    public:
        using ParamsType = FCk_Fragment_Ability_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_Params, _Params)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_Current);

        friend UCk_Utils_Ability_UE;

    private:
        TWeakObjectPtr<UCk_Ability_Script_PDA> _AbilityScript = nullptr;
        TWeakObjectPtr<UCk_Ability_Script_PDA> _AbilityArchetypeCopy = nullptr;

        ECk_Ability_Status _Status = ECk_Ability_Status::NotActive;

    public:
        CK_PROPERTY_GET(_AbilityScript);
        CK_PROPERTY_GET(_Status);
        CK_PROPERTY_GET(_AbilityArchetypeCopy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_Current, _AbilityScript)
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER(FFragment_Ability_Source);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAbilities, FCk_Handle_Ability);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, OnAbilityActivated, FCk_Delegate_Ability_OnActivated_MC, FCk_Handle_Ability);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, OnAbilityDeactivated, FCk_Delegate_Ability_OnDeactivated_MC, FCk_Handle_Ability);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    struct MatchesAnyAbilityActivationCancelledTags
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTagContainer _Tags;

    public:
        CK_DEFINE_CONSTRUCTOR(MatchesAnyAbilityActivationCancelledTags, _Tags);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct MatchesAbilityScriptClass
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        TSubclassOf<class UCk_Ability_Script_PDA> _ScriptClass;

    public:
        CK_DEFINE_CONSTRUCTOR(MatchesAbilityScriptClass, _ScriptClass);
    };
}

// --------------------------------------------------------------------------------------------------------------------
