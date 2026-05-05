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

    // Allocate a slot pointing at InRegistry. Slot is reusable after Free.
    // Game-thread only — fires ensure in non-shipping if called off-thread.
    // Asserts if InRegistry is null.
    CKECS_API auto Allocate(EnttRegistryType* InRegistry) -> FCk_RegistryHandle;

    // Free a slot. Increments the slot's generation so future Resolve calls
    // with the old handle return null. The pointer stored in the slot is
    // nulled (the caller is responsible for actually deleting the registry).
    // Game-thread only.
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
    CKECS_API auto Resolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*;

    // Silent variant for callers that legitimately want "is this stale?"
    // semantics without firing an ensure. Used internally by
    // ck::IsValid / FCk_Handle::IsValid where a stale handle is a normal
    // condition (the check itself is the point), not a bug.
    CKECS_API auto TryResolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*;

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
    CKECS_API auto Debug_ForceSlotGenerationNearWrap_DoNotUseInProduction(int32 SlotIndex) -> void;
#endif
}
