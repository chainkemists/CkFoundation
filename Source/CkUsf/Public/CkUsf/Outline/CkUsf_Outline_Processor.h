#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Actor-backed entities: apply/refresh the outline on the entity's owning actor. Per-frame over
    // outlined entities only (view = OutlineTarget + OwningActor) — cost scales with outlined count.
    class CKUSF_API FProcessor_Usf_OutlineActor_Sync : public TProcessor<
        FProcessor_Usf_OutlineActor_Sync,
        TReadOnly<FFragment_Usf_OutlineTarget>,
        TReadOnly<FFragment_OwningActor_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineTarget& InTarget,
            const FFragment_OwningActor_Current& InOwningActor) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Undo when the target fragment is gone but the actor-path outline is still applied.
    class CKUSF_API FProcessor_Usf_OutlineActor_Remove : public TProcessor<
        FProcessor_Usf_OutlineActor_Remove,
        TReadOnly<FFragment_Usf_OutlineApplied_Actor>,
        TExclude<FFragment_Usf_OutlineTarget>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineApplied_Actor& InApplied) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUSF_API FProcessor_Usf_OutlineActor_EndPlay : public TProcessor<
        FProcessor_Usf_OutlineActor_EndPlay,
        TReadOnly<FFragment_Usf_OutlineApplied_Actor>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineApplied_Actor& InApplied) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
