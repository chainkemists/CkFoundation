#pragma once

#include "CkActorInfo_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

namespace ck
{
    class CKACTOR_API FCk_Processor_ActorInfo_LinkUp
        : public TProcessor<FCk_Processor_ActorInfo_LinkUp, FTag_ActorInfo_LinkUp>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(FTimeType InDeltaT,
            FHandleType InHandle,
            FTag_ActorInfo_LinkUp& InTag);
    };
}
