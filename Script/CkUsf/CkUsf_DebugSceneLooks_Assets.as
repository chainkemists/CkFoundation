// Language=angelscript

//============================================================================
// DEBUG SCENE LOOKS - the two materials CkDebugScene draws everything with
//============================================================================
//
// Replacements for the hand-authored /CkFoundation/DebugScene/M_CkDebugScene_Opaque and
// M_CkDebugScene_Translucent. Both are consumed by CkDebugScene_Materials and reach the Crowd
// debugger's 3d viewport, the AI Overview's spatial pane and the Jolt debug renderer.
//
// WHY THESE EXIST AS LOOKS
//
// The hand-authored pair are single-sided, and nothing about them says so where anyone can read
// it: UE only serializes non-default material properties, so `Two Sided` being off is the ABSENCE
// of a token in a binary asset. That cost real performance. Because the materials cull backfaces,
// the Crowd debugger's scene adapter fakes two-sidedness in geometry - AppendTwoSidedTriangle
// emits every navmesh triangle twice with reversed winding - which doubles the vertex count, the
// mesh-description build and the tangent pass on every navmesh rebuild. Measured at 194ms for one
// rebuild on BusterBlock's main map, roughly half of it attributable to the duplication.
//
// As look definitions the flag is one line of reviewable text, and the duplication can be deleted.
//
// OPACITY
//
// CkUsf passes vector parameters into a look as `.rgb` (CkUsf_Generator, ECk_Usf_ParamType::Vector),
// so a vector parameter's alpha never reaches the shader. The hand-authored materials carried
// opacity in the alpha of their `Color` parameter. These looks therefore expose `Opacity` as its
// own scalar, and CkDebugScene must set it alongside `Color` when it adopts them - see the
// adoption notes at the bottom of this file. Without that change the translucent navmesh overlay
// renders fully opaque.
//============================================================================

namespace CkUsf
{
    // Shared by both variants: a flat colour and its opacity. Defaults are the values
    // CkDebugScene_Target hands the MID before it overrides them, so a look that is generated but
    // never parameterized still renders as a visible mid-grey rather than as nothing.
    TArray<FCk_Usf_ParamDesc> Usf_DebugSceneParams(float InDefaultOpacity)
    {
        auto Params = TArray<FCk_Usf_ParamDesc>();

        Params.Add(CkUsf::Usf_Vector(n"Color", FLinearColor(0.5, 0.5, 0.5, 1.0)));
        Params.Add(CkUsf::Usf_Scalar(n"Opacity", InDefaultOpacity));

        return Params;
    }

    // Opaque fill - agent capsules, queue origin/reservation boxes, and every Jolt collision shape
    // drawn in solid mode.
    asset DebugSceneOpaque of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DebugScene.ush";
        _UshFunctionName = n"CkUsf_Look_DebugScene";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Opaque;
        _LookName        = n"DebugSceneOpaque";

        // The entire point of the migration: debug geometry is inspected from every angle,
        // including from under the navmesh, so backface culling is never wanted here.
        _TwoSided        = true;

        // CkDebugScene renders exclusively through UInstancedStaticMeshComponent buckets
        // (CkDebugScene_Target's CkDebugSceneIsm components). Without this the material is
        // unusable on them.
        _UsedWithInstancedStaticMeshes = true;

        _Parameters = CkUsf::Usf_DebugSceneParams(1.0);
    }

    // Translucent fill - the navmesh overlay, path-network ribbons, and the queue reservation
    // boxes, all of which are drawn over world geometry and must not hide it.
    asset DebugSceneTranslucent of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/DebugScene.ush";
        _UshFunctionName = n"CkUsf_Look_DebugScene";
        _Domain          = ECk_Usf_Domain::SurfaceUnlit;
        _BlendMode       = ECk_Usf_BlendMode::Translucent;
        _LookName        = n"DebugSceneTranslucent";
        _TwoSided        = true;
        _UsedWithInstancedStaticMeshes = true;

        // 0.15 is the navmesh overlay's alpha in CkCrowdDebugger_3dSceneAdapter, i.e. the most
        // common translucent consumer's value.
        _Parameters = CkUsf::Usf_DebugSceneParams(0.15);
    }
}

//============================================================================
// ADOPTION - what still has to happen for these to replace the hand-authored pair
//============================================================================
//
// These definitions are inert on their own. Generating them changes nothing until the consumer is
// repointed, which is deliberate: it keeps the switchover one reviewable, testable step.
//
//  1. Run the CkUsf generator so the masters are emitted.
//  2. Repoint CkDebugScene_Materials.cpp off
//     "/CkFoundation/DebugScene/M_CkDebugScene_Opaque.M_CkDebugScene_Opaque" and its translucent
//     twin, onto the generated look paths (ck::usf::Get_GeneratedMasterPackagePath).
//  3. Set the Opacity scalar in CkDebugScene_Target.cpp everywhere ColorParameter is set today
//     (two call sites: the wire MID and the base MID), sourcing it from the appearance colour's
//     alpha, which is where that value lives now.
//  4. Delete AppendTwoSidedTriangle's duplicate emit in CkCrowdDebugger_3dSceneAdapter - the
//     reason for the migration. Ck.Jolt.DebugDraw.SingleTriangleBuild already asserts that one
//     source triangle produces exactly one rendered triangle, so it covers this directly.
//  5. Re-run Ck.Jolt.DebugDraw.* and Ck.CrowdDebugger.Viewport3d.*, and look at the navmesh
//     overlay and Jolt collision draw in a real session. Neither suite renders pixels, so
//     "translucency still reads correctly" is not something they can tell you.
//  6. Retire the two hand-authored assets.
//============================================================================
