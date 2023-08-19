#pragma once

#include "CkIntent_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // TODO: See if empty Tags were really the issue
    struct FTag_Intent_Setup
    {
        bool MakeTagNotBeATag = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTENT_API FCk_Fragment_Intent_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Intent_Params);

    public:
        friend class FCk_Processor_Intent_Setup;

    public:
        FCk_Fragment_Intent_Params() = default;
        explicit FCk_Fragment_Intent_Params(TObjectPtr<UCk_Intent_ReplicatedObject_UE> InIntent_RO);

    private:
        float SomeOtherStaticFloat = 5.0f;
        TObjectPtr<UCk_Intent_ReplicatedObject_UE> _Intent_RO;

    public:
        CK_PROPERTY_GET(_Intent_RO);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINTENT_API FCk_Fragment_Intent_Requests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Intent_Requests);

    public:
        friend class FCk_Processor_Intent_HandleRequests;
        friend class UCk_Utils_Intent_UE;

    public:
        using RequestType = FCk_Request_Intent_NewIntent;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------
}
