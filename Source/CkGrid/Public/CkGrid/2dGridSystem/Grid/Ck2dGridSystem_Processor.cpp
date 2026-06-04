#include "Ck2dGridSystem_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/Settings/CkGrid_Settings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#if WITH_EDITOR
#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "Editor.h"
#include "Selection.h"
#endif

CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridSystem_DebugDrawAll);

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck_2dgridsystem_processor
{
    // The editor preview entity is spawned under the editor subsystem's transient entity (NOT under
    // the spawner actor), so it has no OwningActor link back to ACk_EntitySpawner_UE. Correlate the
    // other way: walk the actors selected in the editor and ask each spawner whether the grid entity
    // we're drawing is its current preview entity.
    auto
        Is_GridEntitySelectedInEditor(
            const FCk_Handle& InGridEntity)
            -> bool
    {
        if (ck::Is_NOT_Valid(InGridEntity))
        { return false; }

        if (ck::Is_NOT_Valid(GEditor))
        { return false; }

        auto* SelectedActors = GEditor->GetSelectedActors();
        if (ck::Is_NOT_Valid(SelectedActors))
        { return false; }

        for (auto Index = 0; Index < SelectedActors->Num(); ++Index)
        {
            const auto* Spawner = Cast<ACk_EntitySpawner_UE>(SelectedActors->GetSelectedObject(Index));
            if (ck::Is_NOT_Valid(Spawner))
            { continue; }

            if (Spawner->Get_EditorEntityHandle() == InGridEntity)
            { return true; }
        }

        return false;
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_2dGridSystem_DebugDrawAll::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
#if WITH_EDITOR
        // In the editor, always tick so ForEachEntity can evaluate per-grid selection — even when
        // the global preview cvar is off. ForEachEntity gates each grid individually.
        TProcessor::DoTick(InDeltaT);
#else
        // Outside the editor (packaged/runtime), behavior is cvar-only.
        if (NOT UCk_Utils_Grid_Settings::Get_DebugPreviewAllGrids())
        { return; }

        TProcessor::DoTick(InDeltaT);
#endif
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
        auto bDraw = UCk_Utils_Grid_Settings::Get_DebugPreviewAllGrids();

#if WITH_EDITOR
        if (NOT bDraw)
        {
            bDraw = ck_2dgridsystem_processor::Is_GridEntitySelectedInEditor(InHandle);
        }
#endif

        if (NOT bDraw)
        { return; }

        const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        UCk_Utils_2dGridSystem_UE::DebugDraw_Grid_Simple(
            WorldContext,
            InHandle,
            0.0f
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------