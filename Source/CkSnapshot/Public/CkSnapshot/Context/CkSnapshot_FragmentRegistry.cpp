#include "CkSnapshot_FragmentRegistry.h"

#include "CkSnapshot/CkSnapshot_Log.h"

namespace ck
{
    auto FCk_Snapshot_FragmentRegistry::Get() -> FCk_Snapshot_FragmentRegistry&
    {
        static FCk_Snapshot_FragmentRegistry Instance;
        return Instance;
    }

    auto FCk_Snapshot_FragmentRegistry::Register(FCk_Snapshot_RegisteredFragment InEntry) -> void
    {
        _Entries.Emplace(MoveTemp(InEntry));
    }

    auto FCk_Snapshot_FragmentRegistry::Get_All() const -> const TArray<FCk_Snapshot_RegisteredFragment>&
    {
        return _Entries;
    }

    auto FCk_Snapshot_FragmentRegistry::Find_ByDisplayName(const FString& InName) const -> const FCk_Snapshot_RegisteredFragment*
    {
        return _Entries.FindByPredicate([&](const auto& E) { return E._DisplayName == InName; });
    }

    auto FCk_Snapshot_FragmentRegistry::Find_ByEnttHash(uint32 InHash) const -> const FCk_Snapshot_RegisteredFragment*
    {
        return _Entries.FindByPredicate([&](const auto& E) { return E._EnttTypeHash == InHash; });
    }
}
