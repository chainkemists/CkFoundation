#include "CkSnapshot_Restore.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // ck::FTag_Snapshot_JustRestored
#include "CkEcs/Snapshot/CkSnapshot_TagDriver.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "CkEcs/Handle/CkHandle.h"        // FCk_Handle + Add<ck::FTag_Snapshot_JustRestored>
#include "CkEcs/Handle/CkHandle_Utils.h"  // ck::MakeHandle
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "Serialization/Archive.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    auto
        Run_Restore_Registry(
            ck::SnapshotRegistryType& InRegistry,
            FArchive& InByteReader,
            const FCk_Snapshot_Header& InHeader,
            FCk_RegistryHandle InLoadRegistryHandle)
        -> FCk_Snapshot_LoadReport
    {
        auto Report = FCk_Snapshot_LoadReport{};
        Report.Set_LoadedHeader(InHeader);
        Report.Set_Result(ECk_SnapshotResult::Failed_IO);

        // ---- WIPE: clear the registry so restore replaces rather than merges -----------------------------------
        // Wipe-then-restore is the core contract: the result must contain EXACTLY the saved entities. A full
        // clear() resets EnTT's entity-ID space, removing all entities + fragments, which is precisely what the
        // FEcsWorld test and the whole-world-replace model need. Because the registry is now empty, the
        // continuous_loader below remaps the saved IDs into a fresh space starting from empty -- so the
        // Snapshot_Handle remap path (via loader.map()) keeps working untouched.
        //
        // TODO(layer-3): in the live-world path, route teardown through EndPlay processors + level reload
        // instead of a raw clear -- a raw clear nukes the EcsWorld subsystem's own bookkeeping (transient
        // entity, ctx). That live-world teardown nuance is out of scope for the registry core (Layer 2/3).
        InRegistry.clear();

        // ---- Build the continuous loader + archive -------------------------------------------------------------
        constexpr auto LoadIfFindFails = true;
        auto ProxyArchive = FObjectAndNameAsStringProxyArchive{InByteReader, LoadIfFindFails};

        auto Loader = entt::basic_continuous_loader<ck::SnapshotRegistryType>{InRegistry};

        // Re-home restored handles onto InLoadRegistryHandle when provided (cross-registry restore, e.g. a unit
        // test capturing in world A and restoring into world B). Defaults to Unset for the same-registry case,
        // where Snapshot_Handle's IsSet() guard skips re-homing and the entity-id remap alone is correct.
        auto Context = ck::FSnapshotContext{Loader, InLoadRegistryHandle};
        auto Reader  = ck::FSnapshotArchive_Reader{ProxyArchive, Context};

        // The entities pass is restored first (Capture wrote it before the manifest entries). The continuous
        // loader maps remote identifiers to local ones; subsequent fragment loads route handle refs through
        // the same loader via FSnapshotContext::Snapshot_Handle.
        Loader.get<ck::SnapshotEntityType>(Reader);

        // ---- Manifest-driven fragment dispatch -----------------------------------------------------------------
        const auto& Registry = ck::FCk_Snapshot_FragmentRegistry::Get();

        auto SkippedTypes = TArray<FString>{};

        for (const auto& Entry : InHeader.Get_Manifest())
        {
            const auto* Registered = Registry.Find_ByEnttHash(Entry.Get_EnttTypeHash());

            if (Registered != nullptr && Registered->_Load)
            {
                Registered->_Load(Loader, Reader, Entry.Get_StorageId());
            }
            else
            {
                // Unknown (or unloadable) fragment type. Byte-jump past it so the stream position stays
                // aligned for the next manifest entry, and record the skip.
                const auto JumpTo = Entry.Get_ByteOffset() + Entry.Get_ByteLength();
                InByteReader.Seek(JumpTo);
                SkippedTypes.Emplace(Entry.Get_DisplayName());

                ck::snapshot::Warning(TEXT("Run_Restore_Registry: skipped unknown fragment type [{}] (hash [{}], [{}] bytes)"),
                    Entry.Get_DisplayName(), Entry.Get_EnttTypeHash(), Entry.Get_ByteLength());
            }

            // Truncated/corrupt stream → archive error flag. Stop rather than deserialize garbage as Success.
            if (InByteReader.IsError())
            {
                ck::snapshot::Error(TEXT("Run_Restore_Registry: byte stream errored while restoring fragment [{}] — the "
                    "save is corrupt/truncated. Aborting restore."), Entry.Get_DisplayName());
                return Report; // Result is still Failed_IO
            }
        }

        // ---- Tag section + defensive lifecycle strip (BEFORE orphans, so tag-only entities survive) -----------
        ck::snapshot::Restore_Tags(InRegistry, Loader, InByteReader, InHeader.Get_TagSectionByteOffset());
        ck::snapshot::Strip_LifecycleTags(InRegistry);

        if (InByteReader.IsError())
        {
            ck::snapshot::Error(TEXT("Run_Restore_Registry: byte stream errored while restoring the tag section — the "
                "save is corrupt/truncated. Aborting restore."));
            return Report; // Result is still Failed_IO
        }

        // ---- Finalize: release entities that ended up with no components ---------------------------------------
        Loader.orphans();

        const auto EntitiesRestored = static_cast<int32>(InRegistry.storage<ck::SnapshotEntityType>().size());

        Report.Set_SkippedFragmentTypes(MoveTemp(SkippedTypes));
        Report.Set_EntitiesRestored(EntitiesRestored);
        Report.Set_Result(ECk_SnapshotResult::Success);

        ck::snapshot::Verbose(TEXT("Run_Restore_Registry: restored [{}] entities, skipped [{}] fragment types"),
            EntitiesRestored, Report.Get_SkippedFragmentTypes().Num());

        return Report;
    }

    auto
        Run_Restore(
            UWorld& InWorld,
            FArchive& InByteReader,
            const FCk_Snapshot_Header& InHeader)
        -> FCk_Snapshot_LoadReport
    {
        auto Report = FCk_Snapshot_LoadReport{};
        Report.Set_LoadedHeader(InHeader);
        Report.Set_Result(ECk_SnapshotResult::Failed_IO);

        // ---- Resolve the live entt registry --------------------------------------------------------------------
        auto* EcsWorld = InWorld.GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EcsWorld))
        {
            ck::snapshot::Error(TEXT("Run_Restore: no UCk_EcsWorld_Subsystem_UE on World [{}]"), InWorld.GetName());
            return Report;
        }

        auto& CkRegistry = EcsWorld->Get_Registry();
        auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
        if (RawRegistry == nullptr)
        {
            ck::snapshot::Error(TEXT("Run_Restore: could not resolve the raw entt registry from World [{}]"), InWorld.GetName());
            return Report;
        }

        // ---- WIPE + REBUILD (live-world). Same wipe-then-restore as Run_Restore_Registry, but afterwards we ADOPT
        // the restored transient: the world's bookkeeping (FCtx_TransientEntity + cached handle + world-fragment) is
        // re-pointed at it, so every restored LifetimeOwner/ContextOwner ref resolves and entities come back LIVE
        // (not data-only). The registry-core Run_Restore_Registry stays clear-based for the FEcsWorld test (no subsystem).
        RawRegistry->clear();

        constexpr auto LoadIfFindFails = true;
        auto ProxyArchive = FObjectAndNameAsStringProxyArchive{InByteReader, LoadIfFindFails};
        auto Loader  = entt::basic_continuous_loader<ck::SnapshotRegistryType>{*RawRegistry};
        // Pass the live registry handle so Snapshot_Handle re-homes every restored handle onto THIS world
        // (post-seamless-travel the saved slot is gone). Without it, restored handles read [INVALID REGISTRY].
        auto Context = ck::FSnapshotContext{Loader, CkRegistry.Get_RegistryHandle()};
        auto Reader  = ck::FSnapshotArchive_Reader{ProxyArchive, Context};

        Loader.get<ck::SnapshotEntityType>(Reader);

        const auto& Registry = ck::FCk_Snapshot_FragmentRegistry::Get();
        auto SkippedTypes = TArray<FString>{};

        for (const auto& Entry : InHeader.Get_Manifest())
        {
            const auto* Registered = Registry.Find_ByEnttHash(Entry.Get_EnttTypeHash());

            if (Registered != nullptr && Registered->_Load)
            {
                Registered->_Load(Loader, Reader, Entry.Get_StorageId());
            }
            else
            {
                InByteReader.Seek(Entry.Get_ByteOffset() + Entry.Get_ByteLength());
                SkippedTypes.Emplace(Entry.Get_DisplayName());

                ck::snapshot::Warning(TEXT("Run_Restore: skipped unknown fragment type [{}] (hash [{}], [{}] bytes)"),
                    Entry.Get_DisplayName(), Entry.Get_EnttTypeHash(), Entry.Get_ByteLength());
            }

            // A truncated/corrupt stream sets the archive error flag (reads past the end return zeroes and mark
            // error). Every subsequent fragment would deserialize garbage — stop HERE and report, rather than
            // completing a garbage half-restore as Success.
            if (InByteReader.IsError())
            {
                ck::snapshot::Error(TEXT("Run_Restore: byte stream errored while restoring fragment [{}] — the save is "
                    "corrupt/truncated. Aborting restore."), Entry.Get_DisplayName());
                Report.Set_Result(ECk_SnapshotResult::Failed_IO);
                return Report;
            }
        }

        // ---- Tag section + defensive lifecycle strip (BEFORE the transient adopt + orphans) ---------------------
        ck::snapshot::Restore_Tags(*RawRegistry, Loader, InByteReader, InHeader.Get_TagSectionByteOffset());
        ck::snapshot::Strip_LifecycleTags(*RawRegistry);

        if (InByteReader.IsError())
        {
            ck::snapshot::Error(TEXT("Run_Restore: byte stream errored while restoring the tag section — the save is "
                "corrupt/truncated. Aborting restore."));
            Report.Set_Result(ECk_SnapshotResult::Failed_IO);
            return Report;
        }

        // ---- ADOPT the restored transient. Do this BEFORE orphans() so it is never released (it carries
        // LifetimeDependents, so orphans would keep it anyway, but the explicit adopt re-wires the world bookkeeping).
        const auto CapturedTransient = static_cast<entt::entity>(InHeader.Get_TransientEntityId());
        const auto RestoredTransient = Loader.map(CapturedTransient);

        if (RestoredTransient != entt::null)
        {
            EcsWorld->Request_AdoptRestoredTransient(FCk_Entity{RestoredTransient});
        }
        else
        {
            ck::snapshot::Error(TEXT("Run_Restore: captured transient id [{}] did not map to a restored entity on World [{}]"),
                InHeader.Get_TransientEntityId(), InWorld.GetName());
        }

        Loader.orphans();

        // Stamp a generic "just restored" marker on every surviving restored entity so replicated features (e.g.
        // CkAttribute) can RE-DRIVE replication of the restored VALUES to clients. The per-feature replication trigger
        // tags are transient + consumed before a save, so they are NOT restored — without this, a client keeps its
        // Construct-default values for restored entities. Collect first: Add mutates the entity storage being iterated.
        {
            auto RestoredEntities = TArray<FCk_Handle>{};
            for (const auto Entity : RawRegistry->storage<ck::SnapshotEntityType>())
            {
                RestoredEntities.Add(ck::MakeHandle(FCk_Entity{Entity}, CkRegistry));
            }
            for (auto& Handle : RestoredEntities)
            {
                if (ck::IsValid(Handle))
                { Handle.Add<ck::FTag_Snapshot_JustRestored>(); }
            }
        }

        Report.Set_SkippedFragmentTypes(MoveTemp(SkippedTypes));
        Report.Set_EntitiesRestored(static_cast<int32>(RawRegistry->storage<ck::SnapshotEntityType>().size()));
        Report.Set_Result(ECk_SnapshotResult::Success);

        ck::snapshot::Verbose(TEXT("Run_Restore: restored [{}] entities into World [{}] (live), skipped [{}] fragment types"),
            Report.Get_EntitiesRestored(), InWorld.GetName(), Report.Get_SkippedFragmentTypes().Num());

        return Report;
    }
}

// --------------------------------------------------------------------------------------------------------------------
