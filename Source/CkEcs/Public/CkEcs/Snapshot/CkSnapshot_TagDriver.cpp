#include "CkSnapshot_TagDriver.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h" // CK_WITH_FIDELITY_ORACLE define (Phase 5 gate; this TU has no other source of it)
#include "CkEcs/Snapshot/CkSnapshot_TagRegistry.h"
#include "CkEcs/Handle/CkHandle.h" // ck::FTag_DestroyEntity_*, ck::FTag_EntityJustCreated
#include "CkEcs/CkEcsLog.h"        // ck::ecs::Warning

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

#if CK_WITH_FIDELITY_ORACLE // Phase 5: Model-A tag capture/restore is oracle/test-only (the v3 load never stamps or captures these)

namespace ck::snapshot
{
    auto
        Capture_Tags(
            ck::SnapshotRegistryType& InRegistry,
            FArchive& InByteWriter)
        -> void
    {
        // Collect present tag types first — the count is written before the entries.
        struct FPresentTag { uint32 _Hash = 0; TArray<uint32> _EnttIds; };
        auto Present = TArray<FPresentTag>{};

        for (const auto& Tag : ck::FCk_Snapshot_TagRegistry::Get().Get_All())
        {
            auto* Storage = InRegistry.storage(Tag._TypeHash);
            if (Storage == nullptr || Storage->empty())
            { continue; }

            auto Entry = FPresentTag{};
            Entry._Hash = Tag._TypeHash;
            Entry._EnttIds.Reserve(static_cast<int32>(Storage->size()));
            for (const auto Entity : *Storage)
            { Entry._EnttIds.Add(static_cast<uint32>(Entity)); }

            Present.Emplace(MoveTemp(Entry));
        }

        auto NumTagTypes = static_cast<int32>(Present.Num());
        InByteWriter << NumTagTypes;

        for (auto& Entry : Present)
        {
            InByteWriter << Entry._Hash;
            auto Count = static_cast<int32>(Entry._EnttIds.Num());
            InByteWriter << Count;
            for (auto Id : Entry._EnttIds)
            { InByteWriter << Id; }
        }
    }

    auto
        Restore_Tags(
            ck::SnapshotRegistryType& InRegistry,
            entt::basic_continuous_loader<ck::SnapshotRegistryType>& InLoader,
            FArchive& InByteReader,
            int64 InTagSectionByteOffset)
        -> void
    {
        InByteReader.Seek(InTagSectionByteOffset);

        auto NumTagTypes = int32{0};
        InByteReader << NumTagTypes;

        const auto& Registry = ck::FCk_Snapshot_TagRegistry::Get();

        for (auto TagIndex = 0; TagIndex < NumTagTypes; ++TagIndex)
        {
            auto Hash  = uint32{0};
            auto Count = int32{0};
            InByteReader << Hash;
            InByteReader << Count;

            const auto* Registered = Registry.Find_ByHash(Hash);
            if (Registered == nullptr)
            {
                for (auto j = 0; j < Count; ++j)
                { auto Skip = uint32{0}; InByteReader << Skip; }

                ck::ecs::Warning(TEXT("Restore_Tags: skipped unknown tag hash [{}] ([{}] entities)"), Hash, Count);
                continue;
            }

            Registered->_EnsureStorage(InRegistry);
            auto* Storage = InRegistry.storage(Hash);

            for (auto j = 0; j < Count; ++j)
            {
                auto SavedId = uint32{0};
                InByteReader << SavedId;

                if (Storage == nullptr)
                { continue; }

                const auto Live = InLoader.map(static_cast<entt::entity>(SavedId));
                if (Live != entt::null)
                { Storage->push(Live); }
            }
        }
    }

    auto
        Strip_LifecycleTags(
            ck::SnapshotRegistryType& InRegistry)
        -> void
    {
        InRegistry.clear<ck::FTag_EntityJustCreated>();
        InRegistry.clear<ck::FTag_DestroyEntity_Initiate>();
        InRegistry.clear<ck::FTag_DestroyEntity_EndPlay>();
        InRegistry.clear<ck::FTag_DestroyEntity_Teardown>();
        InRegistry.clear<ck::FTag_DestroyEntity_Await>();
        InRegistry.clear<ck::FTag_DestroyEntity_Finalize>();
    }
}

#endif // CK_WITH_FIDELITY_ORACLE

// --------------------------------------------------------------------------------------------------------------------
