#include "CkSnapshot_Capture.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Audit.h"
#include "CkEcs/Snapshot/CkSnapshot_TagDriver.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "Misc/EngineVersion.h"
#include "Serialization/Archive.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    auto
        Run_Capture_Registry(
            ck::SnapshotRegistryType& InRegistry,
            FArchive& InByteWriter,
            FCk_Snapshot_Header& InOutHeader)
        -> ECk_SnapshotResult
    {
        // ---- One-shot classification audit -----------------------------------------------------------------------
        // Every RoundTrip-classified holder/record must be registered snapshotable. All macro-emitted RoundTrip
        // announces complete at static-init (before any capture), so a single pass at the first capture catches a
        // forgotten CK_REGISTER_SNAPSHOTABLE and ensures by name instead of silently dropping the type on save/load.
        {
            static auto bAuditedThisProcess = false;
            if (NOT bAuditedThisProcess)
            {
                bAuditedThisProcess = true;
                ck::snapshot_audit::Audit_ExpectedAreRegistered();
            }
        }

        // ---- Stamp header --------------------------------------------------------------------------------------
        // _WorldAssetPath is intentionally left untouched here — the registry core has no UWorld. The UWorld
        // wrapper stamps it before delegating.
        InOutHeader.Set_FormatVersion(1);
        InOutHeader.Set_EngineVersion(FEngineVersion::Current().ToString());
        InOutHeader.Set_TimestampUTC(FDateTime::UtcNow());

        // ---- Build the snapshot saver + archive ----------------------------------------------------------------
        constexpr auto LoadIfFindFails = true;
        auto ProxyArchive = FObjectAndNameAsStringProxyArchive{InByteWriter, LoadIfFindFails};

        auto Snapshot = entt::basic_snapshot<ck::SnapshotRegistryType>{InRegistry};

        auto Context = ck::FSnapshotContext{Snapshot};
        auto Writer  = ck::FSnapshotArchive_Writer{ProxyArchive, Context};

        // The entities pass MUST come first (the loader restores it first too). It is not a manifest entry --
        // it carries the entity set + versions, not a fragment storage.
        Snapshot.get<ck::SnapshotEntityType>(Writer);

        const auto TotalEntities = static_cast<int32>(InRegistry.storage<ck::SnapshotEntityType>().size());
        InOutHeader.Set_EntityCount(TotalEntities);

        // Stamp the transient entity id so the live-world restore can adopt the restored transient. The registry-core
        // path (FEcsWorld with no transient ctx) leaves the header sentinel untouched; only the live-world restore reads it.
        if (const auto* TransientCtx = InRegistry.ctx().find<const ck::FCtx_TransientEntity>())
        {
            InOutHeader.Set_TransientEntityId(static_cast<uint32>(TransientCtx->Entity.Get_ID()));
        }

        // ---- Per-storage manifest entries ----------------------------------------------------------------------
        // Iterate the registry's STORAGES, not just the registered fragment TYPES. Most fragments live only in
        // their default storage (id == entt::type_hash<T>), but dynamic fragments (CkDynamic) keep one NAMED
        // storage per AS struct type (id == hash of the struct path). entt's snapshot is opt-in PER STORAGE, so a
        // single get<T>() with the default id would miss every named storage -- silently dropping all
        // dynamic/AngelScript fragments on save/load. Each storage whose value-type maps to a registered
        // snapshotable fragment is captured with ITS id; restore replays it onto the same id. Ordinary
        // single-storage fragments are captured exactly as before (their only storage IS the default one).
        auto Manifest = TArray<FCk_Snapshot_Header_FragmentManifestEntry>{};

        const auto& FragmentRegistry = ck::FCk_Snapshot_FragmentRegistry::Get();

        for (auto&& StoragePair : InRegistry.storage())
        {
            const auto StorageId = StoragePair.first;
            const auto TypeHash  = static_cast<uint32>(StoragePair.second.info().hash());

            const auto* Registered = FragmentRegistry.Find_ByEnttHash(TypeHash);
            if (Registered == nullptr || NOT Registered->_Save)
            { continue; } // not a registered snapshotable type (e.g. the entity storage, captured separately above).

            const auto ByteOffset = InByteWriter.Tell();
            Registered->_Save(Snapshot, Writer, StorageId);
            const auto ByteLength = InByteWriter.Tell() - ByteOffset;

            auto Entry = FCk_Snapshot_Header_FragmentManifestEntry{};
            Entry.Set_DisplayName(Registered->_DisplayName);
            Entry.Set_EnttTypeHash(TypeHash);
            Entry.Set_StorageId(StorageId);
            // Per-fragment instance count is not available through the type-erased _Save closure (the entt
            // get<T> writes the storage count into the byte stream, not back to us). Left 0 for V1; the
            // header's top-level _EntityCount carries the total entity set size, which is what Restore needs.
            Entry.Set_EntityCount(0);
            Entry.Set_ByteOffset(ByteOffset);
            Entry.Set_ByteLength(ByteLength);

            Manifest.Emplace(MoveTemp(Entry));
        }

        InOutHeader.Set_Manifest(MoveTemp(Manifest));

        // ---- Tag section: self-describing, appended after all fragment passes ----------------------------------
        InOutHeader.Set_TagSectionByteOffset(InByteWriter.Tell());
        ck::snapshot::Capture_Tags(InRegistry, InByteWriter);

        ck::snapshot::Verbose(TEXT("Run_Capture_Registry: captured [{}] entities, [{}] fragment-type manifest entries"),
            TotalEntities, InOutHeader.Get_Manifest().Num());

        return ECk_SnapshotResult::Success;
    }

    auto
        Run_Capture(
            UWorld& InWorld,
            FArchive& InByteWriter,
            FCk_Snapshot_Header& InOutHeader)
        -> ECk_SnapshotResult
    {
        // ---- Resolve the live entt registry --------------------------------------------------------------------
        auto* EcsWorld = InWorld.GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EcsWorld))
        {
            ck::snapshot::Error(TEXT("Run_Capture: no UCk_EcsWorld_Subsystem_UE on World [{}]"), InWorld.GetName());
            return ECk_SnapshotResult::Failed_IO;
        }

        auto& CkRegistry = EcsWorld->Get_Registry();
        auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
        if (RawRegistry == nullptr)
        {
            ck::snapshot::Error(TEXT("Run_Capture: could not resolve the raw entt registry from World [{}]"), InWorld.GetName());
            return ECk_SnapshotResult::Failed_IO;
        }

        // The world asset path is a UWorld-only concern; stamp it here so the registry core stays world-agnostic.
        InOutHeader.Set_WorldAssetPath(FSoftObjectPath{&InWorld});

        const auto Result = Run_Capture_Registry(*RawRegistry, InByteWriter, InOutHeader);

        ck::snapshot::Verbose(TEXT("Run_Capture: captured [{}] entities from World [{}]"),
            InOutHeader.Get_EntityCount(), InWorld.GetName());

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
