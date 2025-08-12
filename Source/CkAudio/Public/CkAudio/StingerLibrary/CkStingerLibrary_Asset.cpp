// ============================================================================
// Stinger Library Implementation - CkStingerLibrary_Asset.cpp
// ============================================================================

#include "CkStingerLibrary_Asset.h"

// ============================================================================

auto
    UCk_StingerLibrary_Base::
    Get_StingerEntry(FGameplayTag InStingerName, bool& bFound) const
    -> FCk_StingerEntry
{
    if (const auto* Entry = _Stingers.FindByPredicate([InStingerName](const FCk_StingerEntry& StingerEntry)
    {
        return StingerEntry.Get_StingerName() == InStingerName;
    }))
    {
        bFound = true;
        return *Entry;
    }

    bFound = false;
    return FCk_StingerEntry{};
}

auto
    UCk_StingerLibrary_Base::
    Get_StingerIndex(FGameplayTag InStingerName) const
    -> int32
{
    return _Stingers.IndexOfByPredicate([InStingerName](const FCk_StingerEntry& Entry)
    {
        return Entry.Get_StingerName() == InStingerName;
    });
}
