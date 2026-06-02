#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"

class UWorld;
class FArchive;
struct FCk_Snapshot_Header;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Captures the live ECS world (resolved from InWorld's UCk_EcsWorld_Subsystem_UE) into InByteWriter,
    // stamping InOutHeader with the format/version metadata and a per-fragment-type manifest (byte offsets
    // + lengths) so Restore can manifest-drive dispatch and byte-jump unknown types.
    //
    // The caller is responsible for pumping the world to quiescence BEFORE calling this (see
    // UCk_EcsWorld_Subsystem_UE::Request_PumpToQuiescence) so the snapshot reflects a settled world.
    //
    // Returns Success, or Failed_IO if the registry could not be resolved.
    CKSNAPSHOT_API auto
    Run_Capture(
        UWorld& InWorld,
        FArchive& InByteWriter,
        FCk_Snapshot_Header& InOutHeader) -> ECk_SnapshotResult;
}

// --------------------------------------------------------------------------------------------------------------------
