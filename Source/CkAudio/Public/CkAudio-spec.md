# 🎵 CkAudio Module - Complete Technical Specification

## Architecture Overview

The CkAudio module implements a priority-based music management system following Ck framework patterns. It uses parent-child entity relationships where **AudioDirector** entities orchestrate **AudioTrack** child entities. All entities exist within the same registry for debugging and interoperability.

### Entity Structure
- **Parent Entity** (Player/Game Object) → owns **AudioDirector Entity**
- **AudioDirector Entity** → owns **AudioTrack Entities** (one per configured track)
- **AudioDirector Entity** → owns **Timer Entities** (for cooldowns via existing CkTimer feature)
- **AudioTrackPool Entity** → owns **AudioTrackPoolEntry Entities** (multiple tracks per pool)

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
- `FFragment_AudioDirector_Current` - Track registry using RecordOfEntities, track map, current priority
- `FFragment_AudioDirector_Requests` - Add/start/stop track requests

**Updated Architecture** (Single Registry Pattern):
```cpp
// AudioDirector_Fragment.h
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTracks, FCk_Handle_AudioTrack);
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTimers, FCk_Handle_Timer);

struct FFragment_AudioDirector_Current {
    // Uses RecordOfEntities instead of separate registries
    int32 _CurrentHighestPriority = -1;
    TMap<FGameplayTag, FCk_Handle_AudioTrack> _TracksByName; // Fast lookup by tag
};
```

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
- **CkAttribute**: For meters that refill automatically (combat intensity, ambient variety, etc.)
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

### Child Entity Management (Updated)
```cpp
// AudioDirector uses RecordOfEntities instead of separate registries
RecordOfAudioTracks_Utils::Request_Connect(DirectorEntity, NewTrackEntity);
auto TrackHandle = UCk_Utils_AudioTrack_UE::Create(DirectorEntity, TrackParams);
```

### Volume Fading
- Processors handle smooth volume transitions over time
- Configurable fade speeds per track
- State machine: Stopped → FadingIn → Playing → FadingOut → Stopped

### Timer Integration
Uses existing CkTimer for:
- Combat music fadeout after inactivity
- Cooldown prevention between track triggers
- Scheduled music changes

### Attribute Integration
Uses existing CkAttribute (Float) for meters that auto-refill:
- Combat intensity meter (drives audio layer intensity)
- Ambient variety meter (influences track selection frequency)
- Player state meters (health, stamina affecting audio)

## Planned Features for Game Audio

### Phase 1: Essential Ambient Features (High Priority)

#### 1. AudioTrackPool Feature - Random Track Selection
**Purpose**: Multiple tracks per tag with random/weighted selection to avoid repetition.

**Architecture** (Updated):
```cpp
// AudioTrackPool_Fragment.h
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTrackPoolEntries, FCk_Handle_AudioTrackPoolEntry);

struct FFragment_AudioTrackPool_Current {
    // Uses RecordOfEntities - no separate registry
    FCk_Handle_AudioTrackPoolEntry _LastPlayedEntry;
    FCk_Time _LastPlayedTime;
    ECk_AudioTrackPool_SelectionAlgorithm _SelectionAlgorithm; // Random, Weighted, AvoidRecent
};

// Pool entries as separate entities
struct FFragment_AudioTrackPoolEntry_ParamsData {
    TObjectPtr<USoundBase> _Sound;
    float _SelectionWeight = 1.0f;
    FCk_Time _AvoidanceTime = FCk_Time{30.0f};
    bool _LazyLoad = true; // Per-entry configurable
};
```

**Key Components**:
- `FCk_Handle_AudioTrackPool` - Type-safe handle
- Pool entries as child entities using RecordOfEntities pattern
- Multiple selection algorithms (strategy pattern)
- Per-entry lazy loading choice
- Each track is its own entity with own state/metadata

**Integration**: AudioDirector references pools instead of single tracks

#### 2. AudioStinger Feature - One-Shot Audio
**Purpose**: Non-looping UI sounds, notifications, short musical phrases that don't interrupt ambient.

**Key Components**:
- `FCk_Handle_AudioStinger` - Type-safe handle
- `ECk_AudioStinger_Priority` - UI/SFX/Musical priorities separate from music tracks
- `FCk_Request_AudioStinger_Play` - Immediate play request
- Separate priority system from main music tracks

**Integration**: AudioDirector manages stingers alongside music tracks

#### 3. LocationTrigger Feature - Area-Based Audio
**Purpose**: Different ambient tracks per game area with smooth transitions.

**Key Components**:
- `FCk_Fragment_LocationTrigger_ParamsData` - Area configs (gameplay tag → track/pool mapping)
- `FCk_Request_LocationTrigger_SetArea` - Area change request
- Cross-fade logic when entering/exiting areas
- Default fallback tracks for unconfigured areas

