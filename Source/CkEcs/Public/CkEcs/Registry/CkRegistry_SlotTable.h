#pragma once

#include "CoreMinimal.h"
#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry_Handle.h"

#include "entt/entity/registry.hpp"

// --------------------------------------------------------------------------------------------------------------------
// ck::registry_table — process-lifetime slot allocator mapping a stable (slot, generation) handle
// to an entt::basic_registry pointer. Design rationale (phoenix singleton): CkEcs/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_table
{
    using EnttRegistryType = entt::basic_registry<FCk_Entity::IdType, std::allocator<FCk_Entity::IdType>>;

    // Hard upper bound on simultaneously-live registries: Slots is pre-reserved to this count and
    // growth past it would relocate the array and break concurrent worker-thread reads. The
    // configurable soft-warning (_RegistrySlot_WarnThreshold) is the canary; this is the backstop.
    inline constexpr int32 kRegistryTable_MaxSlots = 16384;

    // GAME-THREAD ONLY. The slot is reusable after Free. Returns Unset post-ShutdownTable or at
    // the per-process slot cap; ensures if InRegistry is null.
    CKECS_API auto Allocate(EnttRegistryType* InRegistry) -> FCk_RegistryHandle;

    // GAME-THREAD ONLY. Bumps the slot's generation so old handles stop resolving and nulls the
    // stored pointer — deleting the registry itself remains the caller's job. Idempotent;
    // a stale handle ensures in non-shipping (caller double-free / subsystem double-deinit).
    CKECS_API auto Free(FCk_RegistryHandle InHandle) -> void;

    // STRICT default and the recommended access path: returns nullptr for unset, stale, or
    // out-of-range handles, ensuring in non-shipping. Safe from worker threads INSIDE a scheduler
    // parallel region (no concurrent Allocate/Free there); game-thread by convention outside one.
    CKECS_API auto Resolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*;

    // Silent variant for callers where staleness is a normal condition rather than a bug — the
    // ck::IsValid / FCk_Handle::IsValid path. Same threading contract as Resolve.
    CKECS_API auto TryResolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*;

    // ---- Parallel-region tracking (debug-only side-channel) ----
    //
    // TParallelProcessor marks a registry as actively iterating; mutating registry ops call
    // AssertNotInParallelRegion to ensure against it. No-ops in shipping and for unset/stale handles.
    CKECS_API auto BeginParallelRegion(FCk_RegistryHandle InHandle) -> void;
    CKECS_API auto EndParallelRegion(FCk_RegistryHandle InHandle) -> void;
    CKECS_API auto AssertNotInParallelRegion(FCk_RegistryHandle InHandle, const TCHAR* InOperation) -> void;

    // Query variant for callers that must SKIP optional work rather than assert — e.g. the
    // handle debug-info attach, which is a structural mutation: ParallelFor runs a share of
    // its iterations on the CALLING thread, so a game-thread check alone does not prove the
    // registry is outside a parallel region. Always false in Shipping and for unset/stale handles.
    CKECS_API auto Get_IsInParallelRegion(FCk_RegistryHandle InHandle) -> bool;

    // ---- Dirty-marker version tracking ----
    //
    // Per-fragment-type counter bumped on every mutation of that type and queried by the
    // scheduler's pump pass. Side-channel keyed by registry slot so the FCk_Registry view itself
    // stays trivially copyable. A (handle, hash) never mutated reads 0, as do unset/stale handles.
    CKECS_API auto BumpDirtyMarkerVersion(FCk_RegistryHandle InHandle, uint32 InFragmentTypeHash) -> void;
    CKECS_API auto Get_DirtyMarkerVersion(FCk_RegistryHandle InHandle, uint32 InFragmentTypeHash) -> uint64;

    // Idempotent. Afterwards Free/Resolve/TryResolve are silent no-ops and Allocate refuses.
    // Called from FCkEcsModule::ShutdownModule.
    CKECS_API auto ShutdownTable() -> void;

#if !UE_BUILD_SHIPPING
    // Test hooks: drive generation wrap and the sentinel-dead path without hand-rolling 4 billion
    // alloc/free cycles or unloading a DLL.
    CKECS_API auto Debug_SimulateTableDestruction_DoNotUseInProduction() -> void;
    // Re-arms the sentinel; the bytes were never destructed, so this is just a flag flip.
    CKECS_API auto Debug_ReviveTableAfterSimulatedDestruction_DoNotUseInProduction() -> void;
    CKECS_API auto Debug_ForceSlotGenerationNearWrap_DoNotUseInProduction(int32 SlotIndex) -> void;
#endif
}
