#include "CkSnapshot_Restore.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"
#include "CkSnapshot/Context/CkSnapshot_Context.h"
#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

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
            const FCk_Snapshot_Header& InHeader)
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

        auto Context = ck::FSnapshotContext{Loader};
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
                Registered->_Load(Loader, Reader);
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

        Report = Run_Restore_Registry(*RawRegistry, InByteReader, InHeader);

        ck::snapshot::Verbose(TEXT("Run_Restore: restored [{}] entities into World [{}], skipped [{}] fragment types"),
            Report.Get_EntitiesRestored(), InWorld.GetName(), Report.Get_SkippedFragmentTypes().Num());

        return Report;
    }
}

// --------------------------------------------------------------------------------------------------------------------
