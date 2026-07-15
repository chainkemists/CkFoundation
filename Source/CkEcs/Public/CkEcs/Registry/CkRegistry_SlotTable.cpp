#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include "Containers/Array.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"

#include <atomic>
#include <new>

// --------------------------------------------------------------------------------------------------------------------
// Why module-static (phoenix-singleton) and NOT a UEngineSubsystem:
//
//   1. UObject lifecycle: subsystems are torn down DURING editor shutdown,
//      BEFORE the final GC purge that destroys the long tail of unreachable
//      UObjects. Those late-purged UObjects can hold FCk_Handle fields whose
//      destructors call into the slot table — by which time a subsystem-
//      owned table would already be gone. That's the same lifetime-inversion
//      bug this whole migration was built to fix; we must not reintroduce it
//      one level up.
//   2. Module-static storage outlives the entire UObject lifecycle (DLL
//      unload happens after all UObjects are gone). The phoenix sentinel
//      makes Free()/Resolve()/TryResolve() safe through DLL teardown
//      regardless of caller order.
//   3. Trade-off accepted: the FState bytes are intentionally leaked at
//      process exit. This is a finite, process-lifetime leak — not a
//      growing one — and avoids a class of crash that's far worse than
//      a few KB of unfreed memory at shutdown.
//
// Live Coding caveat:
//   Live-Coding-patching CkEcs.dll while a PIE session is live can leave
//   the slot table in an inconsistent state (new DLL's table is empty;
//   existing handles still point at old DLL's storage). If you Live-Code
//   patch CkEcs, fully restart the editor before the next PIE session.
//   There is no automatic recovery.
//
// Symbol naming:
//   File-scope statics carry the FRegistryTable_* / GRegistryTable_* prefix
//   so they cannot collide with same-named symbols in unrelated CkEcs .cpp
//   files under unity builds. The codebase has been bitten by anonymous-
//   namespace-style collisions in the past — see memory entry
//   feedback_unity_build_anon_namespace.md.
//
// Threading invariants:
//   * Allocate / Free are GAME-THREAD-ONLY. They mutate the Slots/FreeList
//     arrays and are enforced via CK_ENSURE_IF_NOT(IsInGameThread()).
//   * Resolve / TryResolve are READ-ONLY and may be called from worker
//     threads — `TParallelProcessor` bodies are the canonical case. Safety
//     relies on a higher-level invariant maintained by the scheduler:
//     no Allocate/Free occurs during a parallel region. The slot's
//     BeginParallelRegion/EndParallelRegion pair (per-slot side-channel,
//     non-shipping) marks that window so AssertNotInParallelRegion can
//     guard mutation paths. Outside parallel regions, callers are
//     game-thread by convention; an off-thread Resolve outside a parallel
//     region is a programming bug at the caller, not here — surfaceable
//     via tooling, not via a one-size-fits-all ensure that would fire
//     spuriously on every legitimate parallel processor body.
//   * The Slots TArray is reserved to a fixed upper bound on init and
//     must never grow past it (would relocate, breaking concurrent reads).
//     Allocate fires CK_ENSURE_IF_NOT if it ever needs to grow past the
//     reserve.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_table
{
    namespace
    {
        // Sub-namespace intentionally elided: no anonymous namespaces — symbols
        // below are file-scope statics with prefixed names to avoid unity collisions.
    }

    // ----

    struct FRegistryTable_Slot
    {
        // Atomic so the Free-vs-Resolve race window has explicit acquire/release
        // ordering rather than relying on transitive publication via the
        // Generation read. Free release-stores nullptr; Resolve acquire-loads.
        // The parallel-region invariant still applies — no concurrent
        // Allocate/Free with workers — but the atomic is belt-and-braces and
        // also defines behaviour for the cross-tick window where game-thread
        // Allocate/Free could overlap with a worker still draining a prior
        // parallel region.
        std::atomic<EnttRegistryType*> Registry{nullptr};
        // Generation tokens are stored as int32 to match the reflected
        // FCk_RegistryHandle::Generation field (uint32 isn't UHT/BP-
        // compatible). Treated as an opaque token — the only operation
        // is increment-with-skip-zero on alloc/free. Reads on aligned int32
        // are atomic on supported archs; the parallel-region invariant
        // prevents concurrent writes during reads.
        int32             Generation = 0; // 0 = never-allocated sentinel.

        // Side-channel state — non-shipping only. Lives parallel to the
        // registry pointer so FCk_Registry can stay a trivially-copyable
        // (slot, gen) view without bloating per-instance storage.
#if !UE_BUILD_SHIPPING
        bool                  IsInParallelRegion = false;
#endif
        // Dirty-marker version map: queried by the scheduler's pump pass
        // (kept in all configs since it gates non-debug behaviour).
        TMap<uint32, uint64>  DirtyMarkerVersions;
    };

    // kRegistryTable_MaxSlots is defined in CkRegistry_SlotTable.h as inline constexpr
    // so it's visible to downstream consumers (settings, CkWatermark, etc.).

    struct FRegistryTable_State
    {
        TArray<FRegistryTable_Slot> Slots;
        TArray<int32>               FreeList;
        // Highest threshold-multiple already reported via the soft-warning
        // path. Monotonic-high-water-mark: never re-fires for a multiple
        // we've already crossed in this process lifetime, even if the
        // active count drops back and climbs again.
        int32                       SoftWarn_HighestMultipleReported = 0;

        FRegistryTable_State()
        {
            Slots.Reserve(kRegistryTable_MaxSlots);
            FreeList.Reserve(kRegistryTable_MaxSlots);
        }
    };

    // Phoenix-singleton storage. The bytes are leaked at process exit; the
    // sentinel flips so further Free/Resolve calls become safe no-ops even
    // if other static destructors run in parallel. Aligned for FRegistryTable_State.
    alignas(FRegistryTable_State) static unsigned char  GRegistryTable_StateStorage[sizeof(FRegistryTable_State)];
    static std::atomic<bool>                            GRegistryTable_StateAlive{false};
    // Tri-state guard: GRegistryTable_StateShutdown is set once ShutdownTable() has fired.
    // Distinct from `!Alive` because pre-init also has Alive=false. Lets Allocate
    // refuse explicitly post-shutdown while still allowing first-touch init.
    static std::atomic<bool>                            GRegistryTable_StateShutdown{false};
    // Atomic so the publication of the placement-new'd state is explicit
    // rather than relying on transitive publication via GRegistryTable_StateAlive.
    // First-touch init still serializes on the game thread (only Allocate can
    // reach the slow path, because Resolve/TryResolve early-out on !Alive).
    static std::atomic<FRegistryTable_State*>           GRegistryTable_StatePtr{nullptr};

    // First-touch initializer. Idempotent. Game-thread only — Allocate/Free
    // are serialized on the game thread, so no double-init race. Returns null
    // if the table has been shut down (ShutdownTable already called).
    static auto Get_RegistryTableState() -> FRegistryTable_State*
    {
        if (GRegistryTable_StateShutdown.load(std::memory_order_acquire))
        { return nullptr; }

        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire))
        {
            if (GRegistryTable_StatePtr.load(std::memory_order_acquire) == nullptr)
            {
                GRegistryTable_StatePtr.store(
                    ::new (&GRegistryTable_StateStorage) FRegistryTable_State{},
                    std::memory_order_release);
                GRegistryTable_StateAlive.store(true, std::memory_order_release);
                // Note: we deliberately do NOT register an atexit handler
                // here. The sentinel-flip happens via ShutdownTable(),
                // called explicitly from CkEcs's ShutdownModule.
            }
        }
        return GRegistryTable_StatePtr.load(std::memory_order_acquire);
    }

    // ----

    // Fires the soft-warning per the project's configured reporting mode.
    // Called only on ascending-multiple crossings (see Allocate).
    static auto DoFire_RegistrySlotSoftWarning(int32 InActiveCount, int32 InThresholdReached) -> void
    {
        const auto Reporting = UCk_Utils_Ecs_Settings_UE::Get_RegistrySlot_WarnReporting();

        switch (Reporting)
        {
            case ECk_Ecs_RegistrySlot_Reporting::Silent:
            {
                return;
            }
            case ECk_Ecs_RegistrySlot_Reporting::Log:
            {
                ck::ecs::Verbose(
                    TEXT("registry slot table: {} active registries; crossed soft-warning threshold ({}). "
                         "Hard cap is {}. Tune via Project Settings -> ECS -> Registry Slot Table."),
                    InActiveCount, InThresholdReached, kRegistryTable_MaxSlots);
                return;
            }
            case ECk_Ecs_RegistrySlot_Reporting::Warning:
            {
                ck::ecs::Warning(
                    TEXT("registry slot table: {} active registries; crossed soft-warning threshold ({}). "
                         "Hard cap is {}. If this isn't expected for your content scale you may have a leak "
                         "(subsystem failing to free a slot in Deinitialize). Tune via Project Settings -> "
                         "ECS -> Registry Slot Table."),
                    InActiveCount, InThresholdReached, kRegistryTable_MaxSlots);
                return;
            }
            case ECk_Ecs_RegistrySlot_Reporting::Ensure:
            {
                CK_ENSURE_IF_NOT(false,
                    TEXT("registry slot table: {} active registries; crossed soft-warning threshold ({}). "
                         "Hard cap is {}. If this is expected for your content scale, raise "
                         "_RegistrySlot_WarnThreshold or set _RegistrySlot_WarnReporting to Warning/Log/Silent "
                         "in Project Settings -> ECS -> Registry Slot Table."),
                    InActiveCount, InThresholdReached, kRegistryTable_MaxSlots)
                { return; }
                return;
            }
        }
    }

    // ----

    auto ShutdownTable() -> void
    {
        // Flip sentinel; future Free/Resolve calls become silent no-ops. We
        // deliberately do NOT run ~FRegistryTable_State (that's a leak, but a
        // finite, process-lifetime one — see comment block at top).
        GRegistryTable_StateAlive.store(false, std::memory_order_release);
        GRegistryTable_StateShutdown.store(true, std::memory_order_release);
    }

