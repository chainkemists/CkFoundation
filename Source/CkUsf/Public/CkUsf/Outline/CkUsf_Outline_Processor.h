#pragma once

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"

namespace ck
{
    class CKUSF_API FProcessor_Usf_OutlineResolved_Clear : public TProcessor<
        FProcessor_Usf_OutlineResolved_Clear,
        TReadOnly<FFragment_Usf_OutlineResolved>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_Usf_OutlineResolved& InResolved) -> void;
    };

    class CKUSF_API FProcessor_Usf_OutlineClaims_Resolve : public TProcessor<
        FProcessor_Usf_OutlineClaims_Resolve,
        TReadWrite<FFragment_Usf_OutlineClaims>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_Usf_OutlineResolved_Clear>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  FFragment_Usf_OutlineClaims& InClaims) -> void;
    };

    class CKUSF_API FProcessor_Usf_OutlineActor_Sync : public TProcessor<
        FProcessor_Usf_OutlineActor_Sync,
        TReadOnly<FFragment_Usf_OutlineResolved>,
        TReadOnly<FFragment_OwningActor_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_Usf_OutlineClaims_Resolve>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_Usf_OutlineResolved& InResolved,
                                  const FFragment_OwningActor_Current& InOwningActor) -> void;
    };

    class CKUSF_API FProcessor_Usf_OutlineActor_Remove : public TProcessor<
        FProcessor_Usf_OutlineActor_Remove,
        TReadOnly<FFragment_Usf_OutlineApplied_Actor>,
        TExclude<FFragment_Usf_OutlineResolved>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_Usf_OutlineClaims_Resolve>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_Usf_OutlineApplied_Actor& InApplied) -> void;
    };

    class CKUSF_API FProcessor_Usf_OutlineActor_EndPlay : public TProcessor<
        FProcessor_Usf_OutlineActor_EndPlay,
        TReadOnly<FFragment_Usf_OutlineApplied_Actor>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_Usf_OutlineApplied_Actor& InApplied) -> void;
    };
}
