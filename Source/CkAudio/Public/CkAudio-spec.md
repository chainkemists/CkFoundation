# 🎵 CkAudio Module - Complete Technical Specification

## Architecture Overview

The CkAudio module implements a priority-based music management system following Ck framework patterns. It uses parent-child entity relationships where **AudioDirector** entities orchestrate **AudioTrack** child entities.

### Entity Structure
- **Parent Entity** (Player/Game Object) → owns **AudioDirector Entity**
- **AudioDirector Entity** → owns **AudioTrack Entities** (one per configured track)
- **AudioDirector Entity** → owns **Timer Entities** (for cooldowns via existing CkTimer feature)

## Core Features

### 1. AudioTrack Feature
**Purpose**: Individual music track with playback control and metadata.

**Files**:
- `CkAudioTrack_Fragment_Data.h/cpp` - Handle, params, requests
- `CkAudioTrack_Fragment.h/cpp` - Fragments, tags, signals
- `CkAudioTrack_Utils.h/cpp` - Public API
- `CkAudioTrack_Processor.h/cpp` - Setup, requests, playback, teardown

**Key Components**:
- `FCk_Handle_AudioTrack` - Type-safe handle
- `FCk_Fragment_AudioTrack_ParamsData` - Track config (sound, priority, fade times, loop, volume, gameplay tag)
- `FFragment_AudioTrack_Current` - Runtime state (UAudioComponent, volume, fade state)
- `FFragment_AudioTrack_Requests` - Play/stop/volume requests
- `ECk_AudioTrack_OverrideBehavior` - Interrupt/Crossfade/Queue

### 2. AudioDirector Feature
**Purpose**: Orchestrates multiple tracks with priority-based switching.

**Files**:
- `CkAudioDirector_Fragment_Data.h/cpp` - Handle, params, requests
- `CkAudioDirector_Fragment.h/cpp` - Fragments, child entity management
- `CkAudioDirector_Utils.h/cpp` - Public API
- `CkAudioDirector_Processor.h/cpp` - Setup, orchestration logic

**Key Components**:
- `FCk_Handle_AudioDirector` - Type-safe handle
- `FCk_Fragment_AudioDirector_ParamsData` - Config (crossfade duration, max concurrent tracks)
- `FFragment_AudioDirector_Current` - Child registries, track map, current priority
- `FFragment_AudioDirector_Requests` - Add/start/stop track requests

## Primary API Usage

### Setup
```cpp
// Add director to entity
auto Director = UCk_Utils_AudioDirector_UE::Add(PlayerEntity, DirectorParams);

// Add tracks with different priorities
auto CombatTrack = FCk_Fragment_AudioTrack_ParamsData{TAG_Combat, CombatSound}
    .Set_Priority(100).Set_OverrideBehavior(ECk_AudioTrack_OverrideBehavior::Interrupt);
auto AmbientTrack = FCk_Fragment_AudioTrack_ParamsData{TAG_Ambient, AmbientSound}
    .Set_Priority(10).Set_OverrideBehavior(ECk_AudioTrack_OverrideBehavior::Crossfade);

UCk_Utils_AudioDirector_UE::Request_AddTrack(Director, CombatTrack);
UCk_Utils_AudioDirector_UE::Request_AddTrack(Director, AmbientTrack);
```

### Triggering Music (Main Interface)
```cpp
// Start track - respects priority system and override behaviors
UCk_Utils_AudioDirector_UE::Request_StartTrack(Director, TAG_Combat);

// Stop specific track
UCk_Utils_AudioDirector_UE::Request_StopTrack(Director, TAG_Combat);

// Emergency stop all
UCk_Utils_AudioDirector_UE::Request_StopAllTracks(Director);
```

## Priority & Override System

### Priority Rules
- **Higher number = Higher priority**
- **Director tracks current highest priority**
- **Override behavior per track determines conflict resolution**

### Override Behaviors
- **Interrupt**: Stop lower priority tracks immediately
- **Crossfade**: Fade out lower priority over crossfade duration while fading in new track
- **Queue**: Allow current tracks to finish naturally (framework ready)

### Priority Logic Flow
1. External system calls `Request_StartTrack(TrackName)`
2. Director finds track, gets effective priority (param or override)
3. If priority > current highest OR (== current highest AND allow same priority):
   - Apply override behavior to conflicting tracks
   - Start new track with appropriate fade
   - Update current highest priority
4. Otherwise ignore request

## Framework Integration

### Logging
- Uses `ck::audio::*` macros (Verbose, VeryVerbose, Warning, Error)
- Example: `ck::audio::Verbose(TEXT("Starting track [{}]"), TrackName);`

### Dependencies
- **CkTimer**: For cooldown/fadeout timers
- **AudioMixer**: For UAudioComponent
- **Standard Ck modules**: CkCore, CkEcs, CkEcsExt, CkLabel

### Processor Injection
- `UCk_AudioTrack_ProcessorInjector_UE` - Track processors
- `UCk_AudioDirector_ProcessorInjector_UE` - Director processors

### Signal System
```cpp
// Director events
UUtils_Signal_OnAudioDirector_TrackStarted::Broadcast(Director, TrackName, TrackHandle);
UUtils_Signal_OnAudioDirector_TrackStopped::Broadcast(Director, TrackName, TrackHandle);

// Track events
UUtils_Signal_OnAudioTrack_PlaybackStarted::Broadcast(Track, State);
UUtils_Signal_OnAudioTrack_FadeCompleted::Broadcast(Track, Volume, State);
```

## Implementation Details

