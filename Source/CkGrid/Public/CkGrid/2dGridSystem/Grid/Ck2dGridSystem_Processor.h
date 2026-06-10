#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // RESTORE RECOMPOSE — Server-side, post-snapshot-load. A grid's Params round-trip (Tier-A), but
    // its LIVE half never can: FFragment_2dGridSystem_Current owns a PRIVATE nested cell registry
    // (TUniquePtr<FEcsWorld>) plus the pivot SceneNode. This processor rebuilds that half from the
    // restored Params for every restored grid-bearing entity — standalone grids, Spatial-inventory
    // grids, and item Dimensions-trait shape grids alike. The view iterates the clean Params fragment
    // and POINT-QUERIES the shared restored marker; "already composed" (Has Current) is the done
    // condition, so no per-feature done tag is needed (no requests are enqueued, composition is
    // immediate and idempotent).
    // ================================================================================================================

    class CKGRID_API FProcessor_2dGridSystem_RestoreRecompose : public ck_exp::TProcessor<
            FProcessor_2dGridSystem_RestoreRecompose,
            FCk_Handle,
            TReadOnly<FFragment_2dGridSystem_Params>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridSystem_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKGRID_API FProcessor_2dGridSystem_DebugDrawAll : public ck_exp::TProcessor<
            FProcessor_2dGridSystem_DebugDrawAll,
            FCk_Handle_2dGridSystem,
            TReadOnly<ck::FFragment_2dGridSystem_Params>,
            TReadOnly<ck::FFragment_2dGridSystem_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridSystem_Params& InParams,
            const FFragment_2dGridSystem_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------