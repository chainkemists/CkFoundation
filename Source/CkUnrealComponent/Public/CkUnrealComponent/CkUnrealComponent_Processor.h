#pragma once

#include "CkUnrealComponent_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

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

    // Change detection is the owner's fragment vs FFragment_UnrealComponent_LastPushedTransform (its
    // own consumed state) — deliberately NOT FTag_Transform_Updated, whose lifetime misses pump-drained
    // one-shot moves, and NOT the live component, whose external drift must survive idle owners
    // (TransformPropagation.DirtyOwnersOnly). Rationale on the fragment's declaration.
    class CKUNREALCOMPONENT_API FProcessor_UnrealComponent_PushTransform : public ck_exp::TProcessor<
            FProcessor_UnrealComponent_PushTransform,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_RecordOfUnrealComponents>,
            ck::TReadWrite<FFragment_UnrealComponent_LastPushedTransform>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_UnrealComponent_Setup>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_RecordOfUnrealComponents& InComponents,
            FFragment_UnrealComponent_LastPushedTransform& InLastPushed) -> void;
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
        using Group = FGroup_PostTransform;
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
