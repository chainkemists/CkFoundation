# Tween Self-Destruct Feature

## Overview
Add option for tweens to automatically destroy themselves upon completion. This is useful for fire-and-forget tweens that don't need to persist after they finish.

## Current State Analysis
- Tweens are ECS entities owned by another entity (via EntityLifetime)
- Completion logic lives in `FProcessor_Tween_Update::DoCheckLoopCompletion()`
- Completion is determined by: `LoopCount == -1` (infinite) OR `CurrentLoop < LoopCount` (finite)
- When a tween completes without looping, it:
  - Removes `FTag_Tween_Playing`
  - Adds `FTag_Tween_Completed`
  - Fires `OnTweenComplete` signal
  - Chains to next tween if configured

## Implementation Plan

### Phase 1: Add Parameter to Fragment
**File:** `CkTween_Fragment_Data.h`
- Add `bool _SelfDestructOnComplete` to `FCk_Fragment_Tween_ParamsData`
- Default to `false` (backward compatible)
- Use `CK_PROPERTY` to generate getter/setter
- Place after `_YoyoDelay` in the struct

### Phase 2: Add Enum for Stop Behavior
**File:** `CkTween_Fragment_Data.h`
- Add new enum `ECk_TweenStopBehavior`:
  - `DoNothing` (default)
  - `SelfDestruct`
- Update `FCk_Request_Tween_Stop` to include this enum parameter
- Default to `DoNothing` for backward compatibility

### Phase 3: Update Creation Functions
**File:** `CkTween_Utils.h` and `CkTween_Utils.cpp`
- Add `bool InSelfDestructOnComplete = false` parameter to all tween creation functions:
  - `Create_TweenFloat`, `Create_TweenVector`, `Create_TweenRotator`, `Create_TweenLinearColor`
  - Transform shortcuts: `Create_TweenEntityLocation`, `Create_TweenEntityRotation`, etc.
  - Follow target variants: `Create_TweenEntityLocation_FollowTarget`, etc.
- Update corresponding implementations
- Pass parameter through to `DoCreateTween` helper

### Phase 4: Update Processor Logic - Natural Completion
**File:** `CkTween_Processor.cpp`
- Modify `FProcessor_Tween_Update::DoCheckLoopCompletion()`
- After firing `OnTweenComplete` signal and before chaining:
  - Check if `InParams.Get_SelfDestructOnComplete()` is true
  - If true, call `UCk_Utils_EntityLifetime_UE::Request_Destroy(InHandle)` 
  - Important: Destroy AFTER signal fires so listeners get notification
  - Important: Destroy BEFORE chaining so next tween isn't affected

### Phase 5: Update Processor Logic - Manual Stop
**File:** `CkTween_Processor.cpp`
- Modify `FProcessor_Tween_HandleRequests::DoHandleRequest()` for `FCk_Request_Tween_Stop`
- After firing `OnTweenComplete` signal:
  - Check the stop behavior enum
  - If `SelfDestruct`, call `UCk_Utils_EntityLifetime_UE::Request_Destroy(InHandle)`

### Phase 6: Update Stop API
**File:** `CkTween_Utils.h` and `CkTween_Utils.cpp`
- Update `Stop()` function to take `ECk_TweenStopBehavior` parameter
- Default to `DoNothing`
- Update all transform tween variants

## Logic Flow for Completion

### Natural Completion
```
Current time >= Duration
  ↓
Should loop? (infinite OR more loops remaining)
  ↓ YES → Reset and continue
  ↓ NO
Mark as Completed
Fire OnTweenComplete signal
  ↓
Check SelfDestructOnComplete?
  ↓ YES → Destroy entity
  ↓ NO → Continue
Start next chained tween (if any)
```

### Manual Stop
```
Request_Stop received
  ↓
Mark as Cancelled/Completed
Fire OnTweenComplete signal
  ↓
Check StopBehavior enum?
  ↓ SelfDestruct → Destroy entity
  ↓ DoNothing → Continue
```

## Edge Cases Handled
1. **Infinite loops**: `LoopCount == -1` → Never reaches completion → Never self-destructs ✓
2. **Zero loops**: `LoopCount == 0` → Completes after one play → Can self-destruct ✓
3. **Finite loops**: `LoopCount > 0` → Completes after N loops → Can self-destruct ✓
4. **Chained tweens**: First tween self-destructs, next tween still starts ✓
5. **Manual stop with DoNothing**: Stops but doesn't self-destruct (default behavior) ✓
6. **Manual stop with SelfDestruct**: Stops and self-destructs ✓

## Design Decisions
- **Creation only**: Self-destruct flag can only be set at creation time, not modified later
- **Default to false**: Backward compatible, opt-in feature
- **Stop behavior enum**: Explicit control over manual stop behavior
- **Signal fires first**: Listeners receive notification before entity is destroyed
- **Chain before destroy**: Next tween starts before current tween is destroyed
