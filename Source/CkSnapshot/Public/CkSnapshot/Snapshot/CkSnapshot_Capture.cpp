#include "CkSnapshot_Capture.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Context/CkSnapshot_Context.h"
#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
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

        // ---- Per-fragment-type manifest entries ----------------------------------------------------------------
        auto Manifest = TArray<FCk_Snapshot_Header_FragmentManifestEntry>{};

        for (const auto& Registered : ck::FCk_Snapshot_FragmentRegistry::Get().Get_All())
        {
            if (NOT Registered._Save)
            { continue; }

            const auto ByteOffset = InByteWriter.Tell();
            Registered._Save(Snapshot, Writer);
            const auto ByteLength = InByteWriter.Tell() - ByteOffset;

            auto Entry = FCk_Snapshot_Header_FragmentManifestEntry{};
            Entry.Set_DisplayName(Registered._DisplayName);
            Entry.Set_EnttTypeHash(Registered._EnttTypeHash);
            // Per-fragment instance count is not available through the type-erased _Save closure (the entt
            // get<T> writes the storage count into the byte stream, not back to us). Left 0 for V1; the
            // header's top-level _EntityCount carries the total entity set size, which is what Restore needs.
            Entry.Set_EntityCount(0);
            Entry.Set_ByteOffset(ByteOffset);
            Entry.Set_ByteLength(ByteLength);

            Manifest.Emplace(MoveTemp(Entry));
        }

        InOutHeader.Set_Manifest(MoveTemp(Manifest));

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
