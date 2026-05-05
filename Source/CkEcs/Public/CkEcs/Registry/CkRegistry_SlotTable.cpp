#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkCore/Ensure/CkEnsure.h"

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
// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_table
{
    namespace
    {
        // (Sub-namespace intentionally elided per CkEcs CLAUDE.md: "No anonymous
        // namespaces — use static or internal classes." Symbols below are
        // file-scope `static` instead, with prefixed names to avoid unity
        // collisions.)
    }

    // ----

    struct FRegistryTable_Slot
    {
        EnttRegistryType* Registry   = nullptr;
        // Generation tokens are stored as int32 to match the reflected
        // FCk_RegistryHandle::Generation field (uint32 isn't UHT/BP-
        // compatible). Treated as an opaque token — the only operation
        // is increment-with-skip-zero on alloc/free.
        int32             Generation = 0; // 0 = never-allocated sentinel.
    };

    struct FRegistryTable_State
    {
        TArray<FRegistryTable_Slot> Slots;
        TArray<int32>               FreeList;

        FRegistryTable_State()
        {
            Slots.Reserve(16);
            FreeList.Reserve(16);
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
    static FRegistryTable_State*                        GRegistryTable_StatePtr = nullptr;

    // First-touch initializer. Idempotent. Game-thread only — Allocate/Free
    // are serialized on the game thread, so no double-init race. Returns null
    // if the table has been shut down (ShutdownTable already called).
    static auto Get_RegistryTableState() -> FRegistryTable_State*
    {
        if (GRegistryTable_StateShutdown.load(std::memory_order_acquire))
        { return nullptr; }

        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire))
        {
            if (GRegistryTable_StatePtr == nullptr)
            {
                GRegistryTable_StatePtr = ::new (&GRegistryTable_StateStorage) FRegistryTable_State{};
                GRegistryTable_StateAlive.store(true, std::memory_order_release);
                // Note: we deliberately do NOT register an atexit handler
                // here. The sentinel-flip happens via ShutdownTable(),
                // called explicitly from CkEcs's ShutdownModule.
            }
        }
        return GRegistryTable_StatePtr;
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
        if (GRegistryTable_StatePtr != nullptr)
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

        int32 Index;
        if (State->FreeList.Num() > 0)
        {
            Index = State->FreeList.Pop(EAllowShrinking::No);
        }
        else
        {
            Index = State->Slots.Add(FRegistryTable_Slot{});
        }

        auto& Slot = State->Slots[Index];
        Slot.Registry = InRegistry;
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

        Slot.Registry = nullptr;
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
        // Resolve is read-only and called from many paths (incl. validity
        // checks). Non-shipping check: still expects game thread. Shipping:
        // honor-system, since contention is realistically nil.
#if !UE_BUILD_SHIPPING
        ensureMsgf(IsInGameThread(),
            TEXT("registry_table::TryResolve called from non-game thread"));
#endif
        if (NOT InHandle.IsSet()) { return nullptr; }
        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire)) { return nullptr; }

        auto* State = Get_RegistryTableState();
        if (State == nullptr) { return nullptr; }
        if (NOT State->Slots.IsValidIndex(InHandle.SlotIndex)) { return nullptr; }

        const auto& Slot = State->Slots[InHandle.SlotIndex];
        if (Slot.Generation != InHandle.Generation) { return nullptr; }
        return Slot.Registry;
    }

    auto Resolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*
    {
        // Strict default. Fire ensure in non-shipping if the handle is set
        // but the slot is stale — that's a programming bug worth surfacing.
        // Game-thread enforcement parallels TryResolve.
#if !UE_BUILD_SHIPPING
        ensureMsgf(IsInGameThread(),
            TEXT("registry_table::Resolve called from non-game thread"));
#endif
        if (NOT InHandle.IsSet()) { return nullptr; }
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
        return Slot.Registry;
    }
}
