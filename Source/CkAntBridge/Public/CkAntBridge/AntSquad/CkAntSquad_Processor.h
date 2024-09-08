#pragma once

#include "CkAntSquad_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

class UAntSubsystem;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANTBRIDGE_API FProcessor_AntSquad_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AntSquad_HandleRequests,
            FCk_Handle_AntSquad,
            FFragment_AntSquad_Current,
            FFragment_AntSquad_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AntSquad_Requests;

    public:
        FProcessor_AntSquad_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld);


    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AntSquad_Current& InCurrent,
            FFragment_AntSquad_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_AntSquad_Current& InCurrent,
            const FCk_Request_AntSquad_AddAgent& InRequestAgent) const -> void;

    public:
        TWeakObjectPtr<UAntSubsystem> _AntSubsystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANTBRIDGE_API FProcessor_AntSquad_UpdateInstances
    {
    public:
        using RegistryType = FCk_Registry;
        using TimeType = FCk_Time;

    public:
        FProcessor_AntSquad_UpdateInstances(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld);

    public:
        auto
        Tick(
            TimeType InDeltaT) -> void;

    private:
        RegistryType _Registry;
        TWeakObjectPtr<UAntSubsystem> _AntSubsystem;
    };
}

// --------------------------------------------------------------------------------------------------------------------