#if !UE_BUILD_SHIPPING
    auto Debug_SimulateTableDestruction_DoNotUseInProduction() -> void
    {
        ShutdownTable();
    }

    auto Debug_ReviveTableAfterSimulatedDestruction_DoNotUseInProduction() -> void
    {
        // The bytes were never destructed; just re-arm the flags.
        if (GRegistryTable_StatePtr.load(std::memory_order_acquire) != nullptr)
        {
            GRegistryTable_StateShutdown.store(false, std::memory_order_release);
            GRegistryTable_StateAlive.store(true, std::memory_order_release);
        }
    }

    auto Debug_ForceSlotGenerationNearWrap_DoNotUseInProduction(int32 SlotIndex) -> void
    {
        auto* State = Get_RegistryTableState();
        CK_ENSURE_IF_NOT(State != nullptr,
            TEXT("Debug_ForceSlotGenerationNearWrap: state is null (shutdown?)"))
        { return; }
        CK_ENSURE_IF_NOT(State->Slots.IsValidIndex(SlotIndex),
            TEXT("Debug_ForceSlotGenerationNearWrap: SlotIndex {} out of range (table has {} slots)"),
            SlotIndex, State->Slots.Num())
        { return; }

        // Set Generation bits to the all-ones pattern so the next single
        // unsigned increment (in Allocate) wraps to 0; we want to verify
        // the post-wrap value skips 0 (the never-allocated sentinel).
        const auto AtMax = static_cast<int32>(TNumericLimits<uint32>::Max());
        State->Slots[SlotIndex].Generation = AtMax;
    }