**Integration**: Works with AudioDirector to switch tracks based on gameplay tags

#### 4. AudioLayerManager Feature - Layered Audio System
**Purpose**: Multiple simultaneous layers (base + instruments + percussion) with add/remove capability.

**Key Components**:
- `FCk_Fragment_LayerManager_ParamsData` - Layer configurations (base, additive layers)
- `FCk_Request_LayerManager_EnableLayer/DisableLayer` - Layer control
- Synchronized start/stop across layers
- Volume mixing for multiple simultaneous tracks

**Integration**: AudioDirector manages LayerManager entities as special track types

### Phase 2: Enhanced Experience Features (Medium Priority)

#### 5. AudioSegment Feature - Intro/Loop/Outro Support
**Purpose**: Smart looping with intro/outro sections for seamless transitions.

**Key Components**:
- `FCk_AudioSegment_Config` - Intro/loop/outro sound references
- Smart transition logic between segments
- Conditional segment playback (skip intro if already playing)

**Integration**: Extends AudioTrack system with segment awareness

#### 6. EnvironmentalContext Feature - Weather/Time Integration
**Purpose**: Audio changes based on environmental conditions and time-of-day.

**Key Components**:
- `FCk_EnvironmentalContext_Config` - Weather/time → track/layer mappings
- `FCk_Request_EnvironmentalContext_Update` - Context change requests
- Smooth transitions between environmental states
- Context priority system (weather overrides time-of-day)

**Integration**: Works with AudioDirector and LayerManager for context-aware audio

#### 7. PlayerStateContext Feature - Adaptive Audio
**Purpose**: Music responds to player health, inventory, quest status, emotional state.

**Key Components**:
- `FCk_PlayerContext_Config` - State → audio mappings (health states, quest phases)
- `FCk_Request_PlayerContext_Update` - Player state change requests
- Dynamic priority adjustments based on player state
- Contextual audio intensity scaling
- Integration with CkAttribute meters for automatic state tracking

**Integration**: Modifies AudioDirector track priorities based on player state

### Phase 3: Advanced Features (Lower Priority)

#### 8. AudioSaveState Feature - Persistence Support
**Purpose**: Save/load current audio state across game sessions.

#### 9. AudioSynchronization Feature - Beat Matching
**Purpose**: Beat-matching, tempo sync, musical transition timing for rhythm games.

#### 10. AudioModifier Feature - Dynamic Processing
**Purpose**: Pitch/tempo changes, dynamic audio effects based on game state.

#### 11. AudioHints Feature - Gameplay Guidance
**Purpose**: Subtle audio cues for gameplay guidance, directional hints.

### Unreal Engine Built-in (No Custom Implementation Needed)
- ✅ **3D Spatial Audio** - UAudioComponent positioning, attenuation, spatialization
- ✅ **Audio Occlusion/Reverb** - Built-in occlusion system and reverb zones
- ✅ **Audio Categories/Buses** - Sound Classes and Audio Mixer for volume control
- ✅ **Dynamic Range Compression** - Audio Mixer effects and sound class processing
- ✅ **Audio Streaming** - Automatic with sound wave compression settings
- ✅ **Ducking System** - Sound Class ducking built-in
- ✅ **Audio Events Queue** - Engine handles automatically

## Architecture Patterns

### ECS Design Philosophy
- **No TMaps where possible** - Convert to separate entities with fragments instead
- **Single Registry** - All entities in same registry for debugging and interoperability
- **RecordOfEntities** - Use for parent-child relationships instead of separate registries
- **Entity-per-item** - Pool entries, layers, segments as individual entities
- **Fragment separation**:
  - `Fragment_Data.h` - Configuration, requests, handles, enums only
  - `Fragment.h` - Runtime state, friend classes, methods

### Exception Cases for Separate Registries
- **Grid Systems** - Due to grid cell IDs and scale concerns only
- **Not for audio systems** - All audio entities stay in main registry

## Current Status

**✅ Fully Implemented**: AudioTrack and AudioDirector features with complete ECS integration, priority system, fading, and signal events.

**🔄 Next Update Needed**:
1. Remove separate registries from AudioDirector_Current
2. Implement RecordOfEntities pattern for AudioDirector child management

**📋 Next Implementation Priority**:
1. **AudioTrackPool** - Essential for ambient variety, uses RecordOfEntities pattern
2. **AudioStinger** - UI/SFX that doesn't interfere with music
3. **LocationTrigger** - Area-specific ambient audio
4. **AudioLayerManager** - Rich, adaptive soundscapes

**🔧 Framework Integration**:
- CkTimer for cooldowns and scheduling
- CkAttribute for auto-refilling meters (combat intensity, ambient variety)
- Single registry for all audio entities (debugging and interoperability)
- RecordOfEntities for parent-child relationships throughout