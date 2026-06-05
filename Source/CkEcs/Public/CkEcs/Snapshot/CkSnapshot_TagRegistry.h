#pragma once

#include "CoreMinimal.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h" // ck::registry_table::EnttRegistryType (+ entt registry.hpp). NO archive/snapshot.

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp" // entt::type_hash, basic_registry::storage<T> — light; NOT snapshot.hpp

// --------------------------------------------------------------------------------------------------------------------
// Archive-free registry of every snapshotable ECS tag. Populated at static-init by the inline-variable registrars
// CK_DEFINE_ECS_TAG emits (see CkTag.h). Deliberately pulls only entt-registry + Core containers so that CkTag.h —
// a header included by ~every fragment header — never drags in the entt-snapshot / archive machinery.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Snapshot_RegisteredTag
    {
        uint32 _TypeHash = 0;
        // Captureless: materializes storage<T> on the loading registry so storage(hash) is non-null on restore.
        void (*_EnsureStorage)(ck::registry_table::EnttRegistryType&) = nullptr;
        FString _DisplayName;
    };

    class CKECS_API FCk_Snapshot_TagRegistry
    {
    public:
        static auto Get() -> FCk_Snapshot_TagRegistry&;

        // Idempotent across DLLs: dedupes by _TypeHash (inline-var registrars init once per module DLL).
        auto Register(FCk_Snapshot_RegisteredTag InEntry) -> void;
        auto Get_All() const -> const TArray<FCk_Snapshot_RegisteredTag>&;
        auto Find_ByHash(uint32 InTypeHash) const -> const FCk_Snapshot_RegisteredTag*;

    private:
        TArray<FCk_Snapshot_RegisteredTag> _Entries;
        TSet<uint32> _Seen;
    };

    namespace detail
    {
        // Builds the POD and registers it. Uses ONLY entt::type_hash + a captureless lambda — NO StaticStruct/UObject,
        // so it is safe to run at static-init (before UObject packages exist). Called from ck::TTag<T> (CkTag.h) so the
        // CK_DEFINE_ECS_TAG macro stays a plain struct definition, legal at namespace / class / block scope alike.
        template <typename T>
        auto Register_SnapshotableTag() -> bool
        {
            auto Entry = FCk_Snapshot_RegisteredTag{};
            Entry._TypeHash      = entt::type_hash<T>::value();
            Entry._EnsureStorage = [](ck::registry_table::EnttRegistryType& InRegistry) { InRegistry.storage<T>(); };
            Entry._DisplayName   = FString::Printf(TEXT("Tag#%u"), Entry._TypeHash);

            FCk_Snapshot_TagRegistry::Get().Register(MoveTemp(Entry));
            return true;
        }
    }
}
