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
// bucket; lines and text route into the active target's line and label channels. Jolt allows exactly ONE
// DebugRenderer instance per process (JPH_ASSERT in DebugRenderer's constructor), so this owns only the
// world-agnostic geometry/batch cache and reconciles into whichever FCk_Jolt_DebugDrawTarget is active for the
// current draw session. Game-thread only. Rationale: CkJolt/Claude.md § "Debug draw + stats".
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
    // ---- Persistent-slot capture session: the ONLY draw session shape ----

    /// Binds the target, resets its stats, and flushes its line + label channels for this capture.
    auto
    BeginCapture(
        FCk_Jolt_DebugDrawTarget& InTarget) -> void;

    /// Opens one body's draw. Every DrawGeometry until EndBody belongs to this body key, colour-class index and
    /// sensor-material contract; sensor state is deliberately independent of the target's current colour mode.
    auto
    BeginBody(
        uint64 InBodyKey,
        uint8 InColorClassIndex,
        bool InIsSensor) -> void;

    /// Reconciles the body's accumulated draws against its persistent instance slots.
    auto
    EndBody() -> void;

    /// Releases every instance slot held for a body that no longer exists — including the selection and hover
    /// overlays tracing it, which are keyed separately and would otherwise outlive the body they trace.
    /// The counting mode is the caller's: a body that DIED is a removal this capture performed, while a body
    /// merely dropping its Shape flag is bookkeeping the stats must not report as churn.
    auto
    Release_BodySlots(
        uint64 InBodyKey,
        ck::jolt::debug_draw::EStatCounting InStatCounting) -> void;

    /// Prunes dead buckets, refreshes materials, pushes the line + External channels into the target's line
    /// component, then unbinds the target.
    auto
    EndCapture() -> void;

    /*
     * One whole capture into the target: a revision-keyed full pass over every INACTIVE body, a per-frame
     * active-body pass with sleep/activation recolouring, a character pass, the flag-gated per-body extras and
     * the flag-gated constraint draws. Only two callers: the capture processor (for registered targets) and the
     * Jolt subsystem's Tick (for its own in-world default target) — a presentation consumer drives demand,
     * render mode and draw flags through the target alone and never names JPH::PhysicsSystem, so it can never
     * race the step.
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
