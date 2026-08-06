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

    // The retained immutable residue of FCk_PoiDisplayDefinition_Spec: the fields projectors read
    // straight off Params every update. _Tint and _SizeHint are DISSOLVED - they are seeded into
    // FFragment_PoiDisplayDefinition_Current at Add and mutated only through the request processor,
    // so the authored copies would go stale the instant anything called Request_Set*. Leaving them
    // here made "never draw from Params" a rule the reader had to know; omitting them makes it a
    // compile error.
    struct CKPOIDISPLAYDEFINITION_API FFragment_PoiDisplayDefinition_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_PoiDisplayDefinition_Params);

    private:
        FGameplayTag _Consumer;
        TSoftObjectPtr<UTexture2D> _Icon;
        int32 _Priority = 0;
        ECk_Poi_OffscreenPolicy _OffscreenPolicy = ECk_Poi_OffscreenPolicy::Hide;

    public:
        CK_PROPERTY_GET(_Consumer);
        CK_PROPERTY_GET(_Icon);
        CK_PROPERTY_GET(_Priority);
        CK_PROPERTY_GET(_OffscreenPolicy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_PoiDisplayDefinition_Params, _Consumer, _Icon, _Priority,
            _OffscreenPolicy);
    };

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
