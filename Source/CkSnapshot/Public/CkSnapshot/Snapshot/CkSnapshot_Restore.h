#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"

class UWorld;
class FArchive;
struct FCk_Snapshot_Header;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Restores a snapshot byte stream (InByteReader) into the live ECS world resolved from InWorld.
    // Manifest-driven: each fragment-type entry in InHeader is dispatched to its registered loader by
    // entt type hash; unknown types are byte-jumped (Seek past offset+length) and recorded in the report's
    // _SkippedFragmentTypes. The entities pass (written before the manifest by Capture) is restored first.
    //
    // Populates and returns a FCk_Snapshot_LoadReport (result + counts + skipped lists + loaded header).
    CKSNAPSHOT_API auto
    Run_Restore(
        UWorld& InWorld,
        FArchive& InByteReader,
        const FCk_Snapshot_Header& InHeader) -> FCk_Snapshot_LoadReport;
}

// --------------------------------------------------------------------------------------------------------------------
