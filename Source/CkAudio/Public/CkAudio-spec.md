🎵 Audio Director - Technical Implementation Spec
=================================================

Module Structure
----------------

-   **Module**: `CkAudio`
-   **Primary Feature**: `AudioDirector`
-   **Supporting Features**: `AudioTrack`

Architecture Overview
---------------------

### Entity Structure

Following the Ck framework pattern (similar to Timer/Grid systems), the audio system uses a **parent-child entity relationship**:

-   **AudioDirector Entity**: Parent entity that orchestrates music playback
-   **AudioTrack Entities**: Child entities (one per track) owned by the AudioDirector
-   **Timer Entities**: Child entities owned by AudioDirector for cooldowns/fadeouts (using existing `CkTimer` feature)

This mirrors how `2dGridSystem` owns `2dGridCell` entities or how entities own `Timer` entities.

Core Features Breakdown
-----------------------

### 1\. AudioDirector Feature

**Purpose**: Orchestrates music playback with priority-based track switching and crossfading.

**Components**:

-   `FFragment_AudioDirector_Params` - Configuration (default crossfade duration, max concurrent tracks)
-   `FFragment_AudioDirector_Current` - Runtime state (child registry for tracks/timers, current priority track)
-   `FFragment_AudioDirector_Requests` - Pending trigger requests
-   `FProcessor_AudioDirector_HandleRequests` - Processes trigger requests and manages track lifecycle
-   `FProcessor_AudioDirector_UpdatePlayback` - Handles crossfading coordination

**Key Responsibilities**:

-   Create/destroy AudioTrack child entities
-   Route trigger requests to appropriate tracks
-   Manage priority-based overrides
-   Coordinate crossfading between tracks
-   Create Timer entities for cooldowns (combat fadeout, etc.)

### 2\. AudioTrack Feature

**Purpose**: Individual music track with playback control and metadata.

**Components**:

-   `FFragment_AudioTrack_Params` - Track configuration (USoundBase, priority, fade behavior, loop settings, gameplay tag identifier)
-   `FFragment_AudioTrack_Current` - Playback state (UAudioComponent reference, current volume, fade targets)
-   `FFragment_AudioTrack_Requests` - Playback requests (play, stop, fade, volume change)
-   `FProcessor_AudioTrack_Playback` - Handles UAudioComponent creation/destruction and volume fading
-   `FProcessor_AudioTrack_HandleRequests` - Processes playback requests

**Key Responsibilities**:

-   Manage individual UAudioComponent lifecycle
-   Handle volume fading (in/out)
-   Track playback state and notify AudioDirector of completion
-   Support different override behaviors (interrupt, crossfade, queue)

Data Flow & Entity Relationships
--------------------------------

### Entity Creation Flow

1.  **Game calls** `UCk_Utils_AudioDirector_UE::Add(PlayerEntity, DirectorParams)`
2.  **AudioDirector** entity created as child of PlayerEntity
3.  **Track Configuration**: Tracks added via `UCk_Utils_AudioDirector_UE::Request_AddTrack(Director, TrackParams)`
4.  **AudioTrack entities** created as children of AudioDirector (similar to Timer pattern)
5.  **Timer entities** created as children of AudioDirector for cooldowns

### Trigger Flow

1.  **External System** → `UCk_Utils_AudioDirector_UE::Request_StartTrack(DirectorEntity, GameplayTag, Priority)`
2.  **AudioDirector** → Finds track by tag, checks priority vs current tracks
3.  **AudioDirector** → Creates Timer for fadeout duration if needed
4.  **AudioDirector** → Routes play/stop requests to specific AudioTrack entities
5.  **AudioTrack** → Manages UAudioComponent and reports status back to AudioDirector

### Child Entity Management

Following established patterns:

cpp

```
// AudioDirector creates child entities (like 2dGridSystem does for cells)
auto TrackEntity = DirectorEntity.Get<FFragment_AudioDirector_Current>().Request_CreateTrackEntity();
UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(TrackEntity, DirectorEntity);

// Timers created as children (existing pattern)
auto FadeoutTimer = UCk_Utils_Timer_UE::Add(DirectorEntity, TimerParams);
```

Key Interfaces
--------------

### AudioDirector Utils

cpp

