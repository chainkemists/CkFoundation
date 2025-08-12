// ============================================================================
// Audio Director Config Implementation - CkAudioDirector_Config.cpp
// ============================================================================

#include "CkAudioDirector_Config.h"

// ============================================================================

auto
    UCk_AudioDirector_Config::
    Find_MusicLibrary(FGameplayTag InLibraryName) const
    -> UCk_MusicLibrary_Base*
{
    auto* FoundLibrary = _MusicLibraries.FindByPredicate([InLibraryName](const TObjectPtr<UCk_MusicLibrary_Base>& Library)
    {
        return ck::IsValid(Library) && Library->Get_LibraryName() == InLibraryName;
    });

    return FoundLibrary ? FoundLibrary->Get() : nullptr;
}

auto
    UCk_AudioDirector_Config::
    Find_StingerLibrary(FGameplayTag InLibraryName) const
    -> UCk_StingerLibrary_Base*
{
    auto* FoundLibrary = _StingerLibraries.FindByPredicate([InLibraryName](const TObjectPtr<UCk_StingerLibrary_Base>& Library)
    {
        return ck::IsValid(Library) && Library->Get_LibraryName() == InLibraryName;
    });

    return FoundLibrary ? FoundLibrary->Get() : nullptr;
}

auto
    UCk_AudioDirector_Config::
    Get_StingerEntry(FGameplayTag InStingerName, bool& bFound) const
    -> FCk_StingerEntry
{
    // Search across all stinger libraries
    for (const auto& Library : _StingerLibraries)
    {
        if (ck::Is_NOT_Valid(Library))
        {
            continue;
        }

        bool bLibraryFound = false;
        const auto Entry = Library->Get_StingerEntry(InStingerName, bLibraryFound);
        if (bLibraryFound)
        {
            bFound = true;
            return Entry;
        }
    }

    bFound = false;
    return FCk_StingerEntry{};
}
