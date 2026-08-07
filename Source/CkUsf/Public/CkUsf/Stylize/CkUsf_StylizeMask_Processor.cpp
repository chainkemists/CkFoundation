#include "CkUsf_StylizeMask_Processor.h"

#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_StylizeMaskActor_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_StylizeMaskActor_DropAppliedOnOutline);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_StylizeMaskActor_DropAppliedOnCelPattern);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_StylizeMaskActor_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_StylizeMaskActor_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_stylize_mask_processor
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
    // it. UCkUsf_OutlineSubsystem::Remove_Outline_From_Component's precedent verbatim, shared with the cel
    // pattern — all three features must agree, and a primitive already rendering custom depth for a third
    // reason is outside what any of them can see.
    auto
        DoDisableCustomDepth(
            const ck::FFragment_Usf_StylizeMaskApplied_Actor& InApplied)
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

            // Only undo what THIS feature wrote: another system (an outline, a cel pattern) may have taken
            // the value over in the meantime, and turning its custom depth off would erase its silhouette.
            if (Primitive->CustomDepthStencilValue != InApplied.Get_StencilValue())
            { continue; }

            Primitive->SetRenderCustomDepth(false);
        }
    }

    // Whether every primitive still carries what this feature wrote. The applied-state alone is not
    // enough: another feature's undo can turn custom depth off underneath us (an outline removed from a
    // component this feature had claimed first), and without re-checking the primitive the sync
    // early-outs on its own cache forever and the mask never comes back. The ISKM proxy path re-asserts
    // per frame for the same reason; this is the actor path's equivalent, restricted to a comparison so
    // an unchanged frame still costs no writes.
    auto
        DoGet_StencilStillApplied(
            AActor* InActor,
            int32 InStencilValue)
        -> bool
    {
        TArray<UPrimitiveComponent*> Primitives;
        InActor->GetComponents(Primitives);

        for (auto* Primitive : Primitives)
        {
            if (ck::Is_NOT_Valid(Primitive))
            { continue; }

            if (NOT Primitive->bRenderCustomDepth ||
                Primitive->CustomDepthStencilValue != InStencilValue)
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Usf_StylizeMaskActor_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskTarget& InTarget,
            const FFragment_OwningActor_Current& InOwningActor)
        -> void
    {
        auto* Actor = InOwningActor.Get_EntityOwningActor().Get();
        if (ck::Is_NOT_Valid(Actor))
        { return; }

        // Project config rather than a per-world subsystem: the mask claims ONE byte value for the whole
        // project, and no world-local state narrows it. Read every frame so an editor-side change lands
        // without a restart, the way the cel contract's stencil base does.
        const auto StencilValue = UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue();

        // 0 is the engine's "nothing written here" — writing it would mark the mesh with the value that
        // means untagged and read as a silent success. The project setting clamps at 1, so this only
        // fires on an .ini hand-edited past the clamp.
        if (StencilValue <= 0)
        { return; }

        if (InHandle.Has<FFragment_Usf_StylizeMaskApplied_Actor>())
        {
            const auto& Applied = InHandle.Get<FFragment_Usf_StylizeMaskApplied_Actor>();

            if (Applied.Get_Actor() == Actor &&
                Applied.Get_StencilValue() == StencilValue &&
                ck_usf_stylize_mask_processor::DoGet_StencilStillApplied(Actor, StencilValue))
            { return; }

            ck_usf_stylize_mask_processor::DoDisableCustomDepth(Applied);
        }

        ck_usf_stylize_mask_processor::DoWriteStencil(Actor, StencilValue);
        InHandle.AddOrGet<FFragment_Usf_StylizeMaskApplied_Actor>() =
            FFragment_Usf_StylizeMaskApplied_Actor{Actor, StencilValue};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_StylizeMaskActor_DropAppliedOnOutline::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget)
        -> void
    {
        // UNDO before dropping the state that records what to undo. Dropping alone assumes the higher
        // claim's own Sync will overwrite the byte this frame — but that Sync can silently no-op (a null
        // subsystem, a disabled stencil contract, an exhausted outline range), and once this fragment is
        // gone neither _Remove nor _EndPlay matches the entity, so the mesh keeps this feature's stencil
        // forever with nothing left that knows to clear it. The undo is value-guarded, so if the higher
        // claim HAS already written, this is a no-op. The one-frame gap where the mesh renders no custom
        // depth is the accepted cel/outline behaviour.
        ck_usf_stylize_mask_processor::DoDisableCustomDepth(InApplied);

        // Try_Remove, not Remove: the co-matcher is the CEL-PATTERN drop twin, on an entity carrying both
        // higher claims at once, and the order within their group is unspecified. (_Remove is not a
        // co-matcher — it excludes both higher targets, so it can never see this entity.)
        InHandle.Try_Remove<FFragment_Usf_StylizeMaskApplied_Actor>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_StylizeMaskActor_DropAppliedOnCelPattern::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied,
            const FFragment_Usf_CelPatternTarget& InCelPatternTarget)
        -> void
    {
        // Undo first — same reasoning as the outline twin above.
        ck_usf_stylize_mask_processor::DoDisableCustomDepth(InApplied);
        InHandle.Try_Remove<FFragment_Usf_StylizeMaskApplied_Actor>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_StylizeMaskActor_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied)
        -> void
    {
        ck_usf_stylize_mask_processor::DoDisableCustomDepth(InApplied);
        InHandle.Remove<FFragment_Usf_StylizeMaskApplied_Actor>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_StylizeMaskActor_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskApplied_Actor& InApplied)
        -> void
    {
        ck_usf_stylize_mask_processor::DoDisableCustomDepth(InApplied);
        InHandle.Remove<FFragment_Usf_StylizeMaskApplied_Actor>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
