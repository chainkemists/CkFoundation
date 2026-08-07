#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Declarative "render this entity with THIS cel pattern" marker, written by
    // UCk_Utils_Usf_CelPattern_UE; a per-renderer sync processor turns it into a Custom-Stencil value
    // (actor path in CkUsf; ISM/ISKM proxies are not covered). Cascade-derived targets are removed by
    // Request_ClearCelPattern on the parent; explicit targets never are.
    struct CKUSF_API FFragment_Usf_CelPatternTarget
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_CelPatternTarget);

    private:
        ECk_Usf_CelPattern _Pattern = ECk_Usf_CelPattern::Bayer;
        bool _IsCascadeDerived = false;

    public:
        CK_PROPERTY_GET_BY_COPY(_Pattern);
        CK_PROPERTY_GET_BY_COPY(_IsCascadeDerived);

        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_CelPatternTarget, _Pattern, _IsCascadeDerived);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applied-state for the actor-backed path (entities with FFragment_OwningActor_Current): records the
    // pattern + actor + stencil value written, so removal/EndPlay can undo without the Target fragment and
    // so the sync processor can tell a no-op refresh from a real change.
    struct CKUSF_API FFragment_Usf_CelPatternApplied_Actor
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_CelPatternApplied_Actor);

    private:
        ECk_Usf_CelPattern _Pattern = ECk_Usf_CelPattern::Bayer;
        TWeakObjectPtr<AActor> _Actor;
        int32 _StencilValue = 0;

    public:
        CK_PROPERTY_GET_BY_COPY(_Pattern);
        CK_PROPERTY_GET(_Actor);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_CelPatternApplied_Actor, _Pattern, _Actor, _StencilValue);
    };
}

// --------------------------------------------------------------------------------------------------------------------
