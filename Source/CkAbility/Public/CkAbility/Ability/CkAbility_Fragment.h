#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

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
        ECk_Ability_Status _Status = ECk_Ability_Status::NotActive;

    public:
        CK_PROPERTY_GET(_AbilityScript);
        CK_PROPERTY_GET(_Status);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_Current, _AbilityScript)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfAbilities : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, OnAbilityActivated, FCk_Delegate_Ability_OnActivated_MC, FCk_Handle);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, OnAbilityDeactivated, FCk_Delegate_Ability_OnDeactivated_MC, FCk_Handle);
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
}

// --------------------------------------------------------------------------------------------------------------------
