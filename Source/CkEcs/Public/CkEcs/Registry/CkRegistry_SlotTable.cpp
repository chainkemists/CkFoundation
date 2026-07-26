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
// Threading invariants (design rationale for the module-static phoenix singleton: CkEcs/CLAUDE.md):
//   * Allocate / Free are GAME-THREAD-ONLY — they mutate Slots/FreeList.
//   * Resolve / TryResolve are READ-ONLY and legitimately called from worker threads inside a
//     scheduler parallel region, which is why neither ensures IsInGameThread.
//   * Slots is reserved to kRegistryTable_MaxSlots on init and must never grow past it — a
//     relocation would break concurrent worker reads.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::registry_table
{
    struct FRegistryTable_Slot
    {
        std::atomic<EnttRegistryType*> Registry{nullptr};
        int32             Generation = 0; // 0 = never-allocated sentinel.

#if !UE_BUILD_SHIPPING
        bool                  IsInParallelRegion = false;
#endif
        // Unlike the flag above, kept in all configs — the scheduler's pump pass gates
        // non-debug behaviour on it.
        TMap<uint32, uint64>  DirtyMarkerVersions;
    };

    // Wrap is defined by going through unsigned; 0 stays reserved as the never-allocated sentinel.
    static auto DoAdvance_SlotGeneration(FRegistryTable_Slot& InOutSlot) -> void
    {
        auto NextGen = static_cast<uint32>(InOutSlot.Generation) + 1u;
        if (NextGen == 0u) { NextGen = 1u; }
        InOutSlot.Generation = static_cast<int32>(NextGen);
    }

    struct FRegistryTable_State
    {
        TArray<FRegistryTable_Slot> Slots;
        TArray<int32>               FreeList;
        int32                       SoftWarn_HighestMultipleReported = 0;

        FRegistryTable_State()
        {
            Slots.Reserve(kRegistryTable_MaxSlots);
            FreeList.Reserve(kRegistryTable_MaxSlots);
        }
    };

    // Phoenix-singleton storage: the bytes are deliberately leaked at process exit so that
    // Free/Resolve stay safe no-ops through DLL teardown, whatever the static-destructor order.
    alignas(FRegistryTable_State) static unsigned char  GRegistryTable_StateStorage[sizeof(FRegistryTable_State)];
    static std::atomic<bool>                            GRegistryTable_StateAlive{false};
    // Distinct from `NOT Alive` because pre-init is also not-alive: this lets Allocate refuse
    // explicitly post-shutdown while still allowing first-touch init.
    static std::atomic<bool>                            GRegistryTable_StateShutdown{false};
    static std::atomic<FRegistryTable_State*>           GRegistryTable_StatePtr{nullptr};

    // Game-thread only — Allocate/Free are serialized there, so there is no double-init race.
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
                // Deliberately no atexit handler: the sentinel flip is ShutdownTable(), called
                // explicitly from FCkEcsModule::ShutdownModule.
            }
        }
        return GRegistryTable_StatePtr.load(std::memory_order_acquire);
    }

    // ----

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
        // ~FRegistryTable_State is deliberately never run — a finite, process-lifetime leak.
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

        // All-ones so Allocate's next unsigned increment wraps — the test asserts it skips 0.
        const auto GenerationAllOnes = static_cast<int32>(TNumericLimits<uint32>::Max());
        State->Slots[SlotIndex].Generation = GenerationAllOnes;
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

        CK_ENSURE_IF_NOT(NOT GRegistryTable_StateShutdown.load(std::memory_order_acquire),
            TEXT("registry_table::Allocate called after ShutdownTable — module ordering bug"))
        { return FCk_RegistryHandle::Unset(); }

        auto* State = Get_RegistryTableState();
        CK_ENSURE_IF_NOT(State != nullptr,
            TEXT("registry_table::Allocate: state is null after first-touch init (unexpected)"))
        { return FCk_RegistryHandle::Unset(); }

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
            // Hard cap: growing past the reserve relocates Slots under concurrent worker reads.
            CK_ENSURE_IF_NOT(State->Slots.Num() < kRegistryTable_MaxSlots,
                TEXT("registry_table::Allocate: slot table at hard cap of {} simultaneous registries. "
                     "The soft-warning (Project Settings -> ECS -> Registry Slot Table) should have "
                     "surfaced this earlier — either there's a leak (subsystem failing to free a slot "
                     "in Deinitialize), or the cap genuinely needs raising in kRegistryTable_MaxSlots "
                     "in CkRegistry_SlotTable.cpp."),
                kRegistryTable_MaxSlots)
            { return FCk_RegistryHandle::Unset(); }
            // Emplace, not Add: FRegistryTable_Slot holds a non-movable std::atomic, and the
            // init-time Reserve means no movability is ever required.
            Index = State->Slots.Emplace();

            // Grow path only — a recycled FreeList slot cannot advance the high-water mark.
            // Fires once per ascending multiple, never again for a multiple already crossed.
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
#if !UE_BUILD_SHIPPING
        Slot.IsInParallelRegion = false;
#endif
        Slot.DirtyMarkerVersions.Reset();
        DoAdvance_SlotGeneration(Slot);

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

        // Sentinel-dead: silent no-op — UObject destructors firing during DLL teardown must not
        // crash here just because the table's static is gone.
        if (NOT GRegistryTable_StateAlive.load(std::memory_order_acquire)) { return; }

        auto* State = Get_RegistryTableState();
        if (State == nullptr) { return; }
        if (NOT State->Slots.IsValidIndex(InHandle.SlotIndex)) { return; }

        auto& Slot = State->Slots[InHandle.SlotIndex];

#if !UE_BUILD_SHIPPING
        CK_ENSURE_IF_NOT(NOT Slot.IsInParallelRegion,
            TEXT("registry_table::Free called on slot {} while it is in a parallel region — "
                 "scheduler invariant violated, freeing during worker reads is UB"),
            InHandle.SlotIndex)
        { return; }
#endif

        if (Slot.Generation != InHandle.Generation)
        {
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
        DoAdvance_SlotGeneration(Slot);
        State->FreeList.Push(InHandle.SlotIndex);
    }

    auto TryResolve(FCk_RegistryHandle InHandle) -> EnttRegistryType*
    {
        // No IsInGameThread ensure by design: TParallelProcessor bodies legitimately reach here
        // via FCk_Registry::Has/Get, so one would fire spuriously on every parallel processor.
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
        // Same threading contract as TryResolve.
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
    // Side-channel helpers: debug/scheduler plumbing, so they silently no-op on unset / stale /
    // table-dead handles rather than ensuring.

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
