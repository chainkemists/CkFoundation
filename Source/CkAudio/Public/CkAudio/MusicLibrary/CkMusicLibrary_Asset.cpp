#include "CkMusicLibrary_Asset.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// ============================================================================

auto
    UCk_MusicLibrary_Base::
    Get_NextTrack(const TArray<FGameplayTag>& InRecentTracks) const
    -> int32
{
    if (_Tracks.IsEmpty())
    {
        return INDEX_NONE;
    }

    switch (_SelectionMode)
    {
        case ECk_MusicSelectionMode::Random:
            return DoGet_NextTrack_Random();

        case ECk_MusicSelectionMode::WeightedRandom:
            return DoGet_NextTrack_WeightedRandom();

        case ECk_MusicSelectionMode::Sequential:
            return DoGet_NextTrack_Sequential();

        case ECk_MusicSelectionMode::MoodBased:
            return DoGet_NextTrack_MoodBased(InRecentTracks);

        default:
            return DoGet_NextTrack_Random();
    }
}

auto
    UCk_MusicLibrary_Base::
    DoGet_NextTrack_Random() const
    -> int32
{
    return FMath::RandRange(0, _Tracks.Num() - 1);
}

auto
    UCk_MusicLibrary_Base::
    DoGet_NextTrack_WeightedRandom() const
    -> int32
{
    auto TotalWeight = 0.0f;
    for (const auto& Track : _Tracks)
    {
        TotalWeight += Track.Get_Weight();
    }

    if (TotalWeight <= 0.0f)
    {
        return DoGet_NextTrack_Random();
    }

    const auto RandomValue = FMath::RandRange(0.0f, TotalWeight);
    auto CurrentWeight = 0.0f;

    for (auto i = 0; i < _Tracks.Num(); ++i)
    {
        CurrentWeight += _Tracks[i].Get_Weight();
        if (RandomValue <= CurrentWeight)
        {
            return i;
        }
    }

    return _Tracks.Num() - 1; // Fallback to last track
}

auto
    UCk_MusicLibrary_Base::
    DoGet_NextTrack_Sequential() const
    -> int32
{
    // TODO: Implement sequential tracking - needs runtime state
    // For now, fallback to random
    return DoGet_NextTrack_Random();
}

auto
    UCk_MusicLibrary_Base::
    DoGet_NextTrack_MoodBased(const TArray<FGameplayTag>& InRecentTracks) const
    -> int32
{
    if (_ActiveMoodTags.IsEmpty())
    {
        return DoGet_NextTrack_WeightedRandom();
    }

    // Find tracks that match current mood
    TArray<int32> MatchingIndices;

    for (auto i = 0; i < _Tracks.Num(); ++i)
    {
        const auto& Track = _Tracks[i];
        const auto& TrackMoods = Track.Get_MoodTags();

        // Check if track has any matching mood tags
        const auto HasMatchingMood = _ActiveMoodTags.ContainsByPredicate([&TrackMoods](const FGameplayTag& MoodTag)
        {
            return TrackMoods.Contains(MoodTag);
        });

        if (HasMatchingMood)
        {
            MatchingIndices.Add(i);
        }
    }

    if (MatchingIndices.IsEmpty())
    {
        // No mood matches, fallback to weighted random
        return DoGet_NextTrack_WeightedRandom();
    }

    // Apply weighted selection within matching tracks
    auto TotalWeight = 0.0f;
    for (const auto Index : MatchingIndices)
    {
        TotalWeight += _Tracks[Index].Get_Weight();
    }

    if (TotalWeight <= 0.0f)
    {
        // Equal weight selection from matching tracks
        return MatchingIndices[FMath::RandRange(0, MatchingIndices.Num() - 1)];
    }

    const auto RandomValue = FMath::RandRange(0.0f, TotalWeight);
    auto CurrentWeight = 0.0f;

    for (const auto Index : MatchingIndices)
    {
        CurrentWeight += _Tracks[Index].Get_Weight();
        if (RandomValue <= CurrentWeight)
        {
            return Index;
        }
    }

    return MatchingIndices.Last(); // Fallback
}