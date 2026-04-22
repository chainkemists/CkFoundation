#pragma once

#include "CkPmg_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    // Append a wireframe line, given two world-space endpoints. The entity's
    // current FFragment_Transform is used to inverse-transform the points
    // into entity-local space, so per-tick redraw can re-project them under
    // the live transform. Adds FFragment_Pmg_DebugShape_Lines on first call.
    //
    // Drop-in replacement for UCk_Utils_DebugDraw_UE::DrawDebugLine inside
    // PMG Setup processors — Setup processors keep computing endpoints in
    // world space the same way they always have.
    CKPMG_API auto
    Append_DebugLine_World(
        FCk_Handle InHandle,
        const FVector& InWorldStart,
        const FVector& InWorldEnd,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    // Append the 12 edges of an oriented box. Replacement for
    // UCk_Utils_DebugDraw_UE::DrawDebugBox.
    CKPMG_API auto
    Append_DebugBox_World(
        FCk_Handle InHandle,
        const FVector& InCenter,
        const FVector& InExtent,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    // Decompose a capsule into 2 longitudinal side lines + 4 cross arcs (one
    // ring at each end + 2 vertical half-rings tying the ends). Replacement
    // for UCk_Utils_DebugDraw_UE::DrawDebugCapsule.
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

    // Decompose a planar circle into N segment edges. Replacement for
    // UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis.
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

    // 3 perimeter edges. Replacement for the wireframe contour of
    // UCk_Utils_DebugDraw_UE::DrawDebugTriangle (note: the cached form is
    // the triangle outline only — the original draws a filled triangle as
    // well, but in PMG that fill is already provided by the procedural mesh
    // section, so the debug-draw call only contributed wireframe).
    CKPMG_API auto
    Append_DebugTriangle_World(
        FCk_Handle InHandle,
        const FVector& InV1,
        const FVector& InV2,
        const FVector& InV3,
        const FLinearColor& InColor,
        float InThickness)
        -> void;

    // N perimeter edges (closed loop). Same wireframe-only-contour caveat
    // as Append_DebugTriangle_World. Replacement for
    // UCk_Utils_DebugDraw_UE::DrawDebugPolygon.
    CKPMG_API auto
    Append_DebugPolygon_World(
        FCk_Handle InHandle,
        const TArray<FVector>& InVertices,
        const FLinearColor& InColor,
        float InThickness)
        -> void;
}
