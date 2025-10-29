#include "CkAudioCue_EntityScript.h"

#include "CkAudio/CkAudio_Log.h"
#include "CkAudio/AudioDirector/CkAudioDirector_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Probability/CkProbability_Utils.h"

#include "CkTimer/CkTimer_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Timer_AudioCueStartDelay, TEXT("Timer.AudioCue.StartDelay"));

// --------------------------------------------------------------------------------------------------------------------

UCk_AudioCue_EntityScript::
    UCk_AudioCue_EntityScript(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    this->_LifetimeBehavior = ECk_Cue_LifetimeBehavior::Custom;
}

auto
    UCk_AudioCue_EntityScript::
    Get_CueName_Implementation() const
    -> FGameplayTag
{
    if (GetClass() == StaticClass())
    { return TAG_Cue_DoNotExecute; }

    return Super::Get_CueName_Implementation();
}

auto
    UCk_AudioCue_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Ret = Super::Construct(InHandle, InSpawnParams);

    CK_ENSURE_IF_NOT(Get_IsConfigurationValid(),
        TEXT("AudioCue configuration is invalid for Entity [{}]"), InHandle)
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    const auto AudioDirectorParams = FCk_Fragment_AudioDirector_ParamsData{}
        .Set_DefaultCrossfadeDuration(_DefaultCrossfadeDuration)
        .Set_MaxConcurrentTracks(_MaxConcurrentTracks)
        .Set_SamePriorityBehavior(_SamePriorityBehavior);

    auto AudioDirectorHandle = UCk_Utils_AudioDirector_UE::Add(_AssociatedEntity, AudioDirectorParams);

    const auto LifetimeBehavior = Get_LifetimeBehavior();
    if (LifetimeBehavior == ECk_Cue_LifetimeBehavior::Persistent ||
        LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed)
    {
        if (Get_HasValidTrackLibrary())
        {
            for (const auto& TrackParams : _TrackLibrary)
            {
                UCk_Utils_AudioDirector_UE::Request_AddTrack(AudioDirectorHandle, TrackParams);
            }
        }
        if (Get_HasValidSingleTrack())
        {
            UCk_Utils_AudioDirector_UE::Request_AddTrack(AudioDirectorHandle, _SingleTrack);
        }
    }

    _RecentTracks.Empty();
    _LastSelectedIndex = INDEX_NONE;

    return Ret;
}

auto
    UCk_AudioCue_EntityScript::
    BeginPlay()
    -> void
{
    auto AudioDirectorHandle = UCk_Utils_AudioDirector_UE::Cast(_AssociatedEntity);

    if (Get_LifetimeBehavior() == ECk_Cue_LifetimeBehavior::Custom)
    {
        DoBindToAllTracksFinished(AudioDirectorHandle);
    }

    switch (_PlaybackBehavior)
    {
        case ECk_AudioCue_PlaybackBehavior::AutoPlay:
        {
            Request_Play();
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
            const auto TimerParams = FCk_Fragment_Timer_ParamsData{_DelayTime}
                .Set_TimerName(TAG_Label_Timer_AudioCueStartDelay)
                .Set_CountDirection(ECk_Timer_CountDirection::CountUp)
                .Set_Behavior(ECk_Timer_Behavior::PauseOnDone)
                .Set_StartingState(ECk_Timer_State::Running);

            auto DelayTimer = UCk_Utils_Timer_UE::Add(_AssociatedEntity, TimerParams);

            auto Delegate = FCk_Delegate_Timer{};
            Delegate.BindDynamic(this, &ThisType::OnDelayTimerComplete);
            UCk_Utils_Timer_UE::BindTo_OnDone(DelayTimer, Delegate);

            ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] will start playback after [{}] seconds"),
                Get_CueName(), _DelayTime.Get_Seconds());
            break;
        }
        default:
        {
            CK_INVALID_ENUM(_PlaybackBehavior);
            break;
        }
    }

    Super::BeginPlay();
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
    Request_Play();
    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] delayed playback started"), Get_CueName());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    Request_Play()
    -> void
{
    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] received Request_Play"), Get_CueName());
    DoSelectAndPlayTrack();
}

