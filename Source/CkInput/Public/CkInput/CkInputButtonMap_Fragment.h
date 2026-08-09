#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkInput/CkInputButtonMap_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_InputButtonMap_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_InputButtonMap_Params = FCk_Fragment_InputButtonMap_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    // The two-way association state: every button this map has ever minted, each paired with the key that
    // currently produces it. Rows are only ever added or re-pointed, never removed, because an identity is a
    // promise to whatever named it.
    //
    // Key -> button is ONE-TO-MANY and the rows are kept as a flat list rather than two indices for exactly
    // that reason: two mappings in different categories legitimately share a key, and duplicate bindings exist
    // in shipped profiles. A reverse lookup therefore has to answer with every holder, which a scan does
    // honestly and a key-indexed map only does by storing the same list again.
    struct CKINPUT_API FFragment_InputButtonMap_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_InputButtonMap_Current);

    public:
        friend class FProcessor_InputButtonMap_HandleRequests;

    private:
        TArray<FCk_Input_ButtonAssociation> _Associations;

    public:
        CK_PROPERTY_GET(_Associations);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKINPUT_API FFragment_InputButtonMap_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InputButtonMap_Requests);

    public:
        friend class FProcessor_InputButtonMap_HandleRequests;
        friend class ::UCk_Utils_InputButtonMap_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_InputButtonMap_Rederive,
            FCk_Request_InputButtonMap_RegisterPhysicalButton>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
