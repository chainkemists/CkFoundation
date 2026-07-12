#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::SnapshotRegistryType
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h" // ECk_SnapshotResult

class UWorld;
class FArchive;
struct FCk_Snapshot_HeaderV3;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Format-v3 capture (spec §4.2 rebuild+hydrate) — written ALONGSIDE the Model-A image until Phase 5. Instead of
    // a raw fragment image, v3 writes a per-entity recipe/identity table (provenance EngineOwned / ConstructSpawned /
    // RuntimeSpawned) + minimal hydration payloads (every Save-flagged handler's Produce, one per persisted entity).
    //
    // Registry-level core: classifies + serializes into InByteWriter and stamps InOutHeader's census. InWorldOrNull
    // enables the EngineOwned player-pawn rendezvous (rule 2) — pass null on the bare-registry path (SaveKey-only).
    // Handle refs inside recipe params / payloads route through FSnapshotContext::Snapshot_Handle (ck::snapshot::
    // RemapHandles); a params handle referencing a non-persisted entity fires a loud ensure (unsupported on v3).
    CKSNAPSHOT_API auto
    Run_CaptureV3_Registry(
        ck::SnapshotRegistryType& InRegistry,
        FCk_RegistryHandle InRegistryHandle,
        UWorld* InWorldOrNull,
        FArchive& InByteWriter,
        FCk_Snapshot_HeaderV3& InOutHeader) -> ECk_SnapshotResult;

    // Wrapper — resolves the live entt registry from InWorld's UCk_EcsWorld_Subsystem_UE, stamps the world asset
    // path, then delegates to Run_CaptureV3_Registry (with the world, so player-pawn rendezvous is available).
    CKSNAPSHOT_API auto
    Run_CaptureV3(
        UWorld& InWorld,
        FArchive& InByteWriter,
        FCk_Snapshot_HeaderV3& InOutHeader) -> ECk_SnapshotResult;
}

// --------------------------------------------------------------------------------------------------------------------
