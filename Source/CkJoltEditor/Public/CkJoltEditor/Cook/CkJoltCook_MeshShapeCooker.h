#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

/// Per-mesh-asset shape cooker: sweeps every UStaticMesh under the configured _BakedMeshShapeRoots
/// and writes one UCk_Jolt_CookedMeshShape_UE (scale-1 SaveWithChildren blob) per mesh whose
/// collision is worth pre-baking (contains a convex hull or cooked tri-mesh — pure-primitive
/// collision is cheaper to rebuild at runtime than to load). Incremental: an existing asset whose
/// BodySetupGuid, trace flag, and versions all match is skipped. Orphaned shape assets (source
/// mesh gone or no longer worth baking) are LOGGED, not auto-deleted — same v1 policy as the map
/// cook. Consumed by UCk_JoltCook_EditorSubsystem_UE and UCk_JoltCook_Commandlet.
class CKJOLTEDITOR_API FCk_Jolt_MeshShapeCooker
{
public:
    CK_GENERATED_BODY(FCk_Jolt_MeshShapeCooker);

    struct FCookStats
    {
        int32 _NumMeshesConsidered = 0;
        int32 _NumShapesCooked = 0;
        int32 _NumUpToDate = 0;
        int32 _NumSkippedNotWorthBaking = 0;
        int32 _NumFailed = 0;
        int32 _NumOrphans = 0;
        bool _Success = false;
    };

public:
    static auto
    Cook_MeshShapes(
        bool InDryRun = false) -> FCookStats;
};

// --------------------------------------------------------------------------------------------------------------------
