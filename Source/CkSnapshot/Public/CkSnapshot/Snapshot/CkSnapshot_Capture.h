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
    // Core — captures a raw entt registry into InByteWriter, stamping InOutHeader with the format/version
    // metadata and a per-fragment-type manifest (byte offsets + lengths) so Restore can manifest-drive
    // dispatch and byte-jump unknown types.
    //
    // This is the registry-level entry point: it does NOT touch any UWorld and leaves _WorldAssetPath unset.
    // Used directly by the FEcsWorld core round-trip test and by the Run_Capture(UWorld&) wrapper below.
    //
    // The caller is responsible for pumping the world to quiescence BEFORE calling this (see
    // UCk_EcsWorld_Subsystem_UE::Request_PumpToQuiescence) so the snapshot reflects a settled world.
    //
    // Returns Success. (The registry-level core has no IO-resolution failure mode — the registry is passed
    // in directly; resolution failures live in the UWorld wrapper.)
    CKSNAPSHOT_API auto
    Run_Capture_Registry(
        ck::SnapshotRegistryType& InRegistry,
        FArchive& InByteWriter,
        FCk_Snapshot_Header& InOutHeader) -> ECk_SnapshotResult;

    // Wrapper — resolves the live entt registry from InWorld's UCk_EcsWorld_Subsystem_UE, stamps the
    // world asset path into InOutHeader, then delegates to Run_Capture_Registry.
    //
    // Returns Success, or Failed_IO if the registry could not be resolved.
    CKSNAPSHOT_API auto
    Run_Capture(
        UWorld& InWorld,
        FArchive& InByteWriter,
        FCk_Snapshot_Header& InOutHeader) -> ECk_SnapshotResult;
}

// --------------------------------------------------------------------------------------------------------------------
