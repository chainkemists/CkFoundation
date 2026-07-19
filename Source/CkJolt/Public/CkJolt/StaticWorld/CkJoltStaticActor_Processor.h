#pragma once

#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment_Data.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_JoltStaticWorld_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Frees a source actor's baked static-world bodies when its attribution entity dies — the ENTITY-first arm
    // of the bidirectional lifecycle. If a JoltStaticActor entity is destroyed before the subsystem's
    // level-streaming / Request_RemoveActor path removes it, this routes through the SAME subsystem funnel
    // (Request_RemoveBodiesForEntity), which is idempotent: it empties the fragment's body-id array, so a
    // later subsystem-driven pass over the same handle is a no-op (and vice-versa). A missing subsystem
    // (world teardown) is a correct silent skip.
    class CKJOLT_API FProcessor_JoltStaticActor_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltStaticActor_EndPlay,
            FCk_Handle_JoltStaticActor,
            ck::TReadWrite<FFragment_JoltStaticActor_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // Mirrors FProcessor_JoltBody_EndPlay: non-runtime worlds never have a Jolt subsystem, so running there
        // would touch state that was never created.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltStaticActor_Current& InCurrent) const -> void;

    private:
        TWeakObjectPtr<UCk_JoltStaticWorld_Subsystem_UE> _StaticWorldSubsystem;
    };
}

// --------------------------------------------------------------------------------------------------------------------
