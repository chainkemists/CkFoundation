#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Templates/PimplPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UInstancedStaticMeshComponent;
class UWorld;
class FCk_Jolt_DebugRenderer;

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_Jolt_DebugDraw_RenderMode : uint8
{
    Solid,
    Wireframe
};

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_Jolt_DebugDraw_ColorClass : uint8
{
    Static,
    Kinematic,
    Dynamic_Awake,
    Dynamic_Sleeping,
    Sensor,
    BakedStatic,
    Character
};

// --------------------------------------------------------------------------------------------------------------------

struct CKJOLT_API FCk_Jolt_DebugDrawPalette
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDrawPalette);

public:
    auto
    Get_Color(
        ECk_Jolt_DebugDraw_ColorClass InColorClass) const -> FLinearColor;

private:
    FLinearColor _StaticColor          = FLinearColor{0.35f, 0.35f, 0.38f, 1.0f};
    FLinearColor _KinematicColor       = FLinearColor{0.10f, 0.80f, 0.25f, 1.0f};
    FLinearColor _DynamicAwakeColor    = FLinearColor{0.95f, 0.85f, 0.10f, 1.0f};
    FLinearColor _DynamicSleepingColor = FLinearColor{0.90f, 0.20f, 0.15f, 1.0f};
    FLinearColor _SensorColor          = FLinearColor{0.15f, 0.55f, 0.95f, 1.0f};
    FLinearColor _BakedStaticColor     = FLinearColor{0.55f, 0.45f, 0.30f, 1.0f};
    FLinearColor _CharacterColor       = FLinearColor{0.85f, 0.35f, 0.85f, 1.0f};

    float _SleepingDimFactor = 0.55f;
    float _Opacity = 0.5f;

public:
    CK_PROPERTY(_StaticColor);
    CK_PROPERTY(_KinematicColor);
    CK_PROPERTY(_DynamicAwakeColor);
    CK_PROPERTY(_DynamicSleepingColor);
    CK_PROPERTY(_SensorColor);
    CK_PROPERTY(_BakedStaticColor);
    CK_PROPERTY(_CharacterColor);
    CK_PROPERTY(_SleepingDimFactor);
    CK_PROPERTY(_Opacity);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    struct FDebugDrawStats
    {
        int32 _BodiesCaptured = 0;
        int32 _InstancesAdded = 0;
        int32 _InstancesUpdated = 0;
        int32 _InstancesRemoved = 0;
        bool  _FullPassRan = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Per-world retained state of the batched Jolt debug draw: the (geometry, colour-class) bucket map and the
 * UInstancedStaticMeshComponents behind it, the material mode, and the palette. The geometry cache itself is
 * world-agnostic and lives on the single FCk_Jolt_DebugRenderer, which reconciles into whichever target is
 * active for the current draw session.
 *
 * NO Jolt type appears in this header — a presentation consumer binds a target, flips demand and render mode,
 * and reads counts, without ever seeing JPH. Everything Jolt-shaped lives behind the opaque impl.
 *
 * The target reacts to its world being torn down (PIE end, map change) by releasing its components, so it
 * never roots a dying world; it stays reusable afterwards.
 *
 * Game thread only. Non-copyable: the destructor destroys the components it owns.
 */
class CKJOLT_API FCk_Jolt_DebugDrawTarget
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDrawTarget);

public:
    friend class ::FCk_Jolt_DebugRenderer;

public:
    explicit FCk_Jolt_DebugDrawTarget(
        UWorld* InWorld);

    ~FCk_Jolt_DebugDrawTarget();

    FCk_Jolt_DebugDrawTarget(const FCk_Jolt_DebugDrawTarget&) = delete;
    auto operator=(const FCk_Jolt_DebugDrawTarget&) -> FCk_Jolt_DebugDrawTarget& = delete;

public:
    /// Clear every live instance and drop the retained capture state, so the next capture rebuilds against
    /// fresh world state. Idempotent and cheap when nothing is live.
    auto
    HideAll() -> void;

    /// Consumer demand switch. Dropping demand hides immediately — a target that stops being pumped must not
    /// leave a frozen snapshot of the world behind it.
    auto
    Set_IsDesired(
        bool InIsDesired) -> FCk_Jolt_DebugDrawTarget&;

    /// Swap every bucket's material 0 between the solid and wireframe MIDs. No geometry is rebuilt.
    auto
    Set_RenderMode(
        ECk_Jolt_DebugDraw_RenderMode InRenderMode) -> void;

    auto
    Set_Opacity(
        float InOpacity) -> void;

    /// Invalidates the retained capture so the next one repaints every body through the new palette.
    auto
    Set_Palette(
        const FCk_Jolt_DebugDrawPalette& InPalette) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_World() const -> TWeakObjectPtr<UWorld>;

    auto
    Get_IsDesired() const -> bool;

    auto
    Get_RenderMode() const -> ECk_Jolt_DebugDraw_RenderMode;

    auto
    Get_Palette() const -> const FCk_Jolt_DebugDrawPalette&;

    auto
    Get_LastCaptureStats() const -> const ck::jolt::debug_draw::FDebugDrawStats&;

    /// Live instance count across every bucket.
    auto
    Get_NumInstances() const -> int32;

    auto
    Get_NumBuckets() const -> int32;

    auto
    Get_Isms() const -> TArray<UInstancedStaticMeshComponent*>;

    auto
    Get_BucketColorClasses() const -> TArray<ECk_Jolt_DebugDraw_ColorClass>;

public:
    // Opaque by design: the declaration is public only so the module's own debug-draw translation units can
    // name it. It is incomplete everywhere else, and _Impl itself stays private.
    struct FImpl;

private:
    TPimplPtr<FImpl> _Impl;
};

// --------------------------------------------------------------------------------------------------------------------