auto
    UCk_AudioCue_EntityScript::
    Request_Stop()
    -> void
{
    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] received Request_Stop"), Get_CueName());

    auto AudioDirectorHandle = UCk_Utils_AudioDirector_UE::Cast(_AssociatedEntity);
    UCk_Utils_AudioDirector_UE::Request_StopAllTracks(AudioDirectorHandle, _DefaultCrossfadeDuration);
}

auto
    UCk_AudioCue_EntityScript::
    Request_StopAll()
    -> void
{
    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] received Request_StopAll"), Get_CueName());

    auto AudioDirectorHandle = UCk_Utils_AudioDirector_UE::Cast(_AssociatedEntity);
    UCk_Utils_AudioDirector_UE::Request_StopAllTracks(AudioDirectorHandle, _DefaultCrossfadeDuration);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    Get_HasValidSingleTrack() const
    -> bool
{
    return ck::IsValid(_SingleTrack.Get_Sound()) && ck::IsValid(_SingleTrack.Get_TrackName());
}

auto
    UCk_AudioCue_EntityScript::
    Get_HasValidTrackLibrary() const
    -> bool
{
    if (_TrackLibrary.IsEmpty())
    { return false; }

    return _TrackLibrary.ContainsByPredicate([](const FCk_Fragment_AudioTrack_ParamsData& Track)
    {
        return ck::IsValid(Track.Get_Sound()) && ck::IsValid(Track.Get_TrackName());
    });
}

