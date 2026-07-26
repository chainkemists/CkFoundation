#pragma once

#include "CkPmg_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    // World-space endpoints in, stored entity-local; adds FFragment_Pmg_DebugShape_Lines on first call.
    CKPMG_API auto
    Append_DebugLine_World(
        FCk_Handle InHandle,
        const FVector& InWorldStart,
        const FVector& InWorldEnd,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    CKPMG_API auto
    Append_DebugBox_World(
        FCk_Handle InHandle,
        const FVector& InCenter,
        const FVector& InExtent,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    CKPMG_API auto
    Append_DebugCapsule_World(
        FCk_Handle InHandle,
        const FVector& InCenter,
        float InHalfHeight,
        float InRadius,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    CKPMG_API auto
    Append_DebugCircle_PlaneAxis_World(
        FCk_Handle InHandle,
        const FVector& InCenter,
        float InRadius,
        ECk_Plane_Axis InAxis,
        int32 InSegments,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    // Outline only — in PMG the fill is the procedural mesh section, not these lines.
    CKPMG_API auto
    Append_DebugTriangle_World(
        FCk_Handle InHandle,
        const FVector& InV1,
        const FVector& InV2,
        const FVector& InV3,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    // Closed loop, outline only — same caveat as Append_DebugTriangle_World.
    CKPMG_API auto
    Append_DebugPolygon_World(
        FCk_Handle InHandle,
        const TArray<FVector>& InVertices,
        const FLinearColor& InColor,
        float InThickness)
        -> void;
}
