#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::SnapshotRegistryType
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h" // ECk_SnapshotResult

class UWorld;
class FArchive;
struct FCk_Snapshot_HeaderV3;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Audit is a subset of Classify, not a sibling.
    struct CKSNAPSHOT_API FCaptureTimings
    {
        FCk_Time Classify;
        FCk_Time Audit;
        FCk_Time Payloads;
        FCk_Time Tables;
        int32    AuditProbeCount   = 0;
        int64    PayloadByteTotal  = 0;
        int32    DistinctTypePaths = 0;
    };

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
        FCk_Snapshot_HeaderV3& InOutHeader,
        FCaptureTimings* OutTimings = nullptr) -> ECk_SnapshotResult;

    CKSNAPSHOT_API auto
    Run_CaptureV3(
        UWorld& InWorld,
        FArchive& InByteWriter,
        FCk_Snapshot_HeaderV3& InOutHeader,
        FCaptureTimings* OutTimings = nullptr) -> ECk_SnapshotResult;
}

// --------------------------------------------------------------------------------------------------------------------
