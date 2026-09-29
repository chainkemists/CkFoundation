#pragma once

#include "CkSway/CkSway_Fragment_Data.h"
#include "CkSway/CkSway_Kernel.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Sway_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Sway_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Sway_Disabled);
    // Latched the first time something other than the Sway processor wrote this node's offset (DESIGN §7).
    CK_DEFINE_ECS_TAG(FTag_Sway_ForeignOffsetReported);

    // --------------------------------------------------------------------------------------------------------------------

    // Wholesale alias (CkChain/CkTween precedent). Tunables, not Params: Request_UpdateSpec replaces it at runtime.
    // _StartingState is consumed once by Setup (into FTag_Sway_Disabled); nothing reads it afterwards.
    using FFragment_Sway_Tunables = FCk_Sway_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSWAY_API FFragment_Sway
    {
    public:
        CK_GENERATED_BODY(FFragment_Sway);

    public:
        friend class FProcessor_Sway_Setup;
        friend class FProcessor_Sway_HandleRequests;
        friend class FProcessor_Sway_Update;
        friend class UCk_Utils_Sway_UE;

    private:
        // Node offset = SwayOffset * _RestOffset (sway::Compose_NodeOffset). Seeded from the node's offset at Add.
        FTransform _RestOffset = FTransform::Identity;
        sway::FChannelState _Location;
        sway::FChannelState _Rotation;
        TOptional<FTransform> _LastDriverWorld;
        FTransform _LastWrittenOffset = FTransform::Identity;
        FCk_Sway_Stimulus _LastStimulus;

    public:
        CK_PROPERTY_GET(_RestOffset);
        CK_PROPERTY_GET(_Location);
        CK_PROPERTY_GET(_Rotation);
        CK_PROPERTY_GET(_LastDriverWorld);
        CK_PROPERTY_GET(_LastWrittenOffset);
        CK_PROPERTY_GET(_LastStimulus);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sway, _RestOffset);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSWAY_API FFragment_Sway_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sway_Requests);

        friend class FProcessor_Sway_HandleRequests;
        friend class FProcessor_Sway_CancelPendingRequests;
        friend class UCk_Utils_Sway_UE;

        using RequestType = std::variant<FCk_Request_Sway_UpdateSpec, FCk_Request_Sway_EnableDisable, FCk_Request_Sway_Reset, FCk_Request_Sway_SetRestOffset>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Sway_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
