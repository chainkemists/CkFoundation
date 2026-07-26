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

// Batched JPH::DebugRenderer: geometry becomes transient UStaticMeshes instanced per (geometry, color)
// bucket; line/text primitives stay immediate-mode. Game-thread only, mirroring DrawBodies' invocation
// from the subsystem Tick. Rationale: CkJolt/CLAUDE.md § "Debug draw + stats".
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
