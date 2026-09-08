#include "CkDynamic/CkDynamic_ScriptQueryProcessor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Utils.h"        // Get_StorageId
#include "CkDynamic/CkDynamic_Fragment.h"     // ck::FFragment_DynamicFragment_Data
#include "CkDynamic/CkDynamic_Fragment_Data.h"// ECk_DestroyFilter

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_Script.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <Misc/ScopeExit.h>
#include <HAL/PlatformTime.h>
#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_script_query_processor
{
    auto
        PassesDestroyFilter(
            const FCk_Handle& InHandle,
            ECk_DestroyFilter InFilter)
        -> bool
    {
        switch (InFilter)
        {
            case ECk_DestroyFilter::None:
            {
                return true;
            }
            case ECk_DestroyFilter::IgnorePendingKill:
            {
                return NOT (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
                            UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Teardown) ||
                            UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Destroyed));
            }
            case ECk_DestroyFilter::Teardown:
            {
                return UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Teardown);
            }
            default:
            {
                CK_INVALID_ENUM(InFilter);
                return false;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_ScriptQueryHosted::
        FProcessor_ScriptQueryHosted(
            const RegistryType& InRegistry,
            UClass* InDevClass,
            UClass* InDriverClass)
        : _Registry(InRegistry)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InDevClass),
            TEXT("FProcessor_ScriptQueryHosted constructed with a null dev UClass"))
        { _Disabled = true; return; }

        // The driver is a SUBCLASS of the dev class, so ONE instance carries both surfaces.
        auto* InstanceClass = ck::IsValid(InDriverClass) ? InDriverClass : InDevClass;
        auto* Instance = NewObject<UCk_Processor_Script_Base_UE>(GetTransientPackage(), InstanceClass);
        _Instance = TStrongObjectPtr<UCk_Processor_Script_Base_UE>{Instance};

        CK_ENSURE_IF_NOT(ck::IsValid(_Instance.Get()),
            TEXT("FProcessor_ScriptQueryHosted failed to construct an instance for [{}]"), InstanceClass)
        { _Disabled = true; return; }

        // Stat row keyed by the DEV class name — that is the name authors know and RunAfter references use.
        _TickStatId = CK_CREATE_DYNAMIC_STAT_ID(STATGROUP_CkProcessors,
            ck::Format_UE(TEXT("script::{}"), InDevClass->GetName()));

        const auto TransientHandle = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InRegistry);
        _Instance->Set_Handle(TransientHandle);

        _Instance->Configure(_Query);

        const auto AdmissionSucceeded = NOT _Query._AdmissionFailed;
        CK_ENSURE_IF_NOT(AdmissionSucceeded,
            TEXT("Script processor [{}] query admission failed. Disabling it instead of dispatching a partial query."),
            InDevClass)
        { _Disabled = true; return; }

        const auto QueryContext = InDevClass->GetPathName();
        if (NOT ck::dynamic::Validate_ScriptQuerySlots(_Query, *QueryContext))
        { _Disabled = true; return; }

        const auto QueryIsValid = (NOT _Query._Slots.IsEmpty()) || _Query._NoEntities;
        CK_ENSURE_IF_NOT(QueryIsValid,
            TEXT("Script processor [{}] declared an empty query and did not call NoEntities(). Disabling it."), InDevClass)
        { _Disabled = true; return; }

        if (NOT _Query._NoEntities)
        {
            const auto HasNonExcludeSlot = _Query._Slots.ContainsByPredicate(
                [](const FCk_ScriptQuerySlot& InSlot) { return InSlot._Access != ECk_ScriptQueryAccess::Exclude; });

            CK_ENSURE_IF_NOT(HasNonExcludeSlot,
                TEXT("Script processor [{}] declared only Exclude slots — there is no pool to drive the join. "
                     "Add a ReadOnly/ReadWrite/Require slot or call NoEntities(). Disabling it."), InDevClass)
            { _Disabled = true; return; }
        }

        _QuerySlotDirtyMarkerHashes.Reserve(_Query._Slots.Num());
        for (const auto& Slot : _Query._Slots)
        {
            _QuerySlotDirtyMarkerHashes.Add(
                UCk_Utils_DynamicFragment_UE::Get_DirtyMarkerHash(Slot._StructType.Get()));
        }

        _Instance->BeginPlay();
    }

    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_ScriptQueryHosted::
        ~FProcessor_ScriptQueryHosted()
    {
        // EndPlay only mirrors a BeginPlay that actually ran — the ctor calls it last, and only when not disabled.
        if (NOT _Disabled && ck::IsValid(_Instance.Get()))
        {
            _Instance->EndPlay();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _LastTickVisitedCount = 0;

        if (_Disabled)
        { return; }

        FScopeCycleCounter TickStatCounter{_TickStatId};

        if (ck::Is_NOT_Valid(_Instance.Get()))
        { return; }

        // During registry teardown the scheduler keeps ticking for a few frames after the transient entity is
        // destroyed; every registry access below would ensure on the dead handle. C++ processors see an empty view.
        if (ck::Is_NOT_Valid(_Instance->Get_Handle()))
        { return; }

        if (_Query._NoEntities)
        {
            _BatchState._Slots.Reset();
            _BatchState._Entities.Reset();
            _BatchState._AnyHandle = _Instance->Get_Handle();

            constexpr auto VisitedCountUnknown = -1;
            _LastTickVisitedCount = VisitedCountUnknown;

            DoDispatchBatch(InDeltaT);
            return;
        }

        if (NOT DoResolveAndJoin())
        { return; }   // empty-join early-out — no VM call

        _LastTickVisitedCount = _BatchState._Entities.Num();

        DoDispatchBatch(InDeltaT);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        Pump()
        -> int32
    {
        Tick(TimeType::ZeroSecond());
        return _LastTickVisitedCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoesStructuralCacheNeedRebuild(
        const FCk_Registry& InRegistry,
        const entt::sparse_set& InDriveStorage) const
    -> bool
    {
        if (_StructuralCacheDriveStorage != &InDriveStorage ||
            _StructuralCacheSlots.Num() != _BatchState._Slots.Num())
        { return true; }

        for (int32 Index = 0; Index < _BatchState._Slots.Num(); ++Index)
        {
            const auto& Slot = _BatchState._Slots[Index];
            const auto& CachedSlot = _StructuralCacheSlots[Index];
            const auto SlotIsUnchanged =
                CachedSlot._Type == Slot._Type &&
                CachedSlot._StorageIdentity == Slot._Storage &&
                CachedSlot._Access == Slot._Access &&
                CachedSlot._StorageSize == static_cast<int32>(Slot._Storage->size()) &&
                CachedSlot._MembershipVersion == InRegistry.Get_DirtyMarkerVersion(_QuerySlotDirtyMarkerHashes[Index]);
            if (NOT SlotIsUnchanged)
            { return true; }
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        SnapshotStructuralCacheState(
        const FCk_Registry& InRegistry,
        const entt::sparse_set& InDriveStorage)
    -> void
    {
        _StructuralCacheDriveStorage = &InDriveStorage;
        _StructuralCacheSlots.Reset();
        _StructuralCacheSlots.Reserve(_BatchState._Slots.Num());

        for (int32 Index = 0; Index < _BatchState._Slots.Num(); ++Index)
        {
            const auto& Slot = _BatchState._Slots[Index];
            _StructuralCacheSlots.Add({
                ._Type = Slot._Type,
                ._StorageIdentity = Slot._Storage,
                ._Access = Slot._Access,
                ._StorageSize = static_cast<int32>(Slot._Storage->size()),
                ._MembershipVersion = InRegistry.Get_DirtyMarkerVersion(_QuerySlotDirtyMarkerHashes[Index])});
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoRebuildStructuralCandidates(
        const FCk_Registry& InRegistry,
        const entt::sparse_set& InDriveStorage)
    -> void
    {
        _StructuralCandidates.Reset();

        // This pass intentionally omits destroy/quarantine admission. A candidate can leave either state without
        // changing a dynamic pool, so preserving it here is what admits it on the next dispatch without a rescan.
        for (const auto Entity : InDriveStorage)
        {
            if (NOT InRegistry.IsValid(FCk_Entity{Entity}))
            { continue; }

            auto PassesStructuralJoin = true;
            for (const auto& Slot : _BatchState._Slots)
            {
                const auto Contains = Slot._Storage->contains(Entity);
                const auto IsExclude = Slot._Access == ECk_ScriptQueryAccess::Exclude;
                if (IsExclude ? Contains : (NOT Contains))
                { PassesStructuralJoin = false; break; }
            }

            if (PassesStructuralJoin)
            { _StructuralCandidates.Add(Entity); }
        }

        SnapshotStructuralCacheState(InRegistry, InDriveStorage);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoResolveAndJoin()
        -> bool
    {
        const auto QueryContext = _Instance->GetClass()->GetPathName();
        if (NOT ck::dynamic::Validate_ScriptQuerySlots(_Query, *QueryContext))
        {
            _Disabled = true;
            return false;
        }

        _BatchState._Slots.Reset();
        _BatchState._Entities.Reset();

        auto AnyHandle = _Instance->Get_Handle();
        _BatchState._AnyHandle = AnyHandle;
        auto RegistryView = AnyHandle.Get_RegistryView();   // by value: a lightweight view over the shared registry

        entt::storage<ck::FFragment_DynamicFragment_Data>* DriveStorage = nullptr;
        auto SmallestCount = MAX_int32;

        for (const auto& Slot : _Query._Slots)
        {
            const auto* Type = Slot._StructType.Get();

            const auto StorageId = UCk_Utils_DynamicFragment_UE::Get_StorageId(Type);
            auto& Storage = RegistryView.Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

            auto StateSlot = FCk_ScriptQueryBatchState::FSlot{};
            StateSlot._Type = Type;
            StateSlot._Storage = &Storage;
            StateSlot._Access = Slot._Access;
            _BatchState._Slots.Add(StateSlot);

            if (Slot._Access == ECk_ScriptQueryAccess::Exclude)
            { continue; }   // exclude pools neither drive nor gate emptiness

            const auto Count = static_cast<int32>(Storage.size());
            if (Count == 0)
            { return false; }   // a required/read pool is empty -> the join is empty

            if (Count < SmallestCount)
            {
                SmallestCount = Count;
                DriveStorage = &Storage;
            }
        }

        if (DriveStorage == nullptr)
        { return false; }   // no non-exclude slots -> nothing to iterate

        const auto& DriveStorageAsSet = static_cast<const entt::sparse_set&>(*DriveStorage);
        const auto PhysicalDriveCount = static_cast<int32>(DriveStorage->size());
        if (NOT _UseStructuralCache)
        {
            // Original join shape, with structural density counted before lifecycle gates. Pending destroy and
            // hydration must never make a dense membership set look sparse and cause mode thrash.
            auto StructuralMatches = 0;
            for (const auto Entity : DriveStorageAsSet)
            {
                if (NOT RegistryView.IsValid(Entity))
                { continue; }

                auto PassesJoin = true;
                for (const auto& StateSlot : _BatchState._Slots)
                {
                    const auto Contains = StateSlot._Storage->contains(Entity);
                    const auto IsExclude = StateSlot._Access == ECk_ScriptQueryAccess::Exclude;
                    if (IsExclude ? Contains : (NOT Contains))
                    { PassesJoin = false; break; }
                }
                if (NOT PassesJoin)
                { continue; }

                ++StructuralMatches;
                auto EntityHandle = AnyHandle.Get_ValidHandle(Entity);
                if (NOT ck_dynamic_script_query_processor::PassesDestroyFilter(EntityHandle, ECk_DestroyFilter::IgnorePendingKill) ||
                    EntityHandle.Has<ck::FTag_Hydration_Quarantine>())
                { continue; }

                _BatchState._Entities.Add(Entity);
            }

            // Physical storage size intentionally includes in-place-delete tombstones. A 1/256 live structural
            // match therefore enters the cache path next tick, while an all-live dense pool stays uncached.
            if (static_cast<int64>(StructuralMatches) * 4 < static_cast<int64>(PhysicalDriveCount) * 3)
            {
                _UseStructuralCache = true;
                _StructuralCacheDriveStorage = nullptr;
            }
            return _BatchState._Entities.Num() > 0;
        }

        if (DoesStructuralCacheNeedRebuild(_Registry, DriveStorageAsSet))
        {
            DoRebuildStructuralCandidates(_Registry, DriveStorageAsSet);
            // Consume the just-built list this tick, but let a dense result return to original traversal next tick.
            if (static_cast<int64>(_StructuralCandidates.Num()) * 4 >= static_cast<int64>(PhysicalDriveCount) * 3)
            { _UseStructuralCache = false; }
        }

        for (const auto Entity : _StructuralCandidates)
        {
            // Destroy has no per-dynamic-pool version, and entity IDs can later be recycled. Validate the full
            // entt generation before constructing a handle or touching any storage.
            if (NOT RegistryView.IsValid(Entity))
            { continue; }

            auto EntityHandle = AnyHandle.Get_ValidHandle(Entity);
            if (NOT ck_dynamic_script_query_processor::PassesDestroyFilter(EntityHandle, ECk_DestroyFilter::IgnorePendingKill))
            { continue; }

            // This remains an every-dispatch gate: leaving hydration quarantine changes no named dynamic pool.
            if (EntityHandle.Has<ck::FTag_Hydration_Quarantine>())
            { continue; }

            _BatchState._Entities.Add(Entity);
        }

        // NoEntities is the sanctioned route for a processor that wants to run without a join.
        return _BatchState._Entities.Num() > 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

#if WITH_DEV_AUTOMATION_TESTS
    auto
        FProcessor_ScriptQueryHosted::
        Run_JoinBenchmarkForTests(
            const FCk_Handle& InAnyHandle,
            bool bForceRebuild)
        -> bool
    {
        const auto InputIsValid = ck::IsValid(InAnyHandle);
        CK_ENSURE_IF_NOT(InputIsValid, TEXT("ScriptQuery benchmark requires a live fixture handle"))
        {}
        if (NOT InputIsValid)
        { return false; }

        // These are deliberately coupled to the sole test fixture. Avoid an AS UClass ABI in a test-only helper.
        auto* DevClass = FindObject<UClass>(nullptr, TEXT("/Script/Angelscript.Bb_TestProcessor_TypedLegacy"));
        auto* DriverClass = FindObject<UClass>(nullptr, TEXT("/Script/Angelscript.Bb_TestProcessor_TypedLegacy_Driver"));
        const auto ClassesAreValid = ck::IsValid(DevClass) && ck::IsValid(DriverClass);
        CK_ENSURE_IF_NOT(ClassesAreValid, TEXT("ScriptQuery benchmark fixture classes were unavailable"))
        {}
        if (NOT ClassesAreValid)
        { return false; }

        auto BenchmarkHost = FProcessor_ScriptQueryHosted{InAnyHandle.Get_RegistryView(), DevClass, DriverClass};
        if (BenchmarkHost._Disabled || NOT BenchmarkHost.DoResolveAndJoin())
        { return false; }

        auto LegacyState = FCk_ScriptQueryBatchState{};
        const auto RunLegacyJoin = [&BenchmarkHost, &LegacyState]() -> bool
        {
            const auto QueryContext = BenchmarkHost._Instance->GetClass()->GetPathName();
            if (NOT ck::dynamic::Validate_ScriptQuerySlots(BenchmarkHost._Query, *QueryContext))
            { return false; }

            LegacyState._Slots.Reset();
            LegacyState._Entities.Reset();
            auto AnyHandle = BenchmarkHost._Instance->Get_Handle();
            LegacyState._AnyHandle = AnyHandle;
            auto RegistryView = AnyHandle.Get_RegistryView();
            entt::storage<ck::FFragment_DynamicFragment_Data>* DriveStorage = nullptr;
            auto SmallestCount = MAX_int32;

            for (const auto& QuerySlot : BenchmarkHost._Query._Slots)
            {
                const auto* Type = QuerySlot._StructType.Get();
                auto& Storage = RegistryView.Storage<ck::FFragment_DynamicFragment_Data>(
                    UCk_Utils_DynamicFragment_UE::Get_StorageId(Type));
                LegacyState._Slots.Add({Type, &Storage, QuerySlot._Access});
                if (QuerySlot._Access == ECk_ScriptQueryAccess::Exclude)
                { continue; }

                const auto Count = static_cast<int32>(Storage.size());
                if (Count == 0)
                { return false; }
                if (Count < SmallestCount)
                {
                    SmallestCount = Count;
                    DriveStorage = &Storage;
                }
            }

            if (DriveStorage == nullptr)
            { return false; }

            // Baseline counterpart of the pre-cache production join: preserve all dynamic membership and lifecycle
            // checks, then compare its exact ordered result against DoResolveAndJoin's cache hit.
            for (const auto Entity : static_cast<const entt::sparse_set&>(*DriveStorage))
            {
                if (NOT RegistryView.IsValid(Entity))
                { continue; }
                auto EntityHandle = AnyHandle.Get_ValidHandle(Entity);
                if (NOT ck_dynamic_script_query_processor::PassesDestroyFilter(EntityHandle, ECk_DestroyFilter::IgnorePendingKill) ||
                    EntityHandle.Has<ck::FTag_Hydration_Quarantine>())
                { continue; }

                auto PassesJoin = true;
                for (const auto& Slot : LegacyState._Slots)
                {
                    const auto Contains = Slot._Storage->contains(Entity);
                    if (Slot._Access == ECk_ScriptQueryAccess::Exclude ? Contains : (NOT Contains))
                    { PassesJoin = false; break; }
                }
                if (PassesJoin)
                { LegacyState._Entities.Add(Entity); }
            }
            return LegacyState._Entities.Num() > 0;
        };

        // Establish both reused states before timing; neither warmup enters the VM.
        RunLegacyJoin();
        BenchmarkHost.DoResolveAndJoin();

        constexpr auto IterationsPerSample = 128;
        auto AllParity = true;
        for (int32 Sample = 0; Sample < 3; ++Sample)
        {
            double LegacySeconds = 0.0;
            double CachedSeconds = 0.0;
            const auto LegacyFirst = (Sample % 2) == 0;
            const auto RunLegacy = [&]()
            {
                const auto Start = FPlatformTime::Seconds();
                for (int32 Iteration = 0; Iteration < IterationsPerSample; ++Iteration)
                { RunLegacyJoin(); }
                LegacySeconds = FPlatformTime::Seconds() - Start;
            };
            const auto RunCached = [&]()
            {
                const auto Start = FPlatformTime::Seconds();
                for (int32 Iteration = 0; Iteration < IterationsPerSample; ++Iteration)
                {
                    if (bForceRebuild)
                    // Deliberately measures worst-case cache-miss work only; membership mutation itself is not timed.
                    { BenchmarkHost._StructuralCacheDriveStorage = nullptr; }
                    BenchmarkHost.DoResolveAndJoin();
                }
                CachedSeconds = FPlatformTime::Seconds() - Start;
            };
            if (LegacyFirst) { RunLegacy(); RunCached(); } else { RunCached(); RunLegacy(); }

            const auto Parity = LegacyState._Entities == BenchmarkHost._BatchState._Entities;
            AllParity &= Parity;
            UE_LOG(LogTemp, Log, TEXT("[CkScriptQueryCache PERF] mode=%s sample=%d iterations=%d legacy_total=%.6fms legacy_per_join=%.6fms cached_total=%.6fms cached_per_join=%.6fms count=%d parity=%s"),
                bForceRebuild ? TEXT("FORCE_REBUILD") : (BenchmarkHost._UseStructuralCache ? TEXT("SPARSE_CACHE") : TEXT("DENSE_UNCACHED")),
                Sample + 1, IterationsPerSample,
                LegacySeconds * 1000.0, LegacySeconds * 1000.0 / IterationsPerSample,
                CachedSeconds * 1000.0, CachedSeconds * 1000.0 / IterationsPerSample,
                BenchmarkHost._BatchState._Entities.Num(), Parity ? TEXT("true") : TEXT("false"));
        }
        return AllParity;
    }
#endif

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoDispatchBatch(
            TimeType InDeltaT)
        -> void
    {
        const auto Generation = ck::dynamic::Open_ScriptQueryBatchState(_BatchState);
        if (Generation == 0)
        { return; }

        auto Batch = FCk_ScriptQueryBatch{};
        Batch._State = &_BatchState;
        Batch._Generation = Generation;

        ON_SCOPE_EXIT
        {
            // A script exception must not leave the host address registered past its access window.
            ck::dynamic::Close_ScriptQueryBatchState(_BatchState, Generation);
        };

        _Instance->ForEachBatch(Batch, InDeltaT);
    }
}
