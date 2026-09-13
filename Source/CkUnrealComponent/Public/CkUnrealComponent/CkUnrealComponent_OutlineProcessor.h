#pragma once

#include "CkUnrealComponent/CkUnrealComponent_Fragment.h"
#include "CkUnrealComponent/CkUnrealComponent_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Outline/CkUsf_Outline_Processor.h"

namespace ck
{
    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_Outline_Sync : public ck_exp::TProcessor<
        FProcessor_UnrealComponent_Outline_Sync,
        FCk_Handle_UnrealComponent,
        TReadOnly<FFragment_UnrealComponent_Current>,
        TReadOnly<FFragment_Usf_OutlineResolved>,
        TExclude<FTag_UnrealComponent_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_UnrealComponent_Setup, FProcessor_Usf_OutlineClaims_Resolve>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_UnrealComponent_Current& InCurrent,
                                  const FFragment_Usf_OutlineResolved& InResolved) -> void;
    };

    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_Outline_Remove : public ck_exp::TProcessor<
        FProcessor_UnrealComponent_Outline_Remove,
        FCk_Handle_UnrealComponent,
        TReadOnly<FFragment_Usf_OutlineApplied_Component>,
        TExclude<FFragment_Usf_OutlineResolved>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_Usf_OutlineClaims_Resolve>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_Usf_OutlineApplied_Component& InApplied) -> void;
    };

    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_Outline_EndPlay : public ck_exp::TProcessor<
        FProcessor_UnrealComponent_Outline_EndPlay,
        FCk_Handle_UnrealComponent,
        TReadOnly<FFragment_Usf_OutlineApplied_Component>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
                                  const FFragment_Usf_OutlineApplied_Component& InApplied) -> void;
    };
}
