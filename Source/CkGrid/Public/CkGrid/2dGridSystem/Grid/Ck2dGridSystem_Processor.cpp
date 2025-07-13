#include "Ck2dGridSystem_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/Settings/CkGrid_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_2dGridSystem_DebugDrawAll::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
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
        const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        UCk_Utils_2dGridSystem_UE::DebugDraw_Grid_Simple(
            WorldContext,
            InHandle,
            0.0f
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------