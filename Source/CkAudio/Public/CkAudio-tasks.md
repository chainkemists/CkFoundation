# CkAudio Module - Implementation Plan

## Phase 0: Fix Existing Architecture (Critical)

### 0. Update AudioDirector to use RecordOfEntities
**Purpose**: Remove separate registries, use single registry with RecordOfEntities pattern.

**Files to Update**:
- [ ] `CkAudioDirector_Fragment.h/cpp` - Replace registries with RecordOfEntities
- [ ] `CkAudioDirector_Processor.cpp` - Update entity creation logic
- [ ] `CkAudioDirector_Utils.cpp` - Update child entity management

**Changes Required**:
```cpp
// AudioDirector_Fragment.h - ADD
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTracks, FCk_Handle_AudioTrack);
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTimers, FCk_Handle_Timer);

// FFragment_AudioDirector_Current - REMOVE
FCk_Registry _TrackRegistry;
FCk_Registry _TimerRegistry;
// REMOVE methods: Request_CreateTrackEntity(), Request_CreateTimerEntity()
```

**Integration**: Ensures all audio entities stay in main registry for debugging/interoperability

---

## Phase 1: Essential Ambient Features (High Priority)

### 1. AudioTrackPool Feature - Entity-Based Track Selection
**Purpose**: Multiple tracks per tag with random/weighted selection, each track as separate entity.

**Files to Create**:
- [ ] `CkAudioTrackPool_Fragment_Data.h/cpp` - Handle, params, requests (config only)
- [ ] `CkAudioTrackPool_Fragment.h/cpp` - Runtime fragments, RecordOfEntities
- [ ] `CkAudioTrackPool_Utils.h/cpp` - Public API
- [ ] `CkAudioTrackPool_Processor.h/cpp` - Pool management, selection logic
- [ ] `CkAudioTrackPoolEntry_Fragment_Data.h/cpp` - Individual entry handle, params
- [ ] `CkAudioTrackPoolEntry_Fragment.h/cpp` - Entry runtime state
- [ ] `CkAudioTrackPoolEntry_Utils.h/cpp` - Entry management API
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add pool processors

**Key Components**:
```cpp
// Pool container
CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTrackPoolEntries, FCk_Handle_AudioTrackPoolEntry);
struct FFragment_AudioTrackPool_Current {
    FCk_Handle_AudioTrackPoolEntry _LastPlayedEntry;
    FCk_Time _LastPlayedTime;
    ECk_AudioTrackPool_SelectionAlgorithm _SelectionAlgorithm;
};

// Individual pool entries as entities
struct FCk_Fragment_AudioTrackPoolEntry_ParamsData {
    TObjectPtr<USoundBase> _Sound;
    float _SelectionWeight = 1.0f;
    FCk_Time _AvoidanceTime = FCk_Time{30.0f};
    bool _LazyLoad = true; // Per-entry choice
};
```

**Integration**: AudioDirector references pools instead of single tracks, uses RecordOfEntities pattern

---

### 2. AudioStinger Feature - One-Shot Audio
**Purpose**: Non-looping UI sounds, notifications, short musical phrases that don't interrupt ambient.

**Files to Create**:
- [ ] `CkAudioStinger_Fragment_Data.h/cpp` - Handle, params, requests
- [ ] `CkAudioStinger_Fragment.h/cpp` - Fragments, playback state
- [ ] `CkAudioStinger_Utils.h/cpp` - Public API
- [ ] `CkAudioStinger_Processor.h/cpp` - One-shot playback logic
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add stinger processors

**Key Components**:
- `FCk_Handle_AudioStinger` - Type-safe handle
- `ECk_AudioStinger_Priority` - UI/SFX/Musical priorities that don't affect background music
- `FCk_Request_AudioStinger_Play` - Immediate play request
- Separate priority system from main music tracks

**Integration**: AudioDirector can manage stingers alongside music tracks

---

### 3. LocationTrigger Feature - Area-Based Audio
**Purpose**: Different ambient tracks per game area with smooth transitions.

**Files to Create**:
- [ ] `CkAudioLocationTrigger_Fragment_Data.h/cpp` - Handle, params, location contexts
- [ ] `CkAudioLocationTrigger_Fragment.h/cpp` - Fragments, area management
- [ ] `CkAudioLocationTrigger_Utils.h/cpp` - Public API for location changes
- [ ] `CkAudioLocationTrigger_Processor.h/cpp` - Area detection, transition logic
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add location processors

