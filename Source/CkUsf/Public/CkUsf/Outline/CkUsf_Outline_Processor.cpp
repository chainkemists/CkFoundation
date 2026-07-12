#include "CkUsf_Outline_Processor.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineActor_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineActor_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_Usf_OutlineActor_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_outline_processor
{
    auto
        DoRemoveOutlineFromActor(
            const ck::FFragment_Usf_OutlineApplied_Actor& InApplied)
        -> void
    {
        auto* Actor = InApplied.Get_Actor().Get();
        if (ck::Is_NOT_Valid(Actor))
        { return; } // actor already gone — the subsystem reaps its dead components + stencil refcounts

        auto* Subsystem = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(Actor);
        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        Subsystem->Remove_Outline_From_Actor(Actor);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Usf_OutlineActor_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineTarget& InTarget,
            const FFragment_OwningActor_Current& InOwningActor)
        -> void
    {
        auto* Actor = InOwningActor.Get_EntityOwningActor().Get();
        auto* Preset = InTarget.Get_Preset().Get();

        if (ck::Is_NOT_Valid(Actor) || ck::Is_NOT_Valid(Preset, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        if (InHandle.Has<FFragment_Usf_OutlineApplied_Actor>())
        {
            const auto& Applied = InHandle.Get<FFragment_Usf_OutlineApplied_Actor>();

            if (Applied.Get_Preset() == InTarget.Get_Preset() && Applied.Get_Actor() == Actor)
            { return; }

            ck_usf_outline_processor::DoRemoveOutlineFromActor(Applied);
        }

        auto* Subsystem = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(Actor);
        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        Subsystem->Apply_Outline_To_Actor(Actor, Preset);
        InHandle.AddOrGet<FFragment_Usf_OutlineApplied_Actor>() =
            FFragment_Usf_OutlineApplied_Actor{Preset, Actor};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_OutlineActor_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineApplied_Actor& InApplied)
        -> void
    {
        ck_usf_outline_processor::DoRemoveOutlineFromActor(InApplied);
        InHandle.Remove<FFragment_Usf_OutlineApplied_Actor>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Usf_OutlineActor_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineApplied_Actor& InApplied)
        -> void
    {
        ck_usf_outline_processor::DoRemoveOutlineFromActor(InApplied);
        InHandle.Remove<FFragment_Usf_OutlineApplied_Actor>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
