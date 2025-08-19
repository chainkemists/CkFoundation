#include "CkAudioCue_EntityScript.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudioCue_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    Get_LifetimeBehavior() const
    -> ECk_Cue_LifetimeBehavior
{
    return UCk_CueBase_EntityScript::Get_LifetimeBehavior();
}

auto
    UCk_AudioCue_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    auto AudioCueHandle = UCk_Utils_AudioCue_UE::Add(_AssociatedEntity, *this);

    // Handle playback behavior
    switch (_PlaybackBehavior)
    {
        case ECk_AudioCue_PlaybackBehavior::AutoPlay:
        {
            UCk_Utils_AudioCue_UE::Request_Play(AudioCueHandle, TOptional<int32>{}, FCk_Time::ZeroSecond());
            ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] auto-started playback"), Get_CueName());
            break;
        }
        case ECk_AudioCue_PlaybackBehavior::Manual:
        {
            ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] waiting for manual play request"), Get_CueName());
            break;
        }
        case ECk_AudioCue_PlaybackBehavior::DelayedPlay:
        {
            // Create timer for delayed playback
            auto TimerParams = FCk_Fragment_Timer_ParamsData{_DelayTime}
                .Set_TimerName(UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("AudioCue.DelayTimer")))
                .Set_CountDirection(ECk_Timer_CountDirection::CountUp)
                .Set_Behavior(ECk_Timer_Behavior::PauseOnDone)
                .Set_StartingState(ECk_Timer_State::Running);

            auto DelayTimer = UCk_Utils_Timer_UE::Add(_AssociatedEntity, TimerParams);

            // Create delegate and bind to timer completion
            auto Delegate = FCk_Delegate_Timer{};
            Delegate.BindUFunction(this, FName("OnDelayTimerComplete"));
            UCk_Utils_Timer_UE::BindTo_OnDone(DelayTimer, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, Delegate);

            ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] will start playback after [{}] seconds"),
                Get_CueName(), _DelayTime.Get_Seconds());
            break;
        }
    }

    return Super::Construct(InHandle, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    OnDelayTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
    -> void
{
    auto AudioCueHandle = UCk_Utils_AudioCue_UE::Cast(_AssociatedEntity);

    if (NOT ck::IsValid(AudioCueHandle))
    { return; }

    UCk_Utils_AudioCue_UE::Request_Play(AudioCueHandle, TOptional<int32>{}, FCk_Time::ZeroSecond());
    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] delayed playback started"), Get_CueName());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    Get_HasValidSingleTrack() const -> bool
{
    return ck::IsValid(_SingleTrack.Get_Sound()) && _SingleTrack.Get_TrackName().IsValid();
}

auto
    UCk_AudioCue_EntityScript::
    Get_HasValidTrackLibrary() const -> bool
{
    if (_TrackLibrary.IsEmpty())
    { return false; }

    // Check if at least one track is valid
    return _TrackLibrary.ContainsByPredicate([](const FCk_Fragment_AudioTrack_ParamsData& Track)
    {
        return ck::IsValid(Track.Get_Sound()) && Track.Get_TrackName().IsValid();
    });
}

auto
    UCk_AudioCue_EntityScript::
    Get_IsConfigurationValid() const -> bool
{
    const auto HasValidSingle = Get_HasValidSingleTrack();
    const auto HasValidLibrary = Get_HasValidTrackLibrary();

    switch (_SourcePriority)
    {
        case ECk_AudioCue_SourcePriority::PreferSingleTrack:
        case ECk_AudioCue_SourcePriority::PreferLibrary:
            return HasValidSingle || HasValidLibrary;

        case ECk_AudioCue_SourcePriority::SingleTrackOnly:
            return HasValidSingle && NOT HasValidLibrary;

        case ECk_AudioCue_SourcePriority::LibraryOnly:
            return HasValidLibrary && NOT HasValidSingle;

        default:
            return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    Get_NextTrackIndex(const TArray<FGameplayTag>& InRecentTracks) const -> int32
{
    if (_TrackLibrary.IsEmpty())
    { return INDEX_NONE; }

    switch (_SelectionMode)
    {
        case ECk_AudioCue_SelectionMode::Random:
            return DoGet_NextTrack_Random();

        case ECk_AudioCue_SelectionMode::WeightedRandom:
            return DoGet_NextTrack_WeightedRandom();

        case ECk_AudioCue_SelectionMode::Sequential:
            return DoGet_NextTrack_Sequential();

        case ECk_AudioCue_SelectionMode::MoodBased:
            return DoGet_NextTrack_MoodBased(InRecentTracks);

        default:
            return DoGet_NextTrack_Random();
    }
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_Random() const -> int32
{
    return FMath::RandRange(0, _TrackLibrary.Num() - 1);
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_WeightedRandom() const -> int32
{
    auto TotalWeight = 0.0f;
    for (const auto& Track : _TrackLibrary)
    {
        // Use priority as weight fallback if no dedicated weight system
        TotalWeight += FMath::Max(1.0f, static_cast<float>(Track.Get_Priority()));
    }

    if (TotalWeight <= 0.0f)
    { return DoGet_NextTrack_Random(); }

    const auto RandomValue = FMath::RandRange(0.0f, TotalWeight);
    auto CurrentWeight = 0.0f;

    for (auto i = 0; i < _TrackLibrary.Num(); ++i)
    {
        CurrentWeight += FMath::Max(1.0f, static_cast<float>(_TrackLibrary[i].Get_Priority()));
        if (RandomValue <= CurrentWeight)
        { return i; }
    }

    return _TrackLibrary.Num() - 1; // Fallback to last track
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_Sequential() const -> int32
{
    // TODO: Implement sequential tracking with runtime state
    // For now, fallback to random
    return DoGet_NextTrack_Random();
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_MoodBased(const TArray<FGameplayTag>& InRecentTracks) const -> int32
{
    if (_ActiveMoodTags.IsEmpty())
    { return DoGet_NextTrack_WeightedRandom(); }

    // Find tracks that match current mood
    TArray<int32> MatchingIndices;

    for (auto i = 0; i < _TrackLibrary.Num(); ++i)
    {
        // For mood matching, we'd need additional mood data in track params
        // For now, include all tracks and use weighted selection
        MatchingIndices.Add(i);
    }

    if (MatchingIndices.IsEmpty())
    { return DoGet_NextTrack_WeightedRandom(); }

    // Apply weighted selection within matching tracks
    auto TotalWeight = 0.0f;
    for (const auto Index : MatchingIndices)
    {
        TotalWeight += FMath::Max(1.0f, static_cast<float>(_TrackLibrary[Index].Get_Priority()));
    }

    if (TotalWeight <= 0.0f)
    { return MatchingIndices[FMath::RandRange(0, MatchingIndices.Num() - 1)]; }

    const auto RandomValue = FMath::RandRange(0.0f, TotalWeight);
    auto CurrentWeight = 0.0f;

    for (const auto Index : MatchingIndices)
    {
        CurrentWeight += FMath::Max(1.0f, static_cast<float>(_TrackLibrary[Index].Get_Priority()));
        if (RandomValue <= CurrentWeight)
        { return Index; }
    }

    return MatchingIndices.Last(); // Fallback
}

// --------------------------------------------------------------------------------------------------------------------
