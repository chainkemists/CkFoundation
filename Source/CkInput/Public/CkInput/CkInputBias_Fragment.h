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
    // _Tunables rather than _Params: Request_SetAxisBias adds-or-updates rows here at runtime, and there is no
    // immutable half of the Spec to split off — the whole table is the tunable. "Params" would promise the
    // conditioning pass could take it TReadOnly, and it cannot.
    struct CKINPUT_API FFragment_InputBias_Tunables
    {
    public:
        CK_GENERATED_BODY(FFragment_InputBias_Tunables);

    public:
        friend class FProcessor_InputBias_HandleRequests;

    private:
        TArray<FCk_InputBias_AxisBias> _AxisBiases;

    public:
        CK_PROPERTY_GET(_AxisBiases);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InputBias_Tunables, _AxisBiases);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The conditioned state consumers sample, one row per axis key that has ever been seen. It is derived state and
    // nothing but the conditioning processor writes it: the recorded raw rows stay physical facts, and biasing a
    // stick can never rewrite what the device actually reported.
    //
    // A row survives until the axis is sampled again, so a value read here is the LAST conditioned value rather
    // than this frame's — an axis that stopped sending events keeps reporting where it was left.
    struct CKINPUT_API FFragment_InputBias
    {
    public:
        CK_GENERATED_BODY(FFragment_InputBias);

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