#endif

    // ----

    auto Allocate(EnttRegistryType* InRegistry) -> FCk_RegistryHandle
    {
        CK_ENSURE_IF_NOT(IsInGameThread(),
            TEXT("registry_table::Allocate called off the game thread — slot-table mutation is not thread-safe"))
        { return FCk_RegistryHandle::Unset(); }

        CK_ENSURE_IF_NOT(InRegistry != nullptr,
            TEXT("registry_table::Allocate: InRegistry must not be null"))
        { return FCk_RegistryHandle::Unset(); }

        // Allocate after ShutdownTable is a hard error in non-shipping (signals
        // a module-ordering bug — somebody is creating a registry mid-tear-
        // down). In shipping we still refuse, returning Unset for safety.
        CK_ENSURE_IF_NOT(NOT GRegistryTable_StateShutdown.load(std::memory_order_acquire),
            TEXT("registry_table::Allocate called after ShutdownTable — module ordering bug"))
        { return FCk_RegistryHandle::Unset(); }

        auto* State = Get_RegistryTableState();
        CK_ENSURE_IF_NOT(State != nullptr,
            TEXT("registry_table::Allocate: state is null after first-touch init (unexpected)"))
        { return FCk_RegistryHandle::Unset(); }

        // Defense in depth: even on the game thread, allocating mid-parallel-region
        // would mutate Slots concurrently with worker reads. The scheduler upholds
        // the invariant in practice; this fires if it ever doesn't.
#if !UE_BUILD_SHIPPING
        for (auto SlotIdx = 0; SlotIdx < State->Slots.Num(); ++SlotIdx)
        {
            CK_ENSURE_IF_NOT(NOT State->Slots[SlotIdx].IsInParallelRegion,
                TEXT("registry_table::Allocate called while slot {} is in a parallel region — "
                     "scheduler invariant violated, mutating slot table during worker reads is UB"),
                SlotIdx)
            { return FCk_RegistryHandle::Unset(); }
        }
#endif

        int32 Index;
        if (State->FreeList.Num() > 0)
        {
            Index = State->FreeList.Pop(EAllowShrinking::No);
        }
        else
        {
            // Hard-cap growth to keep Slots' data pointer stable while worker
            // threads may be reading it via Resolve/TryResolve. A relocation
            // here would race with concurrent reads — see threading
            // invariants comment at top of file.
            CK_ENSURE_IF_NOT(State->Slots.Num() < kRegistryTable_MaxSlots,
                TEXT("registry_table::Allocate: slot table at hard cap of {} simultaneous registries. "
                     "The soft-warning (Project Settings -> ECS -> Registry Slot Table) should have "
                     "surfaced this earlier — either there's a leak (subsystem failing to free a slot "
                     "in Deinitialize), or the cap genuinely needs raising in kRegistryTable_MaxSlots "
                     "in CkRegistry_SlotTable.cpp."),
                kRegistryTable_MaxSlots)
            { return FCk_RegistryHandle::Unset(); }
            // Emplace (not Add) because FRegistryTable_Slot holds a non-movable
            // std::atomic and TArray::Add(T&&) would require move-construction.
            // Reserve(kRegistryTable_MaxSlots) at init guarantees we never relocate,
            // so no movability is required.
            Index = State->Slots.Emplace();

            // Soft-warning check: only on the grow path, because that's the only path
            // that can advance the monotonic high-water mark. Recycled-from-FreeList
            // allocations don't change Slots.Num(). Fires once per ascending multiple
            // of the project-configured threshold.
            const auto Threshold = UCk_Utils_Ecs_Settings_UE::Get_RegistrySlot_WarnThreshold();
            if (Threshold > 0)
            {
                const auto NewCount = State->Slots.Num();
                const auto NewMultiple = NewCount / Threshold;
                if (NewMultiple > State->SoftWarn_HighestMultipleReported)
                {
                    State->SoftWarn_HighestMultipleReported = NewMultiple;
                    DoFire_RegistrySlotSoftWarning(NewCount, NewMultiple * Threshold);
                }
            }
        }

        auto& Slot = State->Slots[Index];
        Slot.Registry.store(InRegistry, std::memory_order_release);
        // Reset side-channel state on allocation — a recycled slot must not
        // inherit dirty markers / parallel-region flags from the prior
        // generation.
#if !UE_BUILD_SHIPPING
        Slot.IsInParallelRegion = false;
#endif
        Slot.DirtyMarkerVersions.Reset();
        // Increment via unsigned to make wrap defined behaviour, then write
        // back. Skip 0 (the "never-allocated" sentinel) on wrap.
        {
            auto NextGen = static_cast<uint32>(Slot.Generation) + 1u;
            if (NextGen == 0u) { NextGen = 1u; }
            Slot.Generation = static_cast<int32>(NextGen);
        }

        FCk_RegistryHandle Handle;
        Handle.SlotIndex  = Index;
        Handle.Generation = Slot.Generation;
        return Handle;
    }

    auto Free(FCk_RegistryHandle InHandle) -> void
    {
        CK_ENSURE_IF_NOT(IsInGameThread(),
            TEXT("registry_table::Free called off the game thread — slot-table mutation is not thread-safe"))
        { return; }

        if (NOT InHandle.IsSet()) { return; }

        // Sentinel-dead: silent no-op. This is the *whole point* of the phoenix
        // singleton — UObject destructors firing during DLL teardown must not
        // crash here just because the slot table's static is gone.
        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire)) { return; }

        auto* State = Get_RegistryTableState();
        if (State == nullptr) { return; }
        if (NOT State->Slots.IsValidIndex(InHandle.SlotIndex)) { return; }

        auto& Slot = State->Slots[InHandle.SlotIndex];

        // Defense in depth: freeing a slot mid-parallel-region nulls the
        // Registry pointer worker threads are concurrently reading. Atomic
        // store on Registry makes the read race well-defined, but the slot
        // also has non-atomic side state; surface the contract violation
        // rather than relying on the atomic alone.