**Key Components**:
- `FCk_Fragment_LocationTrigger_ParamsData` - Area configs (gameplay tag → track/pool mapping)
- `FCk_Request_LocationTrigger_SetArea` - Area change request
- Cross-fade logic when entering/exiting areas
- Default fallback tracks for unconfigured areas

**Integration**: Works with AudioDirector to switch tracks based on gameplay tags

---

### 4. AudioLayerManager Feature - Layered Audio System
**Purpose**: Multiple simultaneous layers (base + instruments + percussion) with add/remove capability.

**Files to Create**:
- [ ] `CkAudioLayerManager_Fragment_Data.h/cpp` - Handle, params, layer configs
- [ ] `CkAudioLayerManager_Fragment.h/cpp` - Fragments, layer registry
- [ ] `CkAudioLayerManager_Utils.h/cpp` - Public API for layer control
- [ ] `CkAudioLayerManager_Processor.h/cpp` - Layer synchronization, enable/disable logic
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add layer processors

**Key Components**:
- `FCk_Fragment_LayerManager_ParamsData` - Layer configurations (base, additive layers)
- `FCk_Request_LayerManager_EnableLayer/DisableLayer` - Layer control
- Synchronized start/stop across layers
- Volume mixing for multiple simultaneous tracks

**Integration**: AudioDirector can manage LayerManager entities as special track types

---

## Phase 2: Enhanced Experience Features (Medium Priority)

### 5. AudioSegment Feature - Intro/Loop/Outro Support
**Purpose**: Smart looping with intro/outro sections for seamless transitions.

**Files to Create**:
- [ ] `CkAudioSegment_Fragment_Data.h/cpp` - Segment definitions, playback state
- [ ] `CkAudioSegment_Fragment.h/cpp` - Fragments, segment management
- [ ] `CkAudioSegment_Utils.h/cpp` - Public API
- [ ] `CkAudioSegment_Processor.h/cpp` - Segment sequencing, loop logic
- [ ] Update existing AudioTrack to support segment-based tracks
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add segment processors

**Key Components**:
- `FCk_AudioSegment_Config` - Intro/loop/outro sound references
- Smart transition logic between segments
- Conditional segment playback (skip intro if already playing)

**Integration**: Extends AudioTrack system with segment awareness

---

### 6. EnvironmentalContext Feature - Weather/Time Integration
**Purpose**: Audio changes based on environmental conditions and time-of-day.

**Files to Create**:
- [ ] `CkAudioEnvironmentalContext_Fragment_Data.h/cpp` - Context definitions
- [ ] `CkAudioEnvironmentalContext_Fragment.h/cpp` - Fragments, context state
- [ ] `CkAudioEnvironmentalContext_Utils.h/cpp` - Public API for context updates
- [ ] `CkAudioEnvironmentalContext_Processor.h/cpp` - Context evaluation, track selection
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add environmental processors

**Key Components**:
- `FCk_EnvironmentalContext_Config` - Weather/time → track/layer mappings
- `FCk_Request_EnvironmentalContext_Update` - Context change requests
- Smooth transitions between environmental states
- Context priority system (weather overrides time-of-day)

**Integration**: Works with AudioDirector and LayerManager for context-aware audio

---

### 7. PlayerStateContext Feature - Adaptive Audio
**Purpose**: Music responds to player health, inventory, quest status, emotional state.

**Files to Create**:
- [ ] `CkAudioPlayerContext_Fragment_Data.h/cpp` - Player state definitions
- [ ] `CkAudioPlayerContext_Fragment.h/cpp` - Fragments, state tracking
- [ ] `CkAudioPlayerContext_Utils.h/cpp` - Public API for state updates
- [ ] `CkAudioPlayerContext_Processor.h/cpp` - State evaluation, adaptive logic
- [ ] Update `CkAudio_ProcessorInjector.h/cpp` - Add player context processors

**Key Components**:
- `FCk_PlayerContext_Config` - State → audio mappings (health states, quest phases)
- `FCk_Request_PlayerContext_Update` - Player state change requests
- Dynamic priority adjustments based on player state
- Contextual audio intensity scaling