auto
    UCk_AudioCue_EntityScript::
    Get_IsConfigurationValid() const
    -> bool
{
    const auto& HasValidSingle = Get_HasValidSingleTrack();
    const auto& HasValidLibrary = Get_HasValidTrackLibrary();

    switch (_SourcePriority)
    {
        case ECk_AudioCue_SourcePriority::PreferSingleTrack:
        case ECk_AudioCue_SourcePriority::PreferLibrary:
        {
            return HasValidSingle || HasValidLibrary;
        }
        case ECk_AudioCue_SourcePriority::SingleTrackOnly:
        {
            return HasValidSingle && NOT HasValidLibrary;
        }
        case ECk_AudioCue_SourcePriority::LibraryOnly:
        {
            return HasValidLibrary && NOT HasValidSingle;
        }
        default:
        {
            CK_INVALID_ENUM(_SourcePriority);
            return false;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    Get_NextTrackIndex(
        const TArray<FGameplayTag>& InRecentTracks) const
    -> int32
{
    if (_TrackLibrary.IsEmpty())
    { return INDEX_NONE; }

    switch (_SelectionMode)
    {
        case ECk_AudioCue_SelectionMode::Random:
        {
            return DoGet_NextTrack_Random();
        }
        case ECk_AudioCue_SelectionMode::WeightedRandom:
        {
            return DoGet_NextTrack_WeightedRandom();
        }
        case ECk_AudioCue_SelectionMode::Sequential:
        {
            return DoGet_NextTrack_Sequential();
        }
        default:
        {
            CK_INVALID_ENUM(_SelectionMode);
            return DoGet_NextTrack_Random();
        }
    }
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_Random() const
    -> int32
{
    return FMath::RandRange(0, _TrackLibrary.Num() - 1);
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_WeightedRandom() const
    -> int32
{
    TArray<double> Weights;
    Weights.Reserve(_TrackLibrary.Num());

    for (const auto& Track : _TrackLibrary)
    {
        Weights.Add(FMath::Max(1.0, static_cast<double>(Track.Get_Priority())));
    }

    const auto& Result = UCk_Utils_Probability_UE::Get_RandomIndexByWeight(Weights);

    if (Result.Get_Index() != INDEX_NONE)
    {
        return Result.Get_Index();
    }

    return DoGet_NextTrack_Random();
}

auto
    UCk_AudioCue_EntityScript::
    DoGet_NextTrack_Sequential() const
    -> int32
{
    return DoGet_NextTrack_Random();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    DoSelectAndPlayTrack()
    -> void
{
    FCk_Fragment_AudioTrack_ParamsData SelectedTrack;
    bool TrackSelected = false;

    switch (_SourcePriority)
    {
        case ECk_AudioCue_SourcePriority::PreferSingleTrack:
        {
            if (Get_HasValidSingleTrack())
            {
                SelectedTrack = _SingleTrack;
                TrackSelected = true;
            }
            else if (Get_HasValidTrackLibrary())
            {
                const auto TrackIndex = Get_NextTrackIndex(_RecentTracks);
                if (TrackIndex != INDEX_NONE)
                {
                    SelectedTrack = _TrackLibrary[TrackIndex];
                    TrackSelected = true;
                    _LastSelectedIndex = TrackIndex;
                }
            }
            break;
        }
        case ECk_AudioCue_SourcePriority::PreferLibrary:
        {
            if (Get_HasValidTrackLibrary())
            {
                const auto TrackIndex = Get_NextTrackIndex(_RecentTracks);
                if (TrackIndex != INDEX_NONE)
                {
                    SelectedTrack = _TrackLibrary[TrackIndex];
                    TrackSelected = true;
                    _LastSelectedIndex = TrackIndex;
                }
            }
            else if (Get_HasValidSingleTrack())
            {
                SelectedTrack = _SingleTrack;
                TrackSelected = true;
            }
            break;
        }
        case ECk_AudioCue_SourcePriority::SingleTrackOnly:
        {
            if (Get_HasValidSingleTrack())
            {
                SelectedTrack = _SingleTrack;
                TrackSelected = true;
            }
            break;
        }
        case ECk_AudioCue_SourcePriority::LibraryOnly:
        {
            if (Get_HasValidTrackLibrary())
            {
                const auto TrackIndex = Get_NextTrackIndex(_RecentTracks);
                if (TrackIndex != INDEX_NONE)
                {
                    SelectedTrack = _TrackLibrary[TrackIndex];
                    TrackSelected = true;
                    _LastSelectedIndex = TrackIndex;
                }
            }
            break;
        }
    }

    CK_ENSURE_IF_NOT(TrackSelected,
        TEXT("AudioCue EntityScript [{}] could not select a valid track"), Get_CueName())
    { return; }

    auto StartTrackRequest = FCk_Request_AudioDirector_StartTrack{SelectedTrack.Get_TrackName()};
    StartTrackRequest.Set_FadeInTime(_DefaultCrossfadeDuration);

    auto AudioDirectorHandle = UCk_Utils_AudioDirector_UE::Cast(_AssociatedEntity);
    UCk_Utils_AudioDirector_UE::Request_AddTrack(AudioDirectorHandle, SelectedTrack);
    UCk_Utils_AudioDirector_UE::Request_StartTrack(AudioDirectorHandle, StartTrackRequest);

    _RecentTracks.Add(SelectedTrack.Get_TrackName());
    if (_RecentTracks.Num() > 10)
    {
        _RecentTracks.RemoveAt(0);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCue_EntityScript::
    DoBindToAllTracksFinished(
        FCk_Handle_AudioDirector InAudioDirectorHandle)
    -> void
{
    auto Delegate = FCk_Delegate_AudioDirector_AllTracksFinished{};
    Delegate.BindDynamic(this, &UCk_AudioCue_EntityScript::OnAllTracksFinished);

    UCk_Utils_AudioDirector_UE::BindTo_OnAllTracksFinished(InAudioDirectorHandle, Delegate);

    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] bound to OnAllTracksFinished for custom lifetime management"), Get_CueName());
}

auto
    UCk_AudioCue_EntityScript::
    OnAllTracksFinished(
        FCk_Handle_AudioDirector InAudioDirector)
    -> void
{
    ck::audio::Verbose(TEXT("AudioCue EntityScript [{}] received OnAllTracksFinished - destroying entity"), Get_CueName());

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
}

// --------------------------------------------------------------------------------------------------------------------
