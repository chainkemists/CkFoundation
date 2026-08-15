#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CkJolt_Common.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"

#include <CoreMinimal.h>
#include <Misc/EnumClassFlags.h>
#include <Misc/Optional.h>
#include <Templates/PimplPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UInstancedStaticMeshComponent;
class ULineBatchComponent;
class UWorld;
class FCk_Jolt_DebugRenderer;
class FCk_Jolt_DebugDrawTarget;
struct FCk_Handle;

// The contact recorder is keyed by the physics world whose solve produced the record, and a world is named by
// its PhysicsSystem address alone — declared, never defined here, so this header still pulls in no Jolt header
// and a presentation consumer still binds a target without seeing Jolt.
// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
}

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_Jolt_DebugDraw_RenderMode : uint8
{
    Solid,
    Wireframe
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * Which of Jolt's debug-draw vocabulary a capture emits into THIS target. Per-target, so the preview viewport and
 * the in-world draw never fight over one set of toggles — with the sole exception of the contact flags, whose JPH
 * statics are process-wide (see CkJolt/Claude.md § "Debug draw + stats").
 *
 * `Shape` is the batched instanced-mesh path; every other member produces LINES, which the target's line channel
 * clears and re-emits every capture. Enabling any per-body extra therefore also defeats the incremental full
 * pass's skip (an inactive body it skipped would emit no lines and its extras would vanish the next capture).
 *
 * Named ECk_ rather than the ruling's FCk_ spelling because this is an enum and every other enum in the module
 * carries the house prefix. SleepStats is deliberately absent: Jolt draws it from MotionProperties' internal
 * sleep-test spheres, which sit below that header's FOR INTERNAL USE ONLY banner and have no public accessor.
 */
enum class ECk_Jolt_DebugDrawFlags : uint32
{
    None                      = 0,
    Shape                     = 1u << 0,
    Velocity                  = 1u << 1,
    AngularVelocity           = 1u << 2,
    WorldTransform            = 1u << 3,
    CenterOfMassTransform     = 1u << 4,
    BoundingBox               = 1u << 5,
    MassAndInertia            = 1u << 6,
    Constraints               = 1u << 7,
    ConstraintLimits          = 1u << 8,
    ConstraintReferenceFrames = 1u << 9,
    ContactPoints             = 1u << 10,
    ContactNormals            = 1u << 11,
    SupportingFaces           = 1u << 12,
    Labels                    = 1u << 13,
};

ENUM_CLASS_FLAGS(ECk_Jolt_DebugDrawFlags);

// --------------------------------------------------------------------------------------------------------------------

/*
 * One JPH DrawText3D, held for the frame that produced it. The facility does not render text itself — a consumer
 * projects these to screen (the preview viewport) or hands them to DrawDebugString (the in-world draw).
 */
struct CKJOLT_API FCk_Jolt_DebugDrawLabel
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDrawLabel);

public:
    FCk_Jolt_DebugDrawLabel() = default;

    FCk_Jolt_DebugDrawLabel(
        FVector InWorldPosition,
        FString InText,
        FLinearColor InColor)
        : _WorldPosition(InWorldPosition)
        , _Text(MoveTemp(InText))
        , _Color(InColor)
    {
    }

private:
    FVector _WorldPosition = FVector::ZeroVector;
    FString _Text;
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY(_WorldPosition);
    CK_PROPERTY(_Text);
    CK_PROPERTY(_Color);
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * What a target colours its bodies BY. The mode decides which class index a body lands in, and therefore which
 * bucket draws it; changing it re-buckets everything the next capture touches.
 *
 * Highlight and Hover are deliberately NOT modes: they are the two mode-independent class indices below, so a
 * selection stays magenta whatever the bodies around it are coloured by.
 */
enum class ECk_Jolt_DebugDrawColorMode : uint8
{
    BodyClass,
    SleepState,
    ObjectLayer,
    ShapeType
};

// --------------------------------------------------------------------------------------------------------------------

/// The classes of ECk_Jolt_DebugDrawColorMode::BodyClass — the default mode. Every other mode computes its own
/// class indices, which are NOT these values (see ck::jolt::debug_draw's per-mode class counts).
enum class ECk_Jolt_DebugDraw_ColorClass : uint8
{
    Static,
    Kinematic,
    Dynamic_Awake,
    Dynamic_Sleeping,
    Sensor,
    BakedStatic,
    Character,

    Count
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    /*
     * ONE class-index space, shared by every colour mode. The visibility mask packs one bit per index into a
     * uint64, so 64 is the hard ceiling and every per-mode class count below has to fit under the two reserved
     * indices.
     *
     * Highlight and Hover sit at the TOP two indices in every mode: a selection must never change colour, or be
     * hideable, because the viewer switched to colouring by island.
     */
    inline constexpr uint8 MaxColorClasses = 64;
    inline constexpr uint8 HighlightClassIndex = 62;
    inline constexpr uint8 HoverClassIndex = 63;

    /// The wheel every "identity" mode (object layer, shape type) draws its colours from — a number whose value
    /// means nothing beyond "not the same as its neighbour".
    inline constexpr uint8 DistinctColorPaletteSize = 16;

    /*
     * How many captures pass between two refreshes of the expensive half of FCk_Jolt_DebugDraw_WorldStats
     * (P5-D61/S10 — a CONSTANT, deliberately not a knob). GetBodyStats is documented in Jolt's own header as
     * "slow, iterates through all bodies", so at 100k bodies a per-frame sample would dominate the capture.
     */
    inline constexpr int32 WorldStatsSampleInterval = 30;

    /*
     * ObjectLayer mode names one class per registered layer. 62 indices are available, so index 61 is the
     * catch-all: a body whose object layer is 61 or higher lands there. P5-D42 phrased the cut as "> 61 → Other";
     * with Highlight and Hover reserved there is no 63rd index to put Other in, so the top NAMED index doubles
     * as it and the legend reads "Layer 61+".
     */
    inline constexpr uint8 MaxNamedObjectLayer = 61;

    /// The classes of ECk_Jolt_DebugDrawColorMode::SleepState.
    enum class ESleepStateClass : uint8
    {
        Static,
        Kinematic,
        Awake,
        Asleep,

        Count
    };

    /*
     * The classes of ECk_Jolt_DebugDrawColorMode::ShapeType. A COMPACT re-indexing of JPH::EShapeSubType rather
     * than the raw enum value: that enum reserves User1..User8 and UserConvex1..UserConvex8 in the middle and
     * appends Plane/TaperedCylinder/Empty at the end, so indexing a name table by its value would both waste 16
     * indices and mislabel the three appended types.
     */
    enum class EShapeTypeClass : uint8
    {
        Sphere,
        Box,
        Triangle,
        Capsule,
        TaperedCapsule,
        Cylinder,
        TaperedCylinder,
        ConvexHull,
        StaticCompound,
        MutableCompound,
        RotatedTranslated,
        Scaled,
        OffsetCenterOfMass,
        Mesh,
        HeightField,
        SoftBody,
        Plane,
        Empty,

        // Every user-defined sub-type, which a game may define and this facility cannot name.
        Other,

        Count
    };

    inline constexpr auto
        Get_ClassIndex(
            ECk_Jolt_DebugDraw_ColorClass InColorClass)
        -> uint8
    {
        return static_cast<uint8>(InColorClass);
    }
}

// --------------------------------------------------------------------------------------------------------------------

/// One row of a colour legend: what a class index is called in the current mode, and the colour it draws in.
struct CKJOLT_API FCk_Jolt_DebugDrawLegendEntry
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDrawLegendEntry);

public:
    FCk_Jolt_DebugDrawLegendEntry() = default;

    FCk_Jolt_DebugDrawLegendEntry(
        uint8 InClassIndex,
        FText InName,
        FLinearColor InColor)
        : _ClassIndex(InClassIndex)
        , _Name(MoveTemp(InName))
        , _Color(InColor)
    {
    }

private:
    uint8 _ClassIndex = 0;
    FText _Name;
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY(_ClassIndex);
    CK_PROPERTY(_Name);
    CK_PROPERTY(_Color);
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * Everything a presentation consumer wants to know about the PRIMARY selected rigid body, sampled by the capture
 * in the same async-safe window it draws from — never by reading JPH::PhysicsSystem from Slate, which this module
 * bans outright (CkJolt/Claude.md § Anti-patterns).
 *
 * JPH-free by construction: Jolt scalars are plain floats, its enums are mirrored onto the module's own ECk_ ones,
 * and the shape's type/sub-type arrive as display strings because a consumer only ever prints them.
 */
struct CKJOLT_API FCk_Jolt_DebugDraw_BodySample
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDraw_BodySample);

private:
    FVector _LinearVelocity = FVector::ZeroVector;
    FVector _AngularVelocity = FVector::ZeroVector;

    /// World-space AABB Jolt keeps for the body, not a bound derived from the drawn instances.
    FBox _WorldBounds = FBox{ForceInit};

    /// The scale a JPH::ScaledShape wraps its child in; unit for every shape that is not one.
    FVector _ShapeScale = FVector::OneVector;

    FString _ShapeType;
    FString _ShapeSubType;

    /// ZERO MEANS INFINITE — Jolt stores an inverse mass, and a static (or explicitly infinite-mass) body has an
    /// inverse of 0, which has no finite reciprocal.
    float _Mass = 0.0f;

    float _Friction = 0.0f;
    float _Restitution = 0.0f;
    float _GravityFactor = 0.0f;

    uint64 _UserData = 0;
    uint16 _ObjectLayer = 0;
    uint8 _BroadPhaseLayer = 0;

    ECk_MotionType _MotionType = ECk_MotionType::Static;
    ECk_MotionQuality _MotionQuality = ECk_MotionQuality::Discrete;

    bool _IsSensor = false;

    /// UNSET for a static body, and only for one: Body::GetAllowSleeping dereferences its MotionProperties
    /// without a guard, so a static body must never be asked (JPH_ENABLE_ASSERTS is on in every configuration).
    TOptional<bool> _AllowsSleeping;

public:
    CK_PROPERTY(_LinearVelocity);
    CK_PROPERTY(_AngularVelocity);
    CK_PROPERTY(_WorldBounds);
    CK_PROPERTY(_ShapeScale);
    CK_PROPERTY(_ShapeType);
    CK_PROPERTY(_ShapeSubType);
    CK_PROPERTY(_Mass);
    CK_PROPERTY(_Friction);
    CK_PROPERTY(_Restitution);
    CK_PROPERTY(_GravityFactor);
    CK_PROPERTY(_UserData);
    CK_PROPERTY(_ObjectLayer);
    CK_PROPERTY(_BroadPhaseLayer);
    CK_PROPERTY(_MotionType);
    CK_PROPERTY(_MotionQuality);
    CK_PROPERTY(_IsSensor);
    CK_PROPERTY(_AllowsSleeping);
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * The character equivalent of FCk_Jolt_DebugDraw_BodySample. A CharacterVirtual has no JPH::Body, so none of the
 * rigid-body fields exist for it — what it has instead is a ground contact, which is the whole question a
 * character debugger is opened to answer.
 */
struct CKJOLT_API FCk_Jolt_DebugDraw_CharacterSample
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDraw_CharacterSample);

private:
    FVector _Velocity = FVector::ZeroVector;
    FVector _GroundNormal = FVector::ZeroVector;
    FVector _GroundVelocity = FVector::ZeroVector;
    FVector _Up = FVector::UpVector;

    /// The ground body in the SAME keyspace the capture draws in (Make_BodyKey), so a consumer can select it.
    /// Unset while the character is unsupported — 0 is a valid body key and would name the first body created.
    TOptional<uint64> _GroundBodyKey;

    ECk_JoltCharacter_GroundState _GroundState = ECk_JoltCharacter_GroundState::InAir;

public:
    CK_PROPERTY(_Velocity);
    CK_PROPERTY(_GroundNormal);
    CK_PROPERTY(_GroundVelocity);
    CK_PROPERTY(_Up);
    CK_PROPERTY(_GroundBodyKey);
    CK_PROPERTY(_GroundState);
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * One body the selection is currently touching, from the capture's on-demand NarrowPhaseQuery::CollideShape of the
 * selected body's own shape. Not a ContactListener record: the listener's manifolds exist only inside the solve,
 * while this is a query the capture can run whenever a consumer asks for it.
 *
 * A negative penetration depth is a SEPARATED pair found within the query's separation distance (the resting case),
 * not an error.
 */
struct CKJOLT_API FCk_Jolt_DebugDraw_ContactEntry
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDraw_ContactEntry);

private:
    uint64 _OtherBodyKey = 0;
    int32 _NumContactPoints = 0;
    float _PenetrationDepth = 0.0f;
    TArray<FVector> _ContactPoints;

public:
    CK_PROPERTY(_OtherBodyKey);
    CK_PROPERTY(_NumContactPoints);
    CK_PROPERTY(_PenetrationDepth);
    CK_PROPERTY(_ContactPoints);
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * What the world itself is doing, for a stats panel — filled BY THE CAPTURE from the physics system, in the same
 * async-safe window it draws from, so a consumer never reads JPH::PhysicsSystem for a count either (P6-D48).
 *
 * The fields split into three groups by what they COST:
 *
 *  - SAMPLED (`_NumBodies` .. `_NumConstraints`): PhysicsSystem::GetBodyStats walks every body and GetConstraints
 *    returns the constraint array BY VALUE with a refcount bump per element. Both are refreshed only every
 *    `WorldStatsSampleInterval` captures, so at the campaign's 100k bar they cost nothing per frame. `_SampleAge`
 *    is how many captures ago that happened — 0 on the capture that refreshed them. A UI labels them "(sampled)".
 *  - LIVE (`_NumActiveRigidBodies`, `_NumActiveSoftBodies`): plain counters, read every capture. Jolt's body stats
 *    carry an active-soft-body count too; it is deliberately NOT mirrored into the sampled block, because the live
 *    one means the same thing and is always fresher.
 *  - PUSHED (`_LastStepDurationMs`, `_ContactPairsLastStep`): owned by the Jolt world rather than the physics
 *    system, so the capture cannot reach them — whoever pumps the capture pushes them in (Set_StepStats).
 */
struct CKJOLT_API FCk_Jolt_DebugDraw_WorldStats
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDraw_WorldStats);

private:
    int32 _NumBodies = 0;
    int32 _MaxBodies = 0;
    int32 _NumStaticBodies = 0;
    int32 _NumDynamicBodies = 0;
    int32 _NumActiveDynamicBodies = 0;
    int32 _NumKinematicBodies = 0;
    int32 _NumActiveKinematicBodies = 0;
    int32 _NumSoftBodies = 0;
    int32 _NumConstraints = 0;

    int32 _NumActiveRigidBodies = 0;
    int32 _NumActiveSoftBodies = 0;

    /// Captures since the sampled block was refreshed. Zero on the capture that refreshed it.
    int32 _SampleAge = 0;

    int32 _ContactPairsLastStep = 0;

    float _LastStepDurationMs = 0.0f;

    /// False until the first capture has taken a sample, so a consumer can tell "no bodies" from "not yet asked".
    bool _HasSample = false;

public:
    CK_PROPERTY(_NumBodies);
    CK_PROPERTY(_MaxBodies);
    CK_PROPERTY(_NumStaticBodies);
    CK_PROPERTY(_NumDynamicBodies);
    CK_PROPERTY(_NumActiveDynamicBodies);
    CK_PROPERTY(_NumKinematicBodies);
    CK_PROPERTY(_NumActiveKinematicBodies);
    CK_PROPERTY(_NumSoftBodies);
    CK_PROPERTY(_NumConstraints);
    CK_PROPERTY(_NumActiveRigidBodies);
    CK_PROPERTY(_NumActiveSoftBodies);
    CK_PROPERTY(_SampleAge);
    CK_PROPERTY(_ContactPairsLastStep);
    CK_PROPERTY(_LastStepDurationMs);
    CK_PROPERTY(_HasSample);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKJOLT_API FCk_Jolt_DebugDrawPalette
{
public:
    CK_GENERATED_BODY(FCk_Jolt_DebugDrawPalette);

public:
    /// The colour of one class index IN ONE MODE. Highlight and Hover answer the same in every mode; every other
    /// index is interpreted by the mode that produced it.
    auto
    Get_Color(
        ECk_Jolt_DebugDrawColorMode InColorMode,
        uint8 InClassIndex) const -> FLinearColor;

private:
    FLinearColor _StaticColor          = FLinearColor{0.35f, 0.35f, 0.38f, 1.0f};
    FLinearColor _KinematicColor       = FLinearColor{0.10f, 0.80f, 0.25f, 1.0f};
    FLinearColor _DynamicAwakeColor    = FLinearColor{0.95f, 0.85f, 0.10f, 1.0f};
    FLinearColor _DynamicSleepingColor = FLinearColor{0.90f, 0.20f, 0.15f, 1.0f};
    FLinearColor _SensorColor          = FLinearColor{0.15f, 0.55f, 0.95f, 1.0f};
    FLinearColor _BakedStaticColor     = FLinearColor{0.55f, 0.45f, 0.30f, 1.0f};
    FLinearColor _CharacterColor       = FLinearColor{0.85f, 0.35f, 0.85f, 1.0f};

    // Magenta, and deliberately not near any other entry: the old amber sat between the awake yellow and the
    // baked tan, which is what made a selection unreadable in PIE (P5-D41).
    FLinearColor _HighlightColor       = FLinearColor{1.00f, 0.15f, 0.85f, 1.0f};

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

    // ----------------------------------------------------------------------------------------------------------------
    // Contact recording (P5-D40). Contacts exist only DURING PhysicsSystem::Update, so they cannot be captured
    // the way bodies are — they are recorded around the step and replayed on the game thread afterwards.
    //
    // Jolt's contact draw switches are process-wide `static bool`s with no per-system variant, so the demand is
    // the UNION of every live target's contact flags: turning contacts off in one target turns them off for the
    // whole process. That is disclosed rather than worked around; every other draw flag stays per-target.
    // ----------------------------------------------------------------------------------------------------------------

    /// True while at least one live target asks for contacts. The recorder and the JPH statics are both driven
    /// from this, so nothing is recorded — and Jolt emits nothing — when no consumer wants it.
    CKJOLT_API auto
    Get_IsAnyTargetDemandingContacts() -> bool;

    /*
     * Opens the record scope around one world's PhysicsSystem::Update. Every DrawLine that arrives while the
     * scope is open (i.e. from inside the solve, on any thread) is appended to a lock-guarded per-world buffer
     * instead of being dropped. A no-op when nothing demands contacts.
     *
     * Buffers are PER WORLD, but only ONE world may record at a time: Jolt's contact draw is process-wide and a
     * DrawLine carries no world, so a second world whose solve overlaps the first is SKIPPED (Verbose, once)
     * rather than having its manifolds mixed into the recording world's record and replayed into its targets.
     * End_ContactRecord swaps that world's filled buffer aside for the game thread.
     */
    CKJOLT_API auto
    Begin_ContactRecord(
        const JPH::PhysicsSystem* InPhysicsSystem) -> void;

    CKJOLT_API auto
    End_ContactRecord(
        const JPH::PhysicsSystem* InPhysicsSystem) -> void;

    /// Drops a world's record buffers. Called when the world goes away, so a PhysicsSystem allocated at the same
    /// address later can never inherit a dead world's contacts.
    CKJOLT_API auto
    Forget_ContactRecord(
        const JPH::PhysicsSystem* InPhysicsSystem) -> void;

    /*
     * Game-thread consumption: takes ONE world's last completed record and writes it into every one of that
     * world's targets that asks for contacts, clearing the channel of every target that does not. Called once
     * per frame by FProcessor_JoltDebugDraw_Capture BEFORE it captures, so the replayed lines are flushed by the
     * same capture that draws the bodies they belong to.
     *
     * Under async physics the record belongs to the step consumed at the start of THIS frame, so the contacts
     * lag the shapes by one frame. Accepted (P5-D61/S1) and documented.
     */
    CKJOLT_API auto
    Replay_RecordedContacts(
        const JPH::PhysicsSystem* InPhysicsSystem,
        TArrayView<const TSharedPtr<FCk_Jolt_DebugDrawTarget>> InTargets) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Per-world retained state of the batched Jolt debug draw: the (geometry, colour-class) bucket map and the
 * UInstancedStaticMeshComponents behind it, the material mode, the palette, the draw flags, and the three
 * non-mesh channels (a ULineBatchComponent for lines, a per-frame label array, and the retained named External
 * sub-channels). The geometry cache itself is world-agnostic and lives on the single FCk_Jolt_DebugRenderer,
 * which reconciles into whichever target is active for the current draw session.
 *
 * No Jolt type appears anywhere on this class — a presentation consumer binds a target, flips demand and render
 * mode, and reads counts, without ever seeing JPH. Everything Jolt-shaped lives behind the opaque impl. (The
 * free-function contact recorder above names a `JPH::PhysicsSystem*` as its world key, but only as an opaque
 * forward-declared address; no Jolt header is pulled in and no presentation consumer calls it.)
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

    // The contact replay writes a channel no public setter exposes — FBatchedLine is an engine render type this
    // header deliberately does not name, and the channel has exactly one legitimate writer.
    friend auto ck::jolt::debug_draw::Replay_RecordedContacts(
        const JPH::PhysicsSystem* InPhysicsSystem,
        TArrayView<const TSharedPtr<FCk_Jolt_DebugDrawTarget>> InTargets) -> void;

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

    /*
     * Show/hide one colour class INDEX of the current mode. Component-level only — the class keeps capturing
     * while hidden, so the toggle costs one SetVisibility per affected bucket and unhiding shows current poses,
     * never stale ones.
     *
     * The Highlight and Hover indices are not hideable: a selection the viewer cannot see is indistinguishable
     * from no selection. Asking to hide one is ignored, and Get_IsClassVisible always answers true for them.
     */
    auto
    Set_ClassVisibility(
        uint8 InClassIndex,
        bool InIsVisible) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_IsClassVisible(
        uint8 InClassIndex) const -> bool;

    /*
     * What bodies are coloured BY. Changing it invalidates the retained capture the same way Set_Palette does,
     * because every body has to be re-bucketed through the new mode — including the static and long-asleep ones
     * the incremental pass would otherwise skip.
     */
    auto
    Set_ColorMode(
        ECk_Jolt_DebugDrawColorMode InColorMode) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_ColorMode() const -> ECk_Jolt_DebugDrawColorMode;

    /*
     * The legend of one mode: every class index it can produce, its display name and its colour. Highlight and
     * Hover are present in every mode. ObjectLayer's names come from the layer names the capture published
     * (Set_ObjectLayerNames) and degrade to a bare `Layer N` when none are known.
     */
    auto
    Get_LegendEntries(
        ECk_Jolt_DebugDrawColorMode InColorMode) const -> TArray<FCk_Jolt_DebugDrawLegendEntry>;

    /*
     * Display names of the Jolt object layers, indexed by layer. Published by the capture from the world's
     * collision-layer table (a read-only reverse lookup — it never registers a layer) so the ObjectLayer legend
     * can read `Layer N — <object channel>` instead of a bare number. An empty entry falls back to `Layer N`.
     */
    auto
    Set_ObjectLayerNames(
        TArray<FString> InLayerNames) -> void;

    auto
    Get_ObjectLayerNames() const -> const TArray<FString>&;

    /*
     * Which parts of the Jolt debug-draw vocabulary the next capture emits into this target. Changing them
     * invalidates the retained capture the same way Set_Palette does, so a flag flip takes effect on the very
     * next capture for static and long-asleep bodies too, not only for the ones that happen to be awake.
     */
    auto
    Set_DrawFlags(
        ECk_Jolt_DebugDrawFlags InFlags) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_DrawFlags() const -> ECk_Jolt_DebugDrawFlags;

    auto
    Get_IsDrawFlagSet(
        ECk_Jolt_DebugDrawFlags InFlag) const -> bool;

    // ---- Line + label + External channels ----

    /*
     * Labels the last capture produced, in capture order. Cleared and refilled every capture, so a consumer
     * reading them between captures sees exactly what the current frame drew.
     */
    auto
    Get_Labels() const -> const TArray<FCk_Jolt_DebugDrawLabel>&;

    /// Lines currently live in the target's line component — JPH output for this capture plus every retained
    /// External sub-channel. Zero before the first capture and whenever the target has nothing to draw.
    auto
    Get_NumLines() const -> int32;

    /*
     * The External channel: line work this facility does NOT produce (probe results, a grid, a drag line). Each
     * named sub-channel is owned by its contributor and RETAINED — a capture re-emits it into the line component
     * without clearing it, because a contributor's push rate has nothing to do with the capture rate and a
     * per-capture clear would make anything pushed between captures flicker. Clear_External empties exactly one
     * sub-channel; nothing else ever does.
     */
    auto
    Draw_ExternalLine(
        FName InChannel,
        const FVector& InFrom,
        const FVector& InTo,
        const FLinearColor& InColor) -> void;

    auto
    Draw_ExternalBox(
        FName InChannel,
        const FBox& InLocalBox,
        const FTransform& InTransform,
        const FLinearColor& InColor) -> void;

    auto
    Draw_ExternalSphere(
        FName InChannel,
        const FVector& InCenter,
        float InRadius,
        const FLinearColor& InColor) -> void;

    auto
    Draw_ExternalArrow(
        FName InChannel,
        const FVector& InFrom,
        const FVector& InTo,
        const FLinearColor& InColor) -> void;

    auto
    Clear_External(
        FName InChannel) -> void;

    auto
    Get_NumExternalLines(
        FName InChannel) const -> int32;

    /// Contact lines the last replay wrote into this target. Zero when the target's flags do not ask for
    /// contacts, and zero for a frame in which the solve produced none.
    auto
    Get_NumContactLines() const -> int32;

    /*
     * Select any number of drawn bodies (or characters, keyed through Make_CharacterBodyKey) for the Highlight
     * overlay. Each keeps its normal instance and gains a SECOND one in the Highlight colour class, which follows
     * it every capture; the overlays coexist because each is slotted under its own Make_HighlightKey. Only the
     * overlays of bodies that LEFT the selection are released here — releasing all of them would make every
     * addition to a multi-selection flicker the ones already in it.
     *
     * The FIRST key is the PRIMARY: it alone is sampled (Get_BodySample / Get_CharacterSample) and it alone is
     * asked for its contacts, because both are per-body reads a debugger renders one of.
     */
    auto
    Set_HighlightedBodies(
        TArray<uint64> InBodyKeys) -> FCk_Jolt_DebugDrawTarget&;

    /// The single-selection convenience over Set_HighlightedBodies. An unset key clears the selection.
    auto
    Set_HighlightedBody(
        TOptional<uint64> InBodyKey) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_HighlightedBodies() const -> const TArray<uint64>&;

    /// The PRIMARY selection — the first highlighted key, unset when nothing is selected.
    auto
    Get_HighlightedBody() const -> TOptional<uint64>;

    /*
     * Isolation: while the set is non-empty, the capture draws ONLY the bodies (and characters) it names and
     * RELEASES the instances of everything else — a hidden colour class keeps its instances, an isolated-out body
     * does not exist on screen at all. Both this and Clear_Isolation re-arm the full inactive-body pass, so a
     * static or long-asleep body appears and disappears on the very next capture rather than waiting for the
     * scene to change.
     */
    auto
    Set_IsolatedBodies(
        TSet<uint64> InBodyKeys) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Clear_Isolation() -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_IsolatedBodies() const -> const TSet<uint64>&;

    /*
     * The subdued sibling of the selection, for a hover preview: the same overlay mechanism in its own always-
     * visible class, at half alpha and a smaller swell, so hovering a body can never be mistaken for selecting
     * it. Independent of the highlight — a body may be both, in which case both overlays draw.
     */
    auto
    Set_HoveredBody(
        TOptional<uint64> InBodyKey) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_HoveredBody() const -> TOptional<uint64>;

    /// World-space bounds of the highlighted bodies' NORMAL instances — the UNION over a multi-selection. Unset
    /// when nothing is highlighted, or when none of the selected bodies has been drawn yet.
    auto
    Get_HighlightedBodyBounds() const -> TOptional<FBox>;

    /*
     * Full sample of the PRIMARY selected rigid body, taken by the capture in the same async-safe window it draws
     * from. Unset when nothing is highlighted, when the primary key belongs to a character (which has no
     * JPH::Body), and when the last capture did not draw the body — a sleeping or static body is only drawn on a
     * scene-revision pass. A consumer reads this instead of querying the physics system, which it must never touch.
     */
    auto
    Get_BodySample() const -> TOptional<FCk_Jolt_DebugDraw_BodySample>;

    /// The character twin of Get_BodySample, filled by the capture's character pass for a primary selection that
    /// IS a character. The two are mutually exclusive — a key is one or the other.
    auto
    Get_CharacterSample() const -> TOptional<FCk_Jolt_DebugDraw_CharacterSample>;

    /*
     * Demand switch for the selection's contacts. OFF by default and deliberately so: the query is a
     * NarrowPhaseQuery::CollideShape of the selected body's own shape, which nothing should pay for while no
     * consumer is showing the result. Turning it off empties the list at the next capture.
     */
    auto
    Set_WantsSelectionContacts(
        bool InWantsContacts) -> FCk_Jolt_DebugDrawTarget&;

    auto
    Get_WantsSelectionContacts() const -> bool;

    /// Bodies the PRIMARY selection is touching, refilled every capture while contacts are wanted and a body is
    /// selected. Empty otherwise — including for a character selection, which has no shape query behind it.
    auto
    Get_SelectionContacts() const -> const TArray<FCk_Jolt_DebugDraw_ContactEntry>&;

    /*
     * Bodies the Jolt world created for its OWN internal use — today exactly the debug-drag anchor, a raw JPH
     * kinematic body with no entity behind it. The capture draws none of them and TryPick_Body returns none of
     * them, so an internal body is invisible to every consumer surface rather than merely uninteresting.
     *
     * Pushed in by whoever pumps the capture (the capture processor, the subsystem's own Tick) from
     * FJoltWorld::Get_DebugInternalBodyKeys — the capture itself only ever sees a PhysicsSystem, which does not
     * know which of its bodies the facility owns.
     */
    auto
    Set_InternalBodyKeys(
        TSet<uint64> InBodyKeys) -> void;

    auto
    Get_InternalBodyKeys() const -> const TSet<uint64>&;

    /*
     * What the world is doing, filled by the capture (physics-derived counts) and by whoever pumps it (the two
     * fields the physics system does not own). See FCk_Jolt_DebugDraw_WorldStats for which fields are throttled.
     */
    auto
    Get_WorldStats() const -> const FCk_Jolt_DebugDraw_WorldStats&;

    /// The two world stats the capture cannot reach for itself: both belong to the Jolt world, not to its
    /// PhysicsSystem. Pushed before the capture, so a consumer reading the stats sees one consistent frame.
    auto
    Set_StepStats(
        float InLastStepDurationMs,
        int32 InContactPairsLastStep) -> void;

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

    /// The class INDEX behind every live bucket, in the current mode. A consumer that wants names or colours
    /// reads Get_LegendEntries instead — the indices alone mean nothing without the mode that produced them.
    auto
    Get_BucketColorClasses() const -> TArray<uint8>;

public:
    // Opaque by design: the declaration is public only so the module's own debug-draw translation units can
    // name it. It is incomplete everywhere else, and _Impl itself stays private.
    struct FImpl;

private:
    TPimplPtr<FImpl> _Impl;
};

// --------------------------------------------------------------------------------------------------------------------
