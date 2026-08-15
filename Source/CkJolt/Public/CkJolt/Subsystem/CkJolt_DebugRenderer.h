#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget_Impl.h"

#include <Templates/PimplPtr.h>

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include <Jolt/Renderer/DebugRenderer.h>

// --------------------------------------------------------------------------------------------------------------------

class UWorld;
struct FCk_Handle;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
}

// --------------------------------------------------------------------------------------------------------------------

// Batched JPH::DebugRenderer: geometry becomes transient UStaticMeshes instanced per (geometry, colour-class)
// bucket; line/text primitives stay immediate-mode. Jolt allows exactly ONE DebugRenderer instance per process
// (JPH_ASSERT in DebugRenderer's constructor), so this owns only the world-agnostic geometry/batch cache and
// reconciles into whichever FCk_Jolt_DebugDrawTarget is active for the current draw session. Game-thread only.
// Rationale: CkJolt/CLAUDE.md § "Debug draw + stats".
class CKJOLT_API FCk_Jolt_DebugRenderer : public JPH::DebugRenderer
{
public:
    FCk_Jolt_DebugRenderer();
    ~FCk_Jolt_DebugRenderer() override;

public:
    /// The process-wide instance, created on first use and destroyed on FCoreDelegates::OnEnginePreExit.
    static auto
    Get_OrCreate() -> FCk_Jolt_DebugRenderer&;

public:
    auto
    DrawLine(
        JPH::RVec3Arg inFrom,
        JPH::RVec3Arg inTo,
        JPH::ColorArg inColor) -> void override;

    auto
    DrawTriangle(
        JPH::RVec3Arg inV1,
        JPH::RVec3Arg inV2,
        JPH::RVec3Arg inV3,
        JPH::ColorArg inColor,
        ECastShadow inCastShadow) -> void override;

    auto
    DrawText3D(
        JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor,
        float inHeight) -> void override;

    auto
    CreateTriangleBatch(
        const Triangle* inTriangles,
        int inTriangleCount) -> Batch override;

    auto
    CreateTriangleBatch(
        const Vertex* inVertices,
        int inVertexCount,
        const JPH::uint32* inIndices,
        int inIndexCount) -> Batch override;

    auto
    DrawGeometry(
        JPH::RMat44Arg inModelMatrix,
        const JPH::AABox& inWorldSpaceBounds,
        float inLODScaleSq,
        JPH::ColorArg inModelColor,
        const GeometryRef& inGeometry,
        ECullMode inCullMode,
        ECastShadow inCastShadow,
        EDrawMode inDrawMode) -> void override;

public:
    // ---- Whole-array draw session (the subsystem's in-world DrawBodies path) ----

    /// Bind the target and reset its per-frame accumulation. Call before DrawBodies.
    auto
    BeginFrame(
        FCk_Jolt_DebugDrawTarget& InTarget) -> void;

    /// Reconcile the active target's accumulated draws into its instanced components, then unbind it.
    auto
    EndFrame() -> void;

    // ---- Persistent-slot capture session (the debug-draw capture processor) ----

    auto
    BeginCapture(
        FCk_Jolt_DebugDrawTarget& InTarget) -> void;

    /// Opens one body's draw. Every DrawGeometry until EndBody belongs to this body key.
    auto
    BeginBody(
        uint64 InBodyKey,
        ECk_Jolt_DebugDraw_ColorClass InColorClass) -> void;

    /// Reconciles the body's accumulated draws against its persistent instance slots.
    auto
    EndBody() -> void;

    /// Releases every instance slot held for a body that no longer exists — including the selection overlay
    /// tracing it, which is keyed separately and would otherwise outlive the body it traces. Counts into the
    /// active capture's stats: every caller is a capture step.
    auto
    Release_BodySlots(
        uint64 InBodyKey) -> void;

    /// Prunes dead buckets and refreshes materials, then unbinds the target.
    auto
    EndCapture() -> void;

    /*
     * One whole capture into the target: a revision-keyed full pass over every INACTIVE body, a per-frame
     * active-body pass with sleep/activation recolouring, and a character pass. The capture processor is the
     * only caller — a presentation consumer drives demand and render mode through the target alone and never
     * names JPH::PhysicsSystem, so it can never race the step.
     * An INVALID transient entity degrades gracefully: no baked-static classification, no characters.
     */
    auto
    Capture_JoltWorld(
        FCk_Jolt_DebugDrawTarget& InTarget,
        JPH::PhysicsSystem& InPhysicsSystem,
        const ck::jolt::debug_draw::FCaptureRevisions& InRevisions,
        const FCk_Handle& InTransientEntity) -> void;

private:
    // Builds the bucket's transient mesh + ISM on first use. False once the bucket is known unbuildable.
    auto
    TryEnsure_BucketIsm(
        FCk_Jolt_DebugDrawTarget& InTarget,
        const ck::jolt::debug_draw::FBucketKey& InKey,
        ck::jolt::debug_draw::FBucket& InOutBucket) -> bool;

private:
    struct FImpl;
    TPimplPtr<FImpl> _Impl;
};

#endif

// --------------------------------------------------------------------------------------------------------------------
