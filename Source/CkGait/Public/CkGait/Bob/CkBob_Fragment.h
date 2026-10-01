#pragma once

#include "CkGait/Bob/CkBob_Fragment_Data.h"
#include "CkGait/Bob/CkBob_Kernel.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Bob_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Bob_Disabled);
    // Latched the first time something other than the Bob processor wrote this node's offset (Sway's rule).
    CK_DEFINE_ECS_TAG(FTag_Bob_ForeignOffsetReported);

    // --------------------------------------------------------------------------------------------------------------------

    // Wholesale alias (CkSway precedent). Tunables, not Params: Request_UpdateSpec replaces it at runtime.
    using FFragment_Bob_Tunables = FCk_Bob_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGAIT_API FFragment_Bob
    {
    public:
        CK_GENERATED_BODY(FFragment_Bob);

    public:
        friend class FProcessor_Bob_HandleRequests;
        friend class FProcessor_Bob_Update;
        friend class UCk_Utils_Bob_UE;

    private:
        // Node offset = BobOffset * _RestOffset. Seeded from the node's offset at Add.
        FTransform _RestOffset = FTransform::Identity;
        bob::FSpringState _Spring;
        bob::FTarget _Smoothed;
        // The gait landing count this bob has already reacted to. Seeded from the gait at Add.
        int32 _ConsumedLandingCount = 0;
        FTransform _LastWrittenOffset = FTransform::Identity;

    public:
        CK_PROPERTY_GET(_RestOffset);
        CK_PROPERTY_GET(_Spring);
        CK_PROPERTY_GET(_Smoothed);
        CK_PROPERTY_GET(_ConsumedLandingCount);
        CK_PROPERTY_GET(_LastWrittenOffset);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Bob, _RestOffset, _ConsumedLandingCount, _LastWrittenOffset);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGAIT_API FFragment_Bob_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Bob_Requests);

        friend class FProcessor_Bob_HandleRequests;
        friend class FProcessor_Bob_CancelPendingRequests;
        friend class UCk_Utils_Bob_UE;

        using RequestType = std::variant<FCk_Request_Bob_UpdateSpec, FCk_Request_Bob_EnableDisable, FCk_Request_Bob_Reset, FCk_Request_Bob_SetRestOffset>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Bob_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