#if !UE_BUILD_SHIPPING
        CK_ENSURE_IF_NOT(NOT Slot.IsInParallelRegion,
            TEXT("registry_table::Free called on slot {} while it is in a parallel region — "
                 "scheduler invariant violated, freeing during worker reads is UB"),
            InHandle.SlotIndex)
        { return; }
#endif

        if (Slot.Generation != InHandle.Generation)
        {
            // Stale Free signals subsystem-level double-deinit — fire ensure
            // in non-shipping so we surface the bug; remain idempotent in
            // shipping for safety.
            CK_ENSURE_IF_NOT(false,
                TEXT("registry_table::Free called with stale handle (slot {} expected gen {} but is at {}). "
                     "Likely subsystem double-deinit or caller-side double-free."),
                InHandle.SlotIndex, InHandle.Generation, Slot.Generation)
            { return; }
        }

        Slot.Registry.store(nullptr, std::memory_order_release);
#if !UE_BUILD_SHIPPING
        Slot.IsInParallelRegion = false;
#endif
        Slot.DirtyMarkerVersions.Reset();
        // Increment via unsigned so wrap is defined; skip 0 on wrap. Mirrors
        // Allocate so wrap-skip-zero is symmetric on both halves of the cycle.
        {
            auto NextGen = static_cast<uint32>(Slot.Generation) + 1u;
            if (NextGen == 0u) { NextGen = 1u; }
            Slot.Generation = static_cast<int32>(NextGen);
        }
        State->FreeList.Push(InHandle.SlotIndex);
    }

    auto TryResolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*
    {
        // Read-only. Safe to call from worker threads inside a parallel
        // region (no concurrent Allocate/Free per the scheduler's parallel-
        // region invariant). Outside parallel regions the caller is game-
        // thread by convention — see the threading-invariants comment at
        // the top of this file. We deliberately do NOT enforce
        // IsInGameThread here: TParallelProcessor bodies legitimately call
        // through to Resolve via FCk_Registry::Has/Get, and a one-size-fits-
        // all ensure would fire spuriously on every parallel processor.
        if (NOT InHandle.IsSet()) { return nullptr; }
        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire)) { return nullptr; }

        auto* State = Get_RegistryTableState();
        if (State == nullptr) { return nullptr; }
        if (NOT State->Slots.IsValidIndex(InHandle.SlotIndex)) { return nullptr; }

        const auto& Slot = State->Slots[InHandle.SlotIndex];
        if (Slot.Generation != InHandle.Generation) { return nullptr; }
        return Slot.Registry.load(std::memory_order_acquire);
    }

    auto Resolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*
    {
        // Strict default. Fire ensure in non-shipping if the handle is set
        // but the slot is stale — that's a programming bug worth surfacing.
        // Same threading contract as TryResolve.
        //
        // Unset handles also fire ensure (once per call site) — they're
        // almost always a default-constructed FCk_Registry / FCk_Handle
        // member that the owner forgot to bind to a slot. The pre-migration
        // FCk_Registry default ctor auto-allocated a private registry and
        // hid these sites; surface them now so they can't sit silent.
        if (NOT InHandle.IsSet())
        {
            CK_ENSURE_IF_NOT(false,
                TEXT("registry_table::Resolve called with an unset handle. ")
                TEXT("Likely a default-constructed FCk_Registry / FCk_Handle being used before it has been bound to a registry slot. ")
                TEXT("Pre-migration the default ctor auto-allocated; post-migration the binding is the caller's responsibility."))
            { return nullptr; }
        }
        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire)) { return nullptr; }

        auto* State = Get_RegistryTableState();
        if (State == nullptr) { return nullptr; }

        if (NOT State->Slots.IsValidIndex(InHandle.SlotIndex))
        {
            CK_ENSURE_IF_NOT(false,
                TEXT("Stale FCk_RegistryHandle: SlotIndex {} out of range (table has {} slots)"),
                InHandle.SlotIndex, State->Slots.Num())
            { return nullptr; }
        }

        const auto& Slot = State->Slots[InHandle.SlotIndex];
        if (Slot.Generation != InHandle.Generation)
        {
            CK_ENSURE_IF_NOT(false,
                TEXT("Stale FCk_RegistryHandle: slot {} expected gen {} but slot is at gen {}"),
                InHandle.SlotIndex, InHandle.Generation, Slot.Generation)
            { return nullptr; }
        }
        return Slot.Registry.load(std::memory_order_acquire);
    }

    // ----
    // Side-channel helpers. All silently no-op on unset / stale / table-dead;
    // they are debug/scheduler plumbing, not load-bearing — firing ensures
    // here would just produce noise.

    static auto Get_LiveSlot(FCk_RegistryHandle InHandle) -> FRegistryTable_Slot*
    {
        if (NOT InHandle.IsSet()) { return nullptr; }
        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire)) { return nullptr; }

        auto* State = Get_RegistryTableState();
        if (State == nullptr) { return nullptr; }
        if (NOT State->Slots.IsValidIndex(InHandle.SlotIndex)) { return nullptr; }

        auto& Slot = State->Slots[InHandle.SlotIndex];
        if (Slot.Generation != InHandle.Generation) { return nullptr; }
        return &Slot;
    }

    auto BeginParallelRegion([[maybe_unused]] FCk_RegistryHandle InHandle) -> void
    {
#if !UE_BUILD_SHIPPING
        if (auto* Slot = Get_LiveSlot(InHandle))
        {
            Slot->IsInParallelRegion = true;
        }
#endif
    }

    auto EndParallelRegion([[maybe_unused]] FCk_RegistryHandle InHandle) -> void
    {
#if !UE_BUILD_SHIPPING
        if (auto* Slot = Get_LiveSlot(InHandle))
        {
            Slot->IsInParallelRegion = false;
        }
#endif
    }

    auto AssertNotInParallelRegion([[maybe_unused]] FCk_RegistryHandle InHandle, [[maybe_unused]] const TCHAR* InOperation) -> void
    {
#if !UE_BUILD_SHIPPING
        auto* Slot = Get_LiveSlot(InHandle);
        if (Slot == nullptr) { return; }

        CK_ENSURE_IF_NOT(NOT Slot->IsInParallelRegion,
            TEXT("THREAD-SAFETY VIOLATION: [{}] called during parallel processor execution. ")
            TEXT("Use InHandle.DeferAdd<>() / DeferRemove<>() instead."), InOperation)
        { return; }
#endif
    }

    auto Get_IsInParallelRegion([[maybe_unused]] FCk_RegistryHandle InHandle) -> bool
    {
#if !UE_BUILD_SHIPPING
        if (const auto* Slot = Get_LiveSlot(InHandle))
        { return Slot->IsInParallelRegion; }
#endif

        return false;
    }

    auto BumpDirtyMarkerVersion(FCk_RegistryHandle InHandle, uint32 InFragmentTypeHash) -> void
    {
        if (auto* Slot = Get_LiveSlot(InHandle))
        {
            ++Slot->DirtyMarkerVersions.FindOrAdd(InFragmentTypeHash);
        }
    }

    auto Get_DirtyMarkerVersion(FCk_RegistryHandle InHandle, uint32 InFragmentTypeHash) -> uint64
    {
        if (auto* Slot = Get_LiveSlot(InHandle))
        {
            const auto* Found = Slot->DirtyMarkerVersions.Find(InFragmentTypeHash);
            return Found != nullptr ? *Found : uint64{0};
        }
        return uint64{0};
    }
}
