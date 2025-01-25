#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Ability_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Ability_Params = FCk_Fragment_Ability_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_RequestAddAndGive : FRequest_Base
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RequestAddAndGive);

        friend UCk_Utils_Ability_UE;

    private:
        FCk_Handle_AbilityOwner _AbilityOwner;
        FCk_Handle _AbilitySource;
        FCk_Ability_Payload_OnGranted _Payload;

    public:
        CK_PROPERTY_GET(_AbilityOwner);
        CK_PROPERTY_GET(_AbilitySource);
        CK_PROPERTY_GET(_Payload);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_RequestAddAndGive, _AbilityOwner, _AbilitySource, _Payload)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_RequestTransferExisting : FRequest_Base
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RequestTransferExisting);

        friend UCk_Utils_Ability_UE;

    private:
        FCk_Handle_AbilityOwner _AbilityOwner;
        FCk_Handle_AbilityOwner _TransferTarget;
        FCk_Handle_Ability _AbilityToTransfer;

    public:
        CK_PROPERTY_GET(_AbilityOwner);
        CK_PROPERTY_GET(_AbilityToTransfer);
        CK_PROPERTY_GET(_TransferTarget);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_RequestTransferExisting, _AbilityOwner, _TransferTarget, _AbilityToTransfer);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_RequestGive : FFragment_Ability_RequestAddAndGive
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RequestGive);

        friend UCk_Utils_Ability_UE;

        using FFragment_Ability_RequestAddAndGive::FFragment_Ability_RequestAddAndGive;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_RequestRevoke : FRequest_Base
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RequestGive);

        friend UCk_Utils_Ability_UE;

    private:
        FCk_Handle_AbilityOwner _AbilityOwner;
        ECk_AbilityOwner_DestructionOnRevoke_Policy _DestructionPolicy = ECk_AbilityOwner_DestructionOnRevoke_Policy::DestroyOnRevoke;

    public:
        CK_PROPERTY_GET(_AbilityOwner);
        CK_PROPERTY_GET(_DestructionPolicy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_RequestRevoke, _AbilityOwner, _DestructionPolicy)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_RequestActivate : FRequest_Base
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RequestActivate);

        friend UCk_Utils_Ability_UE;

    private:
        FCk_Handle_AbilityOwner _AbilityOwner;
        FCk_Ability_Payload_OnActivate _Payload;

    public:
        CK_PROPERTY_GET(_AbilityOwner);
        CK_PROPERTY_GET(_Payload);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_RequestActivate, _AbilityOwner, _Payload)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_RequestDeactivate : FRequest_Base
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RequestDeactivate);

        friend UCk_Utils_Ability_UE;

    private:
        FCk_Handle_AbilityOwner _AbilityOwner;

    public:
        CK_PROPERTY_GET(_AbilityOwner);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_RequestDeactivate, _AbilityOwner)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_Requests);

    public:
        friend class UCk_Utils_Ability_UE;

    public:
        using AddAndGive = FFragment_Ability_RequestAddAndGive;
        using Transfer = FFragment_Ability_RequestTransferExisting;
        using Give = FFragment_Ability_RequestGive;
        using Revoke = FFragment_Ability_RequestRevoke;
        using Activate = FFragment_Ability_RequestActivate;
        using Deactivate = FFragment_Ability_RequestDeactivate;

        using RequestType = std::variant<AddAndGive, Transfer, Give, Revoke, Activate, Deactivate>;
        using RequestList = TArray<RequestType>;

    public:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_Ability_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_Current);

        friend UCk_Utils_Ability_UE;
        friend class FProcessor_Ability_HandleRequests;

    private:
        TWeakObjectPtr<UCk_Ability_Script_PDA> _AbilityScript = nullptr;

        // Represents the instance Archetype provided OR the CDO if the Archetype is invalid
        TWeakObjectPtr<UCk_Ability_Script_PDA> _AbilityScript_DefaultInstance = nullptr;

        ECk_Ability_Status _Status = ECk_Ability_Status::NotActive;

    public:
        CK_PROPERTY_GET(_AbilityScript);
        CK_PROPERTY_GET(_Status);
        CK_PROPERTY_GET(_AbilityScript_DefaultInstance);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ability_Current, _AbilityScript)
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER(FFragment_Ability_Source);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAbilities, FCk_Handle_Ability);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, OnAbilityActivated, FCk_Delegate_Ability_OnActivated_MC, FCk_Handle_Ability, FCk_Ability_Payload_OnActivate);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, OnAbilityDeactivated, FCk_Delegate_Ability_OnDeactivated_MC, FCk_Handle_Ability);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    struct MatchesAnyAbilityActivationCancelledTagsOnSelf
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTagContainer _Tags;

    public:
        CK_DEFINE_CONSTRUCTOR(MatchesAnyAbilityActivationCancelledTagsOnSelf, _Tags);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct MatchesAnyAbilityActivationCancelledTagsOnOwner
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTagContainer _Tags;

    public:
        CK_DEFINE_CONSTRUCTOR(MatchesAnyAbilityActivationCancelledTagsOnOwner, _Tags);
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
