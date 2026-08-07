#include "CkUsf_CelPattern_Processor.h"

#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_CelPatternActor_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_CelPatternActor_DropAppliedOnOutline);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_CelPatternActor_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_CelPatternActor_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_cel_pattern_processor
{
    auto
        DoWriteStencil(
            AActor* InActor,
            int32 InStencilValue)
        -> void
    {
        TArray<UPrimitiveComponent*> Primitives;
        InActor->GetComponents(Primitives);

        for (auto* Primitive : Primitives)
        {
            if (ck::Is_NOT_Valid(Primitive))
            { continue; }

            Primitive->SetRenderCustomDepth(true);
            Primitive->SetCustomDepthStencilValue(InStencilValue);
        }
    }

    // DISABLES custom depth; it does not restore whatever the primitive had before this feature wrote to
    // it. That is UCkUsf_OutlineSubsystem::Remove_Outline_From_Component's precedent verbatim — the two
    // features must agree, and a primitive that was already rendering custom depth for a third reason is
    // outside what either can see. Consequence worth knowing: hand-authored custom depth on a mesh does
    // not survive a cel pattern being applied and then cleared.
    auto
        DoDisableCustomDepth(
            const ck::FFragment_Usf_CelPatternApplied_Actor& InApplied)
        -> void
    {
        auto* Actor = InApplied.Get_Actor().Get();
        if (ck::Is_NOT_Valid(Actor))
        { return; } // actor already gone — nothing left to disable

        TArray<UPrimitiveComponent*> Primitives;
        Actor->GetComponents(Primitives);

        for (auto* Primitive : Primitives)
        {
            if (ck::Is_NOT_Valid(Primitive))
            { continue; }

            // Only undo what THIS feature wrote: another system (an outline) may have taken the value over
            // in the meantime, and turning its custom depth off would erase its silhouette.
            if (Primitive->CustomDepthStencilValue != InApplied.Get_StencilValue())
            { continue; }

            Primitive->SetRenderCustomDepth(false);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Usf_CelPatternActor_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternTarget& InTarget,
            const FFragment_OwningActor_Current& InOwningActor)
        -> void
    {
        auto* Actor = InOwningActor.Get_EntityOwningActor().Get();
        if (ck::Is_NOT_Valid(Actor))
        { return; }

        auto* Subsystem = UCkUsf_CelShadeSubsystem::Get_CelShadeSubsystem(Actor);
        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        // 0 means the stencil contract is switched off in this world's settings — writing it would mark the
        // mesh with the engine's "nothing here" value and read as a silent success.
        const auto StencilValue = Subsystem->Get_StencilValueFor(InTarget.Get_Pattern());
        if (StencilValue == 0)
        { return; }

        if (InHandle.Has<FFragment_Usf_CelPatternApplied_Actor>())
        {
            const auto& Applied = InHandle.Get<FFragment_Usf_CelPatternApplied_Actor>();

            if (Applied.Get_Pattern() == InTarget.Get_Pattern() &&
                Applied.Get_Actor() == Actor &&
                Applied.Get_StencilValue() == StencilValue)
            { return; }

            ck_usf_cel_pattern_processor::DoDisableCustomDepth(Applied);
        }

        ck_usf_cel_pattern_processor::DoWriteStencil(Actor, StencilValue);
        InHandle.AddOrGet<FFragment_Usf_CelPatternApplied_Actor>() =
            FFragment_Usf_CelPatternApplied_Actor{InTarget.Get_Pattern(), Actor, StencilValue};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_CelPatternActor_DropAppliedOnOutline::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternApplied_Actor& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget)
        -> void
    {
        // Try_Remove, not Remove: the plain-removal processor's view also matches an entity whose cel
        // TARGET is gone as well, and the two run in an unspecified order within their group.
        InHandle.Try_Remove<FFragment_Usf_CelPatternApplied_Actor>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_CelPatternActor_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternApplied_Actor& InApplied)
        -> void
    {
        ck_usf_cel_pattern_processor::DoDisableCustomDepth(InApplied);
        InHandle.Remove<FFragment_Usf_CelPatternApplied_Actor>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_CelPatternActor_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternApplied_Actor& InApplied)
        -> void
    {
        ck_usf_cel_pattern_processor::DoDisableCustomDepth(InApplied);
        InHandle.Remove<FFragment_Usf_CelPatternApplied_Actor>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
