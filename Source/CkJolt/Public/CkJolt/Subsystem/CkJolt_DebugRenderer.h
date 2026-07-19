#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Templates/PimplPtr.h>
#include <UObject/WeakObjectPtr.h>

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include <Jolt/Renderer/DebugRenderer.h>

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

// Batched implementation of Jolt's debug renderer.
//
// The previous implementation derived from JPH::DebugRendererSimple, whose DrawGeometry fallback decomposes
// EVERY triangle of EVERY body into individual DrawDebugLine calls, EVERY frame — at stress-gym body counts
// that is hundreds of thousands of one-frame line submissions per frame, all on the game thread.
//
// This implementation uses the batch API the DebugRenderer interface is designed around:
// - CreateTriangleBatch is called ONCE per unique geometry (Jolt's shapes cache their GeometryRef —
//   HeightFieldShape/MeshShape/ConvexHullShape hold a mutable mGeometry; primitives share unit geometry),
//   and we store the triangle data CPU-side, lazily building a transient UStaticMesh on first draw.
// - DrawGeometry only accumulates (batch, model transform, color) into per-(geometry, color) buckets.
// - EndFrame reconciles each bucket into one UInstancedStaticMeshComponent: buckets whose instance
//   transforms did not change (all static bodies, all sleeping bodies) cost nothing; moving bodies cost one
//   instance-transform upload instead of a rebuilt line soup.
//
// DrawLine/DrawTriangle/DrawText3D remain immediate-mode (velocity vectors, transform axes, contact
// normals are genuinely line-shaped and low-count).
//
// Game-thread only, mirroring DrawBodies' invocation from the subsystem Tick.
class CkJoltDebugger : public JPH::DebugRenderer
{
public:
    CkJoltDebugger();
    ~CkJoltDebugger() override;

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
    /// Reset per-frame accumulation. Call before DrawBodies.
    auto
    BeginFrame() -> void;

    /// Reconcile accumulated draws into the instanced components. Call after DrawBodies.
    auto
    EndFrame() -> void;

    /// Clear all live instances. Call when debug draw is gated off so stale geometry does not linger.
    /// Idempotent and cheap when nothing is live.
    auto
    HideAll() -> void;

public:
    TWeakObjectPtr<UWorld> _World;

private:
    struct FImpl;
    TPimplPtr<FImpl> _Impl;
};

#endif

// --------------------------------------------------------------------------------------------------------------------