```
// Director management
UCk_Utils_AudioDirector_UE::Add(Entity, DirectorParams) -> FCk_Handle_AudioDirector
UCk_Utils_AudioDirector_UE::Request_AddTrack(Director, TrackParams) -> FCk_Handle_AudioTrack

// Triggering (primary interface for external systems)
UCk_Utils_AudioDirector_UE::Request_StartTrack(Director, GameplayTag, Priority)
UCk_Utils_AudioDirector_UE::Request_StopTrack(Director, GameplayTag)
UCk_Utils_AudioDirector_UE::Request_StopAllTracks(Director)
```

### AudioTrack Utils

cpp

```
// Track lifecycle (mostly internal, called by AudioDirector)
UCk_Utils_AudioTrack_UE::Request_Play(TrackHandle, FadeInTime)
UCk_Utils_AudioTrack_UE::Request_Stop(TrackHandle, FadeOutTime)
UCk_Utils_AudioTrack_UE::Request_SetVolume(TrackHandle, Volume, FadeTime)
```

Priority System & Override Behaviors
------------------------------------

### Priority Rules

-   **Higher number = Higher priority**
-   **Configurable per track** in AudioTrack_Params
-   **Director maintains** current highest priority for quick comparisons

### Override Behaviors (per track)

cpp

```
enum class ECk_AudioTrack_OverrideBehavior : uint8
{
    Interrupt,   // Stop lower priority immediately, start this track
    Crossfade,   // Fade out lower while fading in this track
    Queue        // Wait for current track to finish naturally
};
```

Timer Integration
-----------------

Using existing `CkTimer` feature for all time-based functionality:

### Combat Music Fadeout Example

1.  Combat starts → `Request_StartTrack("Combat", HighPriority)`
2.  Combat music plays, AudioDirector creates Timer("CombatFadeout", 15s, Paused)
3.  Combat ends → Timer starts counting down
4.  Timer completion → `Request_StopTrack("Combat")`

### Cooldown Prevention

1.  Track stops → AudioDirector creates Timer("TrackCooldown", 5s)
2.  Same track triggered again → Check if cooldown timer still active
3.  If active → Ignore request, if expired → Allow new playback

Configuration Philosophy
------------------------

**No hardcoded data tables** - flexible configuration through:

1.  **Direct API calls** - Simple runtime track management

cpp

```
auto Director = UCk_Utils_AudioDirector_UE::Add(Player, DirectorParams);
UCk_Utils_AudioDirector_UE::Request_AddTrack(Director, CombatTrackParams);
UCk_Utils_AudioDirector_UE::Request_AddTrack(Director, AmbientTrackParams);
```

1.  **DataAssets** - For complex configurations (optional)

cpp

```
UCLASS(BlueprintType)
class UCk_AudioDirectorConfig : public UDataAsset
{
    UPROPERTY(EditAnywhere)
    TArray<FCk_Fragment_AudioTrack_ParamsData> Tracks;

    UPROPERTY(EditAnywhere)
    FCk_Fragment_AudioDirector_ParamsData DirectorSettings;
};
```

1.  **Game integration layer** - Can add data table support without framework dependency

Framework Consistency
---------------------

Following established Ck patterns:

### Handle Types

cpp

```
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct FCk_Handle_AudioDirector : public FCk_Handle_TypeSafe { /* ... */ };

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct FCk_Handle_AudioTrack : public FCk_Handle_TypeSafe { /* ... */ };
```

### Parent-Child Relationships

-   AudioDirector owns AudioTrack entities (like GridSystem owns GridCell)
-   AudioDirector owns Timer entities for cooldowns (standard Timer pattern)
-   Proper cleanup via `UCk_Utils_EntityLifetime_UE` when Director destroyed

### Request-Response Pattern

-   All mutations through Request_ functions
-   Processors handle requests asynchronously
-   State changes via fragments, never direct modification

### Signals & Events

cpp

```
// Director-level events
CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(OnAudioDirector_TrackStarted, ...);
CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(OnAudioDirector_TrackStopped, ...);

// Track-level events
CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(OnAudioTrack_PlaybackFinished, ...);
CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(OnAudioTrack_FadeCompleted, ...);
```

This design provides a robust, scalable foundation that integrates seamlessly with the existing Ck framework while supporting the music management requirements outlined in the design spec.