#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AbilityOwner_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AbilityOwner_Setup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Params);

    public:
        using ParamsType = FCk_Fragment_AbilityOwner_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

        CK_DEFINE_CONSTRUCTORS(FFragment_AbilityOwner_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Current);

    public:
        friend class FProcessor_AbilityOwner_HandleRequests;

    private:
        FGameplayTagContainer _ActiveTags;

    public:
        CK_PROPERTY_GET(_ActiveTags);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Requests);

    public:
        friend class FProcessor_AbilityOwner_HandleRequests;
        friend class UCk_Utils_AbilityOwner_UE;

    public:
        using GiveAbilityRequestType = FCk_Request_AbilityOwner_GiveAbility;
        using RevokeAbilityRequestType = FCk_Request_AbilityOwner_RevokeAbility;
        using ActivateAbilityRequestType = FCk_Request_AbilityOwner_ActivateAbility;
        using EndAbilityRequestType = FCk_Request_AbilityOwner_EndAbility;

        using RequestType = std::variant<GiveAbilityRequestType, RevokeAbilityRequestType,
            ActivateAbilityRequestType, EndAbilityRequestType>;
        using RequestList = TArray<RequestType>;

    public:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
