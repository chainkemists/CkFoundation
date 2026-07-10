#pragma once

#include "CoreMinimal.h"
#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry_Handle.h"

#include "entt/entity/registry.hpp"

// --------------------------------------------------------------------------------------------------------------------
// ck::registry_table — process-lifetime slot allocator that maps a stable
// (slot, generation) handle to an entt::basic_registry pointer. See the
// implementation file for the design rationale (phoenix-singleton).
// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_table
{
    using EnttRegistryType = entt::basic_registry<FCk_Entity::IdType, std::allocator<FCk_Entity::IdType>>;

    // Hard upper bound on simultaneously-live registries. Slots TArray is
    // pre-reserved to this count; growth past it would relocate the array
    // and break concurrent worker-thread reads. Sized to accommodate
    // content-driven sub-registries (e.g. one per CkGrid, one per spatial
    // inventory) on top of the handful of engine-level worlds. The configurable
    // soft-warning in UCk_Ecs_ProjectSettings_UE (_RegistrySlot_WarnThreshold)
    // is the canary for "you crossed a number you didn't expect"; this hard
    // cap is the structural backstop for TArray relocation safety.
    //
    // Exposed via inline constexpr so the settings UPROPERTY and downstream
    // consumers (e.g. CkWatermark) can reference it without indirection.
    inline constexpr int32 kRegistryTable_MaxSlots = 16384;

    // Allocate a slot pointing at InRegistry. Slot is reusable after Free.
    // GAME-THREAD ONLY — enforced via CK_ENSURE_IF_NOT(IsInGameThread()).
    // Asserts if InRegistry is null. Returns Unset if the slot table has
    // already been ShutdownTable'd or if the per-process slot cap is hit.
    CKECS_API auto Allocate(EnttRegistryType* InRegistry) -> FCk_RegistryHandle;

    // Free a slot. Increments the slot's generation so future Resolve calls
    // with the old handle return null. The pointer stored in the slot is
    // nulled (the caller is responsible for actually deleting the registry).
    // GAME-THREAD ONLY — enforced via CK_ENSURE_IF_NOT(IsInGameThread()).
    //
    // Stale-handle Free fires ensure in non-shipping (signals caller-side
    // double-free or subsystem double-deinit). Always idempotent — returns
    // without crashing in shipping or after the table has been destroyed.
    CKECS_API auto Free(FCk_RegistryHandle InHandle) -> void;

    // Resolve a handle to its registry pointer. STRICT default: fires
    // CK_ENSURE_IF_NOT in non-shipping when the handle is set but the slot
    // has been freed/recycled — i.e., the caller is using a stale handle.
    // Returns nullptr on stale handles, unset handles, or out-of-range
    // indices. This is the recommended access path; almost every caller
    // wants the staleness signal in development.
    //
    // Threading: safe to call from worker threads INSIDE a parallel region
    // (TParallelProcessor body, scheduler-driven async work). The
    // scheduler's BeginParallelRegion/End invariant guarantees no
    // concurrent Allocate/Free during that window, and the Slots array is
    // pre-reserved to avoid relocation. Outside parallel regions the
    // caller is game-thread by convention.
    CKECS_API auto Resolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*;

    // Silent variant for callers that legitimately want "is this stale?"
    // semantics without firing an ensure. Used internally by
    // ck::IsValid / FCk_Handle::IsValid where a stale handle is a normal
    // condition (the check itself is the point), not a bug.
    //
    // Same threading contract as Resolve.
    CKECS_API auto TryResolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*;

    // ---- Parallel-region tracking (debug-only side-channel) ----
    //
    // Used by TParallelProcessor to mark a registry as actively iterating
    // in parallel. Mutating registry ops (Add/Remove/Clear/etc.) call
    // AssertNotInParallelRegion to fire an ensure if this flag is set.
    // In shipping these are no-ops. Unset / stale handles are silently
    // ignored.
    CKECS_API auto BeginParallelRegion(FCk_RegistryHandle InHandle) -> void;
    CKECS_API auto EndParallelRegion(FCk_RegistryHandle InHandle) -> void;
    CKECS_API auto AssertNotInParallelRegion(FCk_RegistryHandle InHandle, const TCHAR* InOperation) -> void;

    // Query variant for callers that must SKIP optional work rather than assert — e.g. the
    // handle debug-info attach, which is a structural mutation: ParallelFor runs a share of
    // its iterations on the CALLING thread, so a game-thread check alone does not prove the
    // registry is outside a parallel region. Always false in Shipping (flag compiled out) and
    // for unset/stale handles.
    CKECS_API auto Get_IsInParallelRegion(FCk_RegistryHandle InHandle) -> bool;

    // ---- Dirty-marker version tracking ----
    //
    // Per-fragment-type version counter used by the scheduler's pump pass.
    // Bumped on every mutation (Add/Replace/Remove/Try_Remove/Clear) of
    // that fragment type; queried by the scheduler to detect cascading
    // dirty work. Stored in a side-channel keyed by registry slot so the
    // FCk_Registry view itself stays trivially copyable.
    //
    // Returns 0 for any (handle, hash) that has never been mutated.
    // Unset/stale handles are silent no-ops (Bump) / return 0 (Get).
    CKECS_API auto BumpDirtyMarkerVersion(FCk_RegistryHandle InHandle, uint32 InFragmentTypeHash) -> void;
    CKECS_API auto Get_DirtyMarkerVersion(FCk_RegistryHandle InHandle, uint32 InFragmentTypeHash) -> uint64;

    // Flip the table's "alive" sentinel. After this call, Free()/Resolve()/
    // TryResolve() are silent no-ops; Allocate() is a hard error in
    // non-shipping and returns Unset() in shipping. Called from
    // FCkEcsModule::ShutdownModule. Idempotent.
    CKECS_API auto ShutdownTable() -> void;

#if !UE_BUILD_SHIPPING
    // Test hooks. Public so the unit-test TU and the lifetime-inversion test
    // can drive corner cases (generation wrap, sentinel-dead path) without
    // hand-rolling 4 billion alloc/free cycles or unloading a DLL.
    CKECS_API auto Debug_SimulateTableDestruction_DoNotUseInProduction() -> void;
    // Re-arms the sentinel after Debug_SimulateTableDestruction so subsequent
    // tests in the same process can keep allocating. The bytes were never
    // destructed (phoenix-singleton invariant), so this is just a flag flip.
    CKECS_API auto Debug_ReviveTableAfterSimulatedDestruction_DoNotUseInProduction() -> void;
    CKECS_API auto Debug_ForceSlotGenerationNearWrap_DoNotUseInProduction(int32 SlotIndex) -> void;
#endif
}
