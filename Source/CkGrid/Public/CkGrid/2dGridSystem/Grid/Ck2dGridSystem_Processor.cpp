#include "Ck2dGridSystem_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/Settings/CkGrid_Settings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridSystem_DebugDrawAll);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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