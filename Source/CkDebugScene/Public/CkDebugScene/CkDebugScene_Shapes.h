#pragma once

#include "CkDebugScene/CkDebugScene_Mesh.h"
#include "CkDebugScene/CkDebugScene_Target.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Filled primitives for the retained debug scene.
//
// This is the debug-scene answer to CkPmg's shape utilities, and it is deliberately NOT built the
// same way. CkPmg creates one ECS entity per shape, which needs a registry and a ticking processor
// group in the target world -- neither exists in a debugger's FPreviewScene, and entity-per-shape
// does not survive the volumes a debug scene carries (a navmesh is tens of thousands of triangles).
//
// Here every primitive is a UNIT mesh centred on the origin, built once and shared: an instance
// transform alone places, orients and sizes it, so a thousand rings cost one UStaticMesh and a
// thousand instance transforms on the existing ISM path.
//
// 2D primitives lie in the XY plane facing +Z. Rotate via the instance transform to reach another
// plane. Note they are single-sided: a filled 2D shape viewed from below is invisible unless the
// debug material is two-sided.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::debug_scene::shapes
{
// --- 3D ------------------------------------------------------------------------------------------

/** Unit cube, corners at +/-0.5. Scale by the full extent. */
CKDEBUGSCENE_API auto
Get_Box() -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Unit-radius UV sphere. InRings/InSegments are clamped to sane minimums and cached per pair. */
CKDEBUGSCENE_API auto
Get_Sphere(int32 InRings = 12, int32 InSegments = 16) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Unit-radius capped cylinder of height 1, centred on the origin, axis +Z. */
CKDEBUGSCENE_API auto
Get_Cylinder(int32 InSegments = 24) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Unit-radius cone of height 1: base at Z=-0.5, apex at Z=+0.5. */
CKDEBUGSCENE_API auto
Get_Cone(int32 InSegments = 24) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Unit-RADIUS capsule, axis +Z, centred on the origin. InSegmentRatio is the straight barrel's
 *  height as a multiple of the radius, so total height is (InSegmentRatio + 2) radii.
 *
 *  The proportion is baked into the mesh rather than left to the instance scale on purpose: the
 *  caps are hemispheres, and a non-uniform scale would stretch them into ellipsoids. Scale this
 *  mesh UNIFORMLY by the radius. */
CKDEBUGSCENE_API auto
Get_Capsule(float InSegmentRatio = 1.0f, int32 InRings = 6, int32 InSegments = 16)
    -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Torus of outer radius 1 in the XY plane. InTubeRatio is the tube radius as a fraction of 1. */
CKDEBUGSCENE_API auto
Get_Torus(float InTubeRatio = 0.25f, int32 InMajorSegments = 32, int32 InMinorSegments = 12)
    -> TSharedPtr<FCk_DebugScene_Mesh>;

// --- 2D (XY plane, facing +Z) ---------------------------------------------------------------------

/** Unit square, corners at +/-0.5. */
CKDEBUGSCENE_API auto
Get_Quad() -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Unit-radius filled disc. */
CKDEBUGSCENE_API auto
Get_Disc(int32 InSegments = 32) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Filled annulus of outer radius 1. InInnerRatio is the hole radius as a fraction of 1. */
CKDEBUGSCENE_API auto
Get_Ring(float InInnerRatio = 0.75f, int32 InSegments = 32) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Equilateral triangle inscribed in the unit circle, pointing +X. */
CKDEBUGSCENE_API auto
Get_Triangle() -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Plus-shaped cross spanning +/-0.5. InThicknessRatio is the arm width as a fraction of 1. */
CKDEBUGSCENE_API auto
Get_Cross(float InThicknessRatio = 0.25f) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Flat arrow of length 1 pointing +X, centred on the origin. */
CKDEBUGSCENE_API auto
Get_Arrow(float InShaftRatio = 0.35f, float InHeadRatio = 0.4f) -> TSharedPtr<FCk_DebugScene_Mesh>;

/** Drops every cached mesh. Call on shutdown; the UStaticMesh roots are released with them. */
CKDEBUGSCENE_API auto
Clear_Cache() -> void;
} // namespace ck::debug_scene::shapes
