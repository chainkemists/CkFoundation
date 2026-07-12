#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_OutlinePreset;
class AActor;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Declarative "outline this entity" marker, written by UCk_Utils_Usf_Outline_UE. Each renderer module
    // (CkUsf's actor path below, CkIsmRenderer's shadow ISM, CkIskmRenderer's SKMC path) owns a sync
    // processor that reacts to (OutlineTarget + its own proxy fragment) and applies the outline via its
    // mechanism, recording what it applied in a module-local `...OutlineApplied` fragment. Cascade-derived
    // targets (stamped on lifetime dependents by ECk_Usf_OutlineScope::EntityAndDependents) are removed by
    // Request_RemoveOutline on the parent; explicit targets never are.
    struct CKUSF_API FFragment_Usf_OutlineTarget
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_OutlineTarget);

    private:
        TWeakObjectPtr<UCkUsf_OutlinePreset> _Preset;
        bool _IsCascadeDerived = false;

    public:
        CK_PROPERTY_GET(_Preset);
        CK_PROPERTY_GET_BY_COPY(_IsCascadeDerived);

        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_OutlineTarget, _Preset, _IsCascadeDerived);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applied-state for the actor-backed path (entities with FFragment_OwningActor_Current): records the
    // preset + actor the outline was applied to so removal/EndPlay can undo without the Target fragment.
    struct CKUSF_API FFragment_Usf_OutlineApplied_Actor
    {
    public:
        CK_GENERATED_BODY(FFragment_Usf_OutlineApplied_Actor);

    private:
        TWeakObjectPtr<UCkUsf_OutlinePreset> _Preset;
        TWeakObjectPtr<AActor> _Actor;

    public:
        CK_PROPERTY_GET(_Preset);
        CK_PROPERTY_GET(_Actor);

        CK_DEFINE_CONSTRUCTORS(FFragment_Usf_OutlineApplied_Actor, _Preset, _Actor);
    };
}

// --------------------------------------------------------------------------------------------------------------------
