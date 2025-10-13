#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkHFSM/Service/CkService_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKHFSM_API FProcessor_Service_Setup
        : public ck_exp::TProcessor<FProcessor_Service_Setup, FCk_Handle_Service,
            FFragment_Service_Current, FTag_Service_Setup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Service_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Service_Enter
        : public ck_exp::TProcessor<FProcessor_Service_Enter, FCk_Handle_Service,
            FFragment_Service_Current, FTag_Service_Enter, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Service_Enter;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Service_Exit
        : public ck_exp::TProcessor<FProcessor_Service_Exit, FCk_Handle_Service,
            FFragment_Service_Current, FTag_Service_Exit, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Service_Exit;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Service_Update
        : public ck_exp::TProcessor<FProcessor_Service_Update, FCk_Handle_Service,
            FFragment_Service_Current, FTag_Service_Update, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Service_HandleRequests
        : public ck_exp::TProcessor<FProcessor_Service_HandleRequests, FCk_Handle_Service,
            FFragment_Service_Current, FFragment_Service_Requests, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Service_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent,
            const FFragment_Service_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Service_Current& InCurrent,
            const FCk_Request_Service_Command& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------