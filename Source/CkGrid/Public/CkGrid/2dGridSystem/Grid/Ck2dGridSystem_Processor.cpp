#include "Ck2dGridSystem_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/CkGrid_Log.h"
#include "CkGrid/Settings/CkGrid_Settings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridSystem_RestoreRecompose);
CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridSystem_DebugDrawAll);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_2dGridSystem_RestoreRecompose::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_2dGridSystem_Params& InParams) const
        -> void
    {
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        if (InHandle.Has<FFragment_2dGridSystem_Current>())
        { return; }

        // The composing entity always had a Transform pre-save (grid Add requires it) and Transform
        // is snapshotted — but stay defensive: a skip-if-missing restore may have dropped it.
        auto MutableHandle = InHandle;
        auto TransformHandle = UCk_Utils_Transform_UE::Has(MutableHandle)
            ? UCk_Utils_Transform_UE::CastChecked(MutableHandle)
            : UCk_Utils_Transform_UE::Add(MutableHandle, FTransform::Identity);

        UCk_Utils_2dGridSystem_UE::Request_RecomposeFromSnapshot(TransformHandle);

        ck::grid::Display(TEXT("[RESTORE_DEBUG] RestoreRecompose [{}]: re-composed [{}x{}] grid from restored params"),
            InHandle, InParams.Get_Dimensions().X, InParams.Get_Dimensions().Y);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_2dGridSystem_DebugDrawAll::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        // Gated purely on the cvar (editor AND packaged). The editor's selected-spawner PREVIEW is now
        // drawn by the CkGridEditor component visualizer (the authored-state PDI overlay, identical to
        // the in-paint-mode view) rather than this runtime OBB DebugDraw path — so merely selecting a
        // spawner no longer needs this processor to tick. The cvar (ck.Grid.DebugPreviewAllGrids) keeps
        // the OBB view available as an explicit debug opt-in for ALL live grids.
        if (NOT UCk_Utils_Grid_Settings::Get_DebugPreviewAllGrids())
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_2dGridSystem_DebugDrawAll::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridSystem_Params& InParams,
            const FFragment_2dGridSystem_Current& InCurrent)
            -> void
    {
        // DoTick already gates on the cvar; reaching here means the preview-all-grids opt-in is on.
        const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        UCk_Utils_2dGridSystem_UE::DebugDraw_Grid_Simple(
            WorldContext,
            InHandle,
            0.0f
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------