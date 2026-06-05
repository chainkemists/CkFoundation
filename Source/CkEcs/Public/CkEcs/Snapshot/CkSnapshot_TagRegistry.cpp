#include "CkSnapshot_TagRegistry.h"

namespace ck
{
    auto FCk_Snapshot_TagRegistry::Get() -> FCk_Snapshot_TagRegistry&
    {
        static FCk_Snapshot_TagRegistry Instance;
        return Instance;
    }

    auto FCk_Snapshot_TagRegistry::Register(FCk_Snapshot_RegisteredTag InEntry) -> void
    {
        if (_Seen.Contains(InEntry._TypeHash))
        { return; }

        _Seen.Add(InEntry._TypeHash);
        _Entries.Emplace(MoveTemp(InEntry));
    }

    auto FCk_Snapshot_TagRegistry::Get_All() const -> const TArray<FCk_Snapshot_RegisteredTag>&
    {
        return _Entries;
    }

    auto FCk_Snapshot_TagRegistry::Find_ByHash(uint32 InTypeHash) const -> const FCk_Snapshot_RegisteredTag*
    {
        return _Entries.FindByPredicate([&](const auto& E) { return E._TypeHash == InTypeHash; });
    }
}
