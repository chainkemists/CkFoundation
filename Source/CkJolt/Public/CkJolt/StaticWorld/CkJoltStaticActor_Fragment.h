#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UPrimitiveComponent;
class UCk_Utils_JoltStaticActor_UE;
class UCk_JoltStaticWorld_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_JoltStaticActor_EndPlay;

    // --------------------------------------------------------------------------------------------------------------------

    // An empty _BodyIds array is the idempotence guard for the bidirectional removal helper.
    // _BodiesInScene tracks the collision-sync flip state: bodies flipped out of the scene still EXIST
    // (their ids stay in _BodyIds, ready to re-add) but are absent from the broadphase — the removal
    // funnel must destroy them WITHOUT removing them again.
    struct CKJOLT_API FFragment_JoltStaticActor_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltStaticActor_Current);

    public:
        friend class FProcessor_JoltStaticActor_EndPlay;
        friend class ::UCk_Utils_JoltStaticActor_UE;
        friend class ::UCk_JoltStaticWorld_Subsystem_UE;

    private:
        TArray<uint32>               _BodyIds;
        TWeakObjectPtr<const AActor> _SourceActor;
        FName                        _SourceActorName;

        // Collision-sync bookkeeping: the primitive components whose OnComponentCollisionSettingsChangedEvent
        // this entity is bound to (all of the source actor's primitives for an actor-path entity; the one
        // source component for a component-path entity). _SourceComponent is set ONLY on component-path
        // entities — its validity is what distinguishes the two paths at reconcile time.
        TArray<TWeakObjectPtr<UPrimitiveComponent>> _BoundComponents;
        TWeakObjectPtr<const UPrimitiveComponent>   _SourceComponent;
        bool                                        _BodiesInScene = true;

    public:
        CK_PROPERTY_GET(_BodyIds);
        CK_PROPERTY_GET(_SourceActor);
        CK_PROPERTY_GET(_SourceActorName);
        CK_PROPERTY_GET(_BoundComponents);
        CK_PROPERTY_GET(_SourceComponent);
        CK_PROPERTY_GET(_BodiesInScene);
    };
}

// --------------------------------------------------------------------------------------------------------------------
