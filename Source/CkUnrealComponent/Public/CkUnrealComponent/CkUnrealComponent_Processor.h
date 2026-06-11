#pragma once

#include "CkUnrealComponent_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_Setup : public ck_exp::TProcessor<
            FProcessor_UnrealComponent_Setup,
            FCk_Handle_UnrealComponent,
            ck::TReadOnly<FFragment_UnrealComponent_Params>,
            ck::TReadWrite<FFragment_UnrealComponent_Current>,
            FTag_UnrealComponent_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_UnrealComponent_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_UnrealComponent_Params& InParams,
            FFragment_UnrealComponent_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_PushTransform : public ck_exp::TProcessor<
            FProcessor_UnrealComponent_PushTransform,
            FCk_Handle_UnrealComponent,
            ck::TReadOnly<FFragment_UnrealComponent_Current>,
            FTag_UnrealComponent_IsScene,
            TExclude<FTag_UnrealComponent_NeedsSetup>,
            TExclude<FTag_UnrealComponent_TransformPushDisabled>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using RunAfter = TDepList<FProcessor_UnrealComponent_Setup>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_UnrealComponent_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_Tick : public ck_exp::TProcessor<
            FProcessor_UnrealComponent_Tick,
            FCk_Handle_UnrealComponent,
            ck::TReadOnly<FFragment_UnrealComponent_Current>,
            FTag_UnrealComponent_TickViaProcessor,
            TExclude<FTag_UnrealComponent_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using RunAfter = TDepList<FProcessor_UnrealComponent_PushTransform>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_UnrealComponent_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_EndPlay : public ck_exp::TProcessor<
            FProcessor_UnrealComponent_EndPlay,
            FCk_Handle_UnrealComponent,
            ck::TReadWrite<FFragment_UnrealComponent_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_UnrealComponent_Setup>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_UnrealComponent_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
