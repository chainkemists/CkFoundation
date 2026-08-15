#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Misc/Optional.h>
#include <Templates/PimplPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UInstancedStaticMeshComponent;
class UWorld;
class FCk_Jolt_DebugRenderer;
struct FCk_Handle;

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
    Character,

    // The selection overlay: a SECOND instance of the highlighted body's geometry, drawn alongside the body's
    // normal instance rather than replacing it. Its own class so a population toggle can never hide it.
    Highlight,

    // Sentinel. The visibility mask packs one bit per class into a uint8, so the class count is capped —
    // see the static_assert beside Get_ClassBit.
    Count
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
    FLinearColor _HighlightColor       = FLinearColor{1.00f, 0.85f, 0.10f, 1.0f};

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
    CK_PROPERTY(_HighlightColor);
    CK_PROPERTY(_SleepingDimFactor);
    CK_PROPERTY(_Opacity);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    struct FDebugDrawStats
    {
        // Bodies the capture DREW, not bodies it walked. The full pass skips every inactive body whose pose,
        // shape and colour inputs are unchanged, so a re-run over an unchanged scene reports zero here while
        // still visiting every body.
        int32 _BodiesCaptured = 0;
        int32 _InstancesAdded = 0;
        int32 _InstancesUpdated = 0;
        int32 _InstancesRemoved = 0;
        bool  _FullPassRan = false;
        bool  _SweepRan = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * The two change tokens a capture reconciles against, both owned by the Jolt world and both monotonic.
     * _StaticScene gates the full pass over inactive bodies; _BodyRemoved gates the sweep that releases the
     * slots of a destroyed SLEEPING body, which neither body pass can see. A capture that carried no token
     * would have to do both O(all) walks every frame.
     */
    struct FCaptureRevisions
    {
        uint64 _StaticScene = 0;
        uint64 _BodyRemoved = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * The debug-draw slot key of a rigid body, from its BodyID's index+sequence number. This is the single
     * definition of that widening — the capture keys every body it draws through it, so a consumer holding a
     * body id names the same body to Set_HighlightedBody / reads it back from TryPick_Body without having to
     * know how the keyspace is laid out.
     */
    CKJOLT_API auto
    Make_BodyKey(
        uint32 InIndexAndSequenceNumber) -> uint64;

    /*
     * The debug-draw slot key of a JoltCharacter entity. A CharacterVirtual has no BodyID, so its key is its
     * entity id lifted clear of the BodyID keyspace — this is the single definition of that convention, and the
     * only way a presentation consumer can name a character to Set_HighlightedBody without seeing Jolt.
     * An invalid handle yields 0, which matches no drawn character.
     */
    CKJOLT_API auto
    Make_CharacterBodyKey(
        const FCk_Handle& InCharacterEntity) -> uint64;
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

    /// Show/hide one colour class. Component-level only — the class keeps capturing while hidden, so the
    /// toggle costs one SetVisibility per affected bucket and unhiding shows current poses, never stale ones.
    auto
    Set_ClassVisibility(
        ECk_Jolt_DebugDraw_ColorClass InColorClass,
        bool InIsVisible) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_IsClassVisible(
        ECk_Jolt_DebugDraw_ColorClass InColorClass) const -> bool;

    /*
     * Select one drawn body (or character, keyed through Make_CharacterBodyKey) for the Highlight overlay. The
     * body keeps its normal instance and gains a SECOND one in the Highlight colour class, which follows it
     * every capture. The previous selection's overlay instance is released here rather than at the next
     * capture, so a cleared selection stops drawing immediately. An unset key clears the selection.
     */
    auto
    Set_HighlightedBody(
        TOptional<uint64> InBodyKey) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_HighlightedBody() const -> TOptional<uint64>;

    /// World-space bounds of the highlighted body's NORMAL instances. Unset when nothing is highlighted, or
    /// when the selected body has not been drawn yet.
    auto
    Get_HighlightedBodyBounds() const -> TOptional<FBox>;

    /*
     * World-space linear velocity of the highlighted body, sampled by the capture in the same async-safe window
     * it draws from. Unset when nothing is highlighted, when the highlighted key belongs to a character (a
     * CharacterVirtual has no rigid-body velocity), and when the last capture did not draw the body — a
     * sleeping or static body is only drawn on a scene-revision pass. A consumer reads this instead of querying
     * the physics system, which it must never touch.
     */
    auto
    Get_HighlightedBodyLinearVelocity() const -> TOptional<FVector>;

    /*
     * Nearest live instance a ray hits, as the body key that drew it. Tests the ray against each instance's
     * ORIENTED mesh bounds (the ray is pushed into instance space and tested against the local box), which is
     * tighter than a world-space AABB and needs no per-instance box rebuild. Hidden colour classes and the
     * Highlight overlay are not pickable. O(live instances) per call — this is a click handler, not a tick.
     * InDirection needs no normalization; hit ordering is parametric along it.
     */
    auto
    TryPick_Body(
        const FVector& InOrigin,
        const FVector& InDirection) const -> TOptional<uint64>;

    /// World-space bounds of everything this target currently DRAWS — hidden classes are excluded, because the
    /// caller is a camera framing what the viewer can see. Invalid (`IsValid == 0`) when nothing is drawn.
    /// Derived from the bucket components' own bounds, so it is conservative, not a tight fit.
    auto
    Get_ContentBounds() const -> FBox;

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