### Child Entity Management
```cpp
// AudioDirector creates child entities (like 2dGridSystem pattern)
auto TrackEntity = InCurrent.Request_CreateTrackEntity();
UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(TrackEntity, DirectorEntity);
auto TrackHandle = UCk_Utils_AudioTrack_UE::Create(DirectorEntity, TrackParams);
```

### Volume Fading
- Processors handle smooth volume transitions over time
- Configurable fade speeds per track
- State machine: Stopped → FadingIn → Playing → FadingOut → Stopped

### Timer Integration
Ready for integration with existing CkTimer for:
- Combat music fadeout after inactivity
- Cooldown prevention between track triggers
- Scheduled music changes

## Missing Features for Game Audio

### Unreal Engine Built-in (No Custom Implementation Needed)
- ✅ **3D Spatial Audio** - UAudioComponent positioning, attenuation, spatialization
- ✅ **Audio Occlusion/Reverb** - Built-in occlusion system and reverb zones
- ✅ **Audio Categories/Buses** - Sound Classes and Audio Mixer for volume control
- ✅ **Dynamic Range Compression** - Audio Mixer effects and sound class processing
- ✅ **Audio Streaming** - Automatic with sound wave compression settings
- ✅ **Ducking System** - Sound Class ducking built-in
- ✅ **Audio Events Queue** - Engine handles automatically

### Need Custom Implementation

#### High Priority (Essential for Ambient Audio)

**1. Playlist/Pool Support**
- Random selection from track arrays instead of single tracks
- Weighted random selection based on context
- Avoid recently played tracks
```cpp
FCk_Fragment_AudioTrackPool_ParamsData {
    TArray<USoundBase*> _TrackPool;
    float _RecentTrackAvoidanceTime;
    TMap<FGameplayTag, float> _WeightedSelection;
}
```

**2. Stingers/One-shots**
- Non-looping audio cues that don't interrupt ambient tracks
- UI sounds, notification audio, short musical phrases
- Priority system that doesn't affect background music
```cpp
UCk_Utils_AudioDirector_UE::Request_PlayStinger(Director, TAG_UIClick, Volume);
```

**3. Layered Audio**
- Multiple simultaneous audio layers (base + instruments + percussion)
- Add/remove layers based on game state
- Synchronized start/stop across layers
```cpp
UCk_Utils_AudioDirector_UE::Request_EnableLayer(Director, TAG_BaseAmbient, TAG_Layer_Percussion);
```

**4. Location-based Triggers**
- Different ambient tracks per game area
- Smooth transitions when crossing boundaries
- Area-specific audio parameters
```cpp
UCk_Utils_AudioDirector_UE::Request_SetLocationContext(Director, TAG_Area_Forest);
```

#### Medium Priority (Enhanced Experience)

**5. Adaptive Segments**
- Intro/loop/outro sections for seamless transitions
- Smart looping points within tracks
- Conditional segment playback
```cpp
FCk_AudioSegment {
    USoundBase* _IntroSegment;
    USoundBase* _LoopSegment;
    USoundBase* _OutroSegment;
    bool _RequiresIntro;
}
```

**6. Weather/Time Integration**
- Audio changes based on environmental conditions
- Time-of-day audio variations
- Weather-specific ambient layers
```cpp
UCk_Utils_AudioDirector_UE::Request_SetEnvironmentalContext(Director, Weather, TimeOfDay);
```

**7. Player State Awareness**
- Music responds to health, inventory, quest status
- Dynamic audio based on player progression
- Emotional state audio matching
```cpp
UCk_Utils_AudioDirector_UE::Request_SetPlayerContext(Director, HealthState, QuestState);
```

#### Lower Priority (Polish Features)

**8. Save/Load State**
- Persist current audio state across game sessions
- Resume music from last position
- Save environmental audio contexts

**9. Crowd/Population Density**
- Audio intensity based on NPC count
- Procedural ambient crowd sounds
- Population-based audio mixing

**10. Audio Synchronization**
- Beat-matching for rhythm-sensitive games
- Tempo synchronization across tracks
- Musical transition timing

**11. Audio Modifiers**
- Pitch/tempo changes based on game speed
- Player ability-based audio effects
- Dynamic audio processing

**12. Conditional Layering**
- Complex rule-based layer management
- State machine for audio layers
- Advanced conditional audio logic

**13. Audio Hints System**
- Subtle audio cues for gameplay guidance
- Directional audio hints
- Context-sensitive audio feedback

## Implementation Roadmap

### Phase 1 (Current - ✅ Complete)
- Basic AudioTrack and AudioDirector system
- Priority-based track switching
- Volume fading and crossfading
- Signal system for events

### Phase 2 (Next Priority)
1. **Playlist/Pool Support** - Essential for ambient variety
2. **Stingers/One-shots** - UI and notification audio
3. **Location-based Triggers** - Area-specific ambient audio
4. **Layered Audio** - Rich, adaptive soundscapes

### Phase 3 (Enhancement)
- Adaptive segments for seamless looping
- Environmental context integration
- Player state-aware audio
- Save/load state persistence

### Phase 4 (Polish)
- Advanced conditional layering
- Audio synchronization features
- Procedural audio generation
- Performance optimizations

## Current Status

**✅ Fully Implemented**: AudioTrack and AudioDirector features with complete ECS integration, priority system, fading, and signal events.

**🔄 Ready for Integration**: External systems can trigger music via gameplay tags. Timer-based features can be added using existing CkTimer.

**📋 Next Steps**:
1. Add playlist/pool support for ambient variety
2. Implement stinger system for UI/SFX
3. Integrate with game systems (combat, location, time-of-day)
4. Add layered audio support for adaptive soundscapes