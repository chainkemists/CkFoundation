#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/World/CkJoltWorld.h"

#include "CkJolt_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_Jolt_DebugDrawTarget;
class CkContactListener;
class CkBodyActivationListener;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class TempAllocatorImpl;
    class JobSystem;
    class ObjectLayerPairFilterImpl;
    class PhysicsSystem;
    class BodyInterface;

    class Vec3;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Jolt")
class CKJOLT_API UCk_Jolt_Subsystem : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Jolt_Subsystem);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Tick(
        float InDeltaTime) -> void override;

    auto
    Deinitialize() -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

private:
    TPimplPtr<JPH::TempAllocatorImpl> _TempAllocator;
    JPH::JobSystem* _JobSystem = nullptr;
    TUniquePtr<ck::jolt::FCk_Jolt_CollisionLayerTable> _LayerTable;
    TUniquePtr<ck::jolt::FCk_Jolt_BroadPhaseLayerInterface_Table> _BroadPhaseLayerInterface;
    TUniquePtr<ck::jolt::FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table> _ObjectVsBroadPhaseLayerFilter;
    TUniquePtr<ck::jolt::FCk_Jolt_ObjectLayerPairFilter_Table> _ObjectVsObjectFilter;
    TSharedPtr<JPH::PhysicsSystem> _PhysicsSystem;

    TPimplPtr<CkBodyActivationListener> _BodyActivationListener;
    TPimplPtr<CkContactListener> _ContactListener;

    int32 _CollisionSteps = 1;

    bool _ParallelPhysicsEnabled = false;
    int32 _PhysicsThreadCount = 0;
    bool _AsyncPhysicsUpdate = false;

    // Published to the ECS registry as a TSharedPtr context; holds non-owning pointers into the members above.
    TSharedPtr<ck::FJoltWorld> _JoltWorld;

    // Consumer-installed debug-draw opt-in. No gate installed = no debug draw.
    TFunction<bool()> _DebugDrawGate;

#if JPH_DEBUG_RENDERER
    // The in-world draw the subsystem Tick owns. Consumer-registered targets are separate and are pumped by
    // FProcessor_JoltDebugDraw_Capture, never from here.
    TSharedPtr<FCk_Jolt_DebugDrawTarget> _DefaultDebugDrawTarget;

    TArray<TWeakPtr<FCk_Jolt_DebugDrawTarget>> _RegisteredDebugDrawTargets;
#endif

public:
    auto
    Get_PhysicsSystem() const -> TWeakPtr<JPH::PhysicsSystem>;

    /// Register a consumer of drained contact events (e.g. CkSpatialQuery's probe-overlap
    /// translation). Routers fire on the game thread, in registration order. Names must be unique.
    auto
    RegisterContactRouter(
        FName InName,
        ck::FCk_Jolt_ContactEventRouter InRouter) -> void;

    auto
    UnregisterContactRouter(
        FName InName) -> void;

    /// Register a named vote for "something in this world wants Persisted contact events".
    /// Votes are re-asked and OR-ed once per frame during the contact drain and the result is
    /// published to the Jolt contact listener, which skips generating Persisted events entirely
    /// when nobody votes yes. Re-asking (rather than refcounting) means a consumer cannot leak a
    /// stale interest, and disconnecting opts out on its own. Names must be unique.
    auto
    Register_PersistedContactInterestProvider(
        FName InName,
        TFunction<bool()> InProvider) -> void;

    auto
    Unregister_PersistedContactInterestProvider(
        FName InName) -> void;

    /// Signature-driven layer table (one JPH::ObjectLayer per unique collision signature).
    /// Registration is game-thread only; Jolt filters read it lock-free from workers.
    auto
    Get_LayerTable() -> ck::jolt::FCk_Jolt_CollisionLayerTable&;

    /// Runs PhysicsSystem::OptimizeBroadPhase on the game thread immediately before the next
    /// Update (never while an async update is in flight). Callers batch: request once after
    /// bulk add/remove, not per body.
    auto
    Request_OptimizeBroadPhaseBeforeNextUpdate() -> void;

    auto
    Set_DebugDrawGate(
        TFunction<bool()> InGate) -> void;