**Integration**: Modifies AudioDirector track priorities based on player state

---

## Phase 3: Advanced Features (Lower Priority)

### 8. AudioSaveState Feature - Persistence Support
**Purpose**: Save/load current audio state across game sessions.

**Files to Create**:
- [ ] `CkAudioSaveState_Fragment_Data.h/cpp` - Serializable state data
- [ ] `CkAudioSaveState_Utils.h/cpp` - Save/load API
- [ ] `CkAudioSaveState_Processor.h/cpp` - State serialization logic
- [ ] Integration with game save system

**Key Components**:
- Serializable current track states, volumes, fade states
- Position persistence for long tracks
- Context state persistence (location, environmental, player)

---

### 9. AudioSynchronization Feature - Beat Matching
**Purpose**: Beat-matching, tempo sync, musical transition timing for rhythm games.

**Files to Create**:
- [ ] `CkAudioSynchronization_Fragment_Data.h/cpp` - Timing definitions
- [ ] `CkAudioSynchronization_Utils.h/cpp` - Sync API
- [ ] `CkAudioSynchronization_Processor.h/cpp` - Beat detection, sync logic
- [ ] Integration with existing audio systems

**Key Components**:
- BPM detection and tracking
- Beat-aligned transition timing
- Tempo synchronization across tracks

---

### 10. AudioModifier Feature - Dynamic Processing
**Purpose**: Pitch/tempo changes, dynamic audio effects based on game state.

**Files to Create**:
- [ ] `CkAudioModifier_Fragment_Data.h/cpp` - Modifier definitions
- [ ] `CkAudioModifier_Utils.h/cpp` - Modifier API
- [ ] `CkAudioModifier_Processor.h/cpp` - Real-time audio processing
- [ ] Integration with UAudioComponent parameter system

**Key Components**:
- Real-time pitch/tempo modification
- Game speed → audio speed synchronization
- Dynamic filter and effect application

---

### 11. AudioHints Feature - Gameplay Guidance
**Purpose**: Subtle audio cues for gameplay guidance, directional hints.

**Files to Create**:
- [ ] `CkAudioHints_Fragment_Data.h/cpp` - Hint definitions, 3D positioning
- [ ] `CkAudioHints_Utils.h/cpp` - Hints API
- [ ] `CkAudioHints_Processor.h/cpp` - Spatial audio hints, context detection
- [ ] Integration with game guidance systems

**Key Components**:
- Context-sensitive audio feedback
- 3D positioned directional hints
- Subtle guidance without breaking immersion

---

## Integration Tasks

### Update AudioDirector for New Features
- [ ] Extend `FCk_Request_AudioDirector_*` to support pools, layers, contexts
- [ ] Update AudioDirector processor to handle new entity types
- [ ] Add context-aware track selection logic
- [ ] Implement priority system for different audio types (music vs stingers vs hints)

### Update Build Configuration
- [ ] Add new modules to `CkAudio.Build.cs`
- [ ] Update processor injectors for all new features
- [ ] Add any additional dependencies (TimerMixer integration, etc.)

### Testing & Validation
- [ ] Create test scenarios for each feature
- [ ] Performance testing with multiple simultaneous audio systems
- [ ] Memory usage validation for large track pools
- [ ] Integration testing with existing game systems

### Documentation & Examples
- [ ] Update `CkAudio-spec.md` with implemented features
- [ ] Create usage examples for each new feature
- [ ] Blueprint integration documentation
- [ ] Performance optimization guidelines

---

## Implementation Priority Recommendations

**Start with Phase 1** - These features provide immediate value for ambient audio variety and essential game audio needs.

**AudioTrackPool** should be first - it's foundational and many other systems can build on pool-based selection.

**AudioStinger** second - essential for UI/SFX that doesn't interfere with music.

**LocationTrigger** third - enables area-based ambient audio switching.

**AudioLayerManager** fourth - provides rich, adaptive soundscapes.

Each feature follows the established CkAudio patterns:
- ECS-based with fragments, processors, utils
- Type-safe handles
- Request-based API
- Signal system for events
- Integration with existing AudioDirector orchestration

The modular design allows implementing features independently while maintaining consistency with the existing codebase architecture.