#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/Context/CkSnapshot_Context.h"
#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"

class UWorld;
class FArchive;
struct FCk_Snapshot_Header;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Core — restores a snapshot byte stream (InByteReader) into a raw entt registry with WIPE-THEN-RESTORE
    // semantics: the registry is fully cleared first, so the result contains EXACTLY the saved entities (no
    // merge duplicates, no pre-existing live entities).
    //
    // Manifest-driven: each fragment-type entry in InHeader is dispatched to its registered loader by entt
    // type hash; unknown types are byte-jumped (Seek past offset+length) and recorded in the report's
    // _SkippedFragmentTypes. The entities pass (written before the manifest by Capture) is restored first.
    //
    // This is the registry-level entry point: it does NOT touch any UWorld. Used directly by the FEcsWorld
    // core round-trip test and by the Run_Restore(UWorld&) wrapper below.
    //
    // Populates and returns a FCk_Snapshot_LoadReport (result + counts + skipped lists + loaded header).
    CKSNAPSHOT_API auto
    Run_Restore_Registry(
        ck::SnapshotRegistryType& InRegistry,
        FArchive& InByteReader,
        const FCk_Snapshot_Header& InHeader) -> FCk_Snapshot_LoadReport;

    // Wrapper — resolves the live entt registry from InWorld's UCk_EcsWorld_Subsystem_UE, then delegates to
    // Run_Restore_Registry.
    //
    // Populates and returns a FCk_Snapshot_LoadReport (result + counts + skipped lists + loaded header).
    CKSNAPSHOT_API auto
    Run_Restore(
        UWorld& InWorld,
        FArchive& InByteReader,
        const FCk_Snapshot_Header& InHeader) -> FCk_Snapshot_LoadReport;
}

// --------------------------------------------------------------------------------------------------------------------
