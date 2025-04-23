#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FProcessor_EntityScript_SpawnEntity_HandleRequests : public TProcessor<
            FProcessor_EntityScript_SpawnEntity_HandleRequests,
            FFragment_EntityScript_RequestSpawnEntity,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityScript_RequestSpawnEntity;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_RequestSpawnEntity& InRequestFragment) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityScript_SpawnEntity& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_BeginPlay : public TProcessor<
            FProcessor_EntityScript_BeginPlay,
            FFragment_EntityScript_Current,
            FTag_EntityScript_BeginPlay,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_EntityScript_BeginPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_EndPlay : public TProcessor<
            FProcessor_EntityScript_EndPlay,
            FFragment_EntityScript_Current,
            FTag_EntityScript_HasBegunPlay,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_EntityScript_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
