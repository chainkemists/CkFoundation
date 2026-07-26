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
    class CKJOLT_API FProcessor_JoltStaticActor_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltStaticActor_EndPlay,
            FCk_Handle_JoltStaticActor,
            ck::TReadWrite<FFragment_JoltStaticActor_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // Mirrors FProcessor_JoltBody_EndPlay: non-runtime worlds never have a Jolt subsystem.
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
