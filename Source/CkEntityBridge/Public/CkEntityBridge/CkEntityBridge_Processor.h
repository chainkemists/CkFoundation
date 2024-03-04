#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEntityBridge/CkEntityBridge_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYBRIDGE_API FProcessor_EntityBridge_HandleRequests
        : public TProcessor<FProcessor_EntityBridge_HandleRequests, FFragment_EntityBridge_Requests>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityBridge_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityBridge_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityBridge_SpawnEntity& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
