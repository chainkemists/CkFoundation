#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_TextShapes.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkPmg/CkPmg_ProcessorGroups.h"
#include "CkPmg_Processor_SymbolShapes.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Text_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Text_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_Text_Params>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            FTag_Pmg_DebugShape_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Pmg_DebugShape_Setup;
        using RunAfter = TDepList<FProcessor_Pmg_Pin_Setup>;
        using MarkedDirtyBy = FTag_Pmg_DebugShape_NeedsSetup;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_Text_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
