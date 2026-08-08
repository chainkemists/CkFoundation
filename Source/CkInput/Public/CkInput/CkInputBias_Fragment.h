#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkInput/CkInputBias_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InputBias_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_InputBias_Params = FCk_Fragment_InputBias_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    // The conditioned state consumers sample, one row per axis key that has ever been seen. It is derived state and
    // nothing but the conditioning processor writes it: the recorded raw rows stay physical facts, and biasing a
    // stick can never rewrite what the device actually reported.
    //
    // A row survives until the axis is sampled again, so a value read here is the LAST conditioned value rather
    // than this frame's — an axis that stopped sending events keeps reporting where it was left.
    struct CKINPUT_API FFragment_InputBias_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InputBias_Current);

    public:
        friend class FProcessor_InputBias_Condition;

    private:
        TArray<FCk_InputBias_ConditionedAxis> _ConditionedAxes;

    public:
        CK_PROPERTY_GET(_ConditionedAxes);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKINPUT_API FFragment_InputBias_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InputBias_Requests);

    public:
        friend class FProcessor_InputBias_HandleRequests;
        friend class ::UCk_Utils_InputBias_UE;

    public:
        using RequestType = std::variant<FCk_Request_InputBias_SetAxisBias>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