#if JPH_DEBUG_RENDERER
    /// Register a consumer-owned debug-draw target. Held WEAKLY — the consumer's TSharedRef is the lifetime.
    /// Game thread only. FProcessor_JoltDebugDraw_Capture pumps every registered target that is desired.
    auto
    Register_DebugDrawTarget(
        const TSharedRef<FCk_Jolt_DebugDrawTarget>& InTarget) -> void;

    auto
    Unregister_DebugDrawTarget(
        const TSharedRef<FCk_Jolt_DebugDrawTarget>& InTarget) -> void;

    /// Live registered targets whose consumer currently wants them drawn; also compacts dead registrations.
    auto
    Get_DemandingDebugDrawTargets(
        TArray<TSharedPtr<FCk_Jolt_DebugDrawTarget>>& OutTargets) -> void;

    /// The subsystem's OWN in-world target — never registered, captured from Tick rather than by the capture
    /// processor. Exposed because the contact replay has exactly one consumer per world (the capture
    /// processor), so this target has to be fed there or ck.Jolt.DebugDraw.Contacts could never draw anything.
    auto
    Get_DefaultDebugDrawTarget() const -> TSharedPtr<FCk_Jolt_DebugDrawTarget>;
#endif

    /*
     * Debug pause of the JOLT world alone — the engine's own pause stops the whole game, this one freezes
     * physics so a debugger can inspect it. Request_StepOnce permits exactly one step and then re-pauses; it
     * is ignored while the world is not debug-paused.
     */
    auto
    Request_SetDebugPaused(
        bool InIsDebugPaused) -> void;

    auto
    Request_StepOnce() -> void;

    auto
    Get_IsDebugPaused() const -> bool;

    /// Wall time the last frame's Jolt solve took, in milliseconds. Zero before the first step, and unchanged
    /// while the world is paused — a paused world's last step time is still the last step's.
    auto
    Get_LastStepDurationMs() const -> float;

    /// Contact pairs (added + persisted) the LAST sub-step produced. Reset per step, so it is a rate, not a total.
    auto
    Get_ContactPairsLastStep() const -> int32;

#if !UE_BUILD_SHIPPING
    /*
     * Debug drag (P6-D47) — DEV-ONLY and SIM-MUTATING, unlike every other debug surface on this subsystem, and
     * therefore compiled out of Shipping entirely. The requests queue and are applied by
     * FProcessor_JoltDebugDrag_Apply before the next step; only DYNAMIC bodies can be dragged (anything else is
     * dropped at Verbose). AUTHORITY ONLY: a drag on a client moves a body the server will correct on the next
     * replication.
     *
     * InBodyKey is the debug-draw body key — the same one TryPick_Body returns, so a click picks and drags the
     * same body without the caller converting anything. A key outside the rigid-body keyspace (a character, an
     * overlay) is refused rather than truncated.
     */
    auto
    Request_BeginDrag(
        uint64 InBodyKey,
        const FVector& InWorldGrabPoint) -> void;

    auto
    Request_UpdateDrag(
        const FVector& InWorldTargetPoint) -> void;

    auto
    Request_EndDrag() -> void;

    auto
    Get_IsDragging() const -> bool;

    /// Where the drag's spring is attached and where it is pulling to, for a consumer that draws the drag line.
    /// Unset while nothing is being dragged; both points are cached, so this never reads a JPH body.
    auto
    Get_DragState() const -> TOptional<ck::jolt::FCk_Jolt_DebugDragState>;
#endif

    /// Debug-draw keys of the bodies the facility owns (the drag anchor). Every consumer surface must skip them —
    /// the capture and TryPick_Body already do.
    auto
    Get_DebugInternalBodyKeys() const -> const TSet<uint64>&;

    /// Bumps the Jolt world's static-scene change token. Called by every static-body add/remove funnel.
    auto
    Request_NoteStaticSceneChanged() -> void;

    /// Bumps the Jolt world's body-removed change token. Called by every funnel that destroys a body, whatever
    /// its motion type — the debug draw's sweep for destroyed SLEEPING bodies is gated on it.
    auto
    Request_NoteBodyRemoved() -> void;

    /// The Jolt world's static-scene change token. WORLD-WIDE, never region-scoped: it moves for any static
    /// body anywhere, so a consumer comparing it across time learns only that SOMETHING changed. Zero when
    /// there is no world.
    auto
    Get_StaticSceneRevision() const -> uint64;

    /// This world's ECS transient entity — the registry root a Jolt body's user data is resolved against,
    /// and therefore the only way a Jolt-side consumer can name the entity a body was registered for.
    /// INVALID outside a live ECS world, which every consumer must read as "no attribution available"
    /// rather than as a failure: a preview or transient world legitimately has none.
    auto
    Get_TransientEntity() const -> FCk_Handle;

    CK_PROPERTY_GET(_ParallelPhysicsEnabled);
    CK_PROPERTY_GET(_PhysicsThreadCount);
    CK_PROPERTY_GET(_AsyncPhysicsUpdate);
};

// --------------------------------------------------------------------------------------------------------------------
