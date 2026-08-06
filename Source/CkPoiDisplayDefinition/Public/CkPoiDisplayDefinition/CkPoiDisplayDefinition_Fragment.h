#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_PoiDisplayDefinition_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_PoiDisplayDefinition_HandleRequests;

    // Plain, not counted: one parent, one source — see CLAUDE.md "The cascade contract"
    CK_DEFINE_ECS_TAG(FTag_PoiDisplayDefinition_ParentHidden);

    // Bind-once guard on the OWNER entity, so a second Create does not double-bind the cascade
    CK_DEFINE_ECS_TAG(FTag_PoiDisplayDefinition_CascadeBound);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_PoiDisplayDefinition_Params = FCk_PoiDisplayDefinition_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    // The LIVE display state, seeded from Params by Add and mutated only through the request processor.
    // Consumers read this (never Params) so that a per-instance override — a rental going overdue recolouring
    // one customer's marker — is visible to every projector without re-authoring shared config.
    //
    // Only the MUTABLE half of the visual state lives here. The icon is immutable post-Add and is read
    // straight off Params, so it is deliberately absent.
    struct CKPOIDISPLAYDEFINITION_API FFragment_PoiDisplayDefinition_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_PoiDisplayDefinition_Current);

    public:
        friend class FProcessor_PoiDisplayDefinition_HandleRequests;
        friend class ::UCk_Utils_PoiDisplayDefinition_UE;

    private:
        FLinearColor _Tint = FLinearColor::White;

        FVector2D _SizeHint = FVector2D{32.0, 32.0};

    public:
        CK_PROPERTY_GET(_Tint);
        CK_PROPERTY_GET(_SizeHint);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPOIDISPLAYDEFINITION_API FFragment_PoiDisplayDefinition_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_PoiDisplayDefinition_Requests);

    public:
        friend class FProcessor_PoiDisplayDefinition_HandleRequests;
        friend class ::UCk_Utils_PoiDisplayDefinition_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_PoiDisplayDefinition_SetTint,
            FCk_Request_PoiDisplayDefinition_SetSizeHint>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfPoiDisplayDefinitions, FCk_Handle_PoiDisplayDefinition);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOIDISPLAYDEFINITION_API, OnPoiDisplayDefinition_DisplayChanged, FCk_Delegate_PoiDisplayDefinition_DisplayChanged, FCk_Handle_PoiDisplayDefinition);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_PoiDisplayDefinition_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
