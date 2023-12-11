#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

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
        FCk_Handle _ActivationContextEntity;

    public:
        CK_PROPERTY_GET(_AbilityScript);
        CK_PROPERTY_GET(_Status);
        CK_PROPERTY_GET(_ActivationContextEntity);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_Current, _AbilityScript)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfAbilities : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };
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
