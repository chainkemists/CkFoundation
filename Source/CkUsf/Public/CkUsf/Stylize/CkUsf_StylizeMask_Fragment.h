#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Declarative "this entity is inside the stylize effect mask" marker, written by
    // UCk_Utils_Usf_StylizeMask_UE; a per-renderer sync processor turns it into the project's single mask
    // Custom-Stencil value. Cascade-derived targets are removed by Request_RemoveFromStylizeMask on the
    // parent; explicit targets never are.
    //
    // Carries no value of its own — unlike a cel pattern there is exactly ONE mask stencil value per
    // project, so membership is the whole payload.
    struct CKUSF_API FFragment_Usf_StylizeMaskTarget
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_StylizeMaskTarget);

    private:
        bool _IsCascadeDerived = false;

    public:
        CK_PROPERTY_GET_BY_COPY(_IsCascadeDerived);

        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_StylizeMaskTarget, _IsCascadeDerived);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applied-state for the actor-backed path: records the actor + stencil value written, so removal /
    // EndPlay can undo without the Target fragment and so the sync processor can tell a no-op refresh
    // from a real change (the project's mask value is config and can move between sessions).
    struct CKUSF_API FFragment_Usf_StylizeMaskApplied_Actor
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_StylizeMaskApplied_Actor);

    private:
        TWeakObjectPtr<AActor> _Actor;
        int32 _StencilValue = 0;

    public:
        CK_PROPERTY_GET(_Actor);
        CK_PROPERTY_GET_BY_COPY(_StencilValue);

        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_StylizeMaskApplied_Actor, _Actor, _StencilValue);
    };
}

// --------------------------------------------------------------------------------------------------------------------
