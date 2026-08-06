# PHASE 2 — Built-in packs (Audio, Video)

**Goal:** two opt-in packs register real settings through the same public API any game would use.
Audio introduces the category-volume driver CkFoundation lacks; Video bridges `UGameUserSettings`
via `External` policy.

## Entry criteria

1. Phase 0+1 exit criteria hold (re-verify test names green, trees clean).
2. Skills: `ck-change-control`, `ck-tests-authoring-and-running`.
3. Read `Source/CkAudio/Public/CkAudio/CkAudio_Settings.h` (confirm: no user-facing volume surface
   exists — you are adding the first one, in CkGameSettings, NOT in CkAudio).

## Steps

### 2.1 Project settings additions (`Settings/CkGameSettings_Settings.h`)
- `_EnableAudioPack` (bool, default false), `_EnableVideoPack` (bool, default false).
- Audio pack config: `_AudioMix` (TSoftObjectPtr<USoundMix>),
  `_AudioCategories` (TArray of `FCk_GameSettings_AudioCategory { _CategoryTag (FGameplayTag),
  _SettingKey (FName), _SoundClass (TSoftObjectPtr<USoundClass>), _DefaultVolume (float 0..1) }`).
  Ship four suggested defaults DOCUMENTED in the Claude.md (master/music/sfx/voice) but do NOT
  hardcode them — an empty array + disabled pack is the shipped default (zero-asset doctrine:
  the plugin ships no SoundMix/SoundClass assets; the game supplies its own).

### 2.2 `Packs/CkGameSettings_AudioPack.{h,cpp}`
Free-standing registrar invoked from subsystem `Initialize` when enabled:
- For each configured category: register a `Float` definition (`_Scope = Machine`,
  `_PersistencePolicy = Provider`, `_ApplyBindingType = Handler`, range 0..1, key from config) and
  register its apply handler: async-load the SoundClass/Mix (`CkResourceLoader` NOT required —
  a simple `TSoftObjectPtr::LoadSynchronous` at first apply is acceptable here since this runs at
  boot/menu cadence; note it in Claude.md), then
  `UGameplayStatics::SetSoundMixClassOverride(World, Mix, SoundClass, Volume, /*Pitch*/1.0f,
  /*FadeIn*/0.5f, /*bApplyToChildren*/true)` + `PushSoundMixModifier` once.
- World = `GetGameInstance()->GetWorld()`; guard with `ck::IsValid`; a null world at apply time
  re-queues on the next world (bind `FWorldDelegates::OnPostWorldInitialization`, tracked +
  unbound in `Deinitialize`).

### 2.3 `Packs/CkGameSettings_VideoPack.{h,cpp}`
All definitions `_PersistencePolicy = External`, `_ApplyBindingType = Handler`, external accessors
registered against **`GEngine->GetGameUserSettings()` exclusively** (never a concrete subclass —
PROMPT rejected-approaches table). Settings (keys `video.*`):
- `video.window_mode` (Int32 ↔ `EWindowMode`), `video.resolution` (String `"1920x1080"`),
  `video.vsync` (Bool), `video.fps_cap` (Float; 0 = uncapped), `video.quality_preset`
  (Int32, -1 = custom), and the seven `video.sg.*` scalability Int32s.
- Setter handlers write via the GameUserSettings API then `ApplySettings(false)` (resolution/mode:
  `ApplyResolutionSettings`) + `SaveSettings()`.
- `Request_RunHardwareBenchmark()` on the subsystem (UFUNCTION): `RunHardwareBenchmark()` +
  `ApplySettings` + `SaveSettings`, fires change delegates for all `video.*` keys.
- **Resolution confirm-countdown:** subsystem-level primitive only (the dialog is Phase 3 UI):
  `Request_SetResolutionWithConfirmWindow(NewRes, WindowSeconds)` applies, stashes prior, starts
  an `FTSTicker` countdown; `Request_ConfirmResolution()` cancels the revert;
  expiry reverts + fires change delegates. No UI in this phase.

### 2.4 Tests
- In-module spec: `Ck.CkGameSettings.Packs.VideoValueMapping` (String "1920x1080" ↔ FIntPoint,
  window-mode enum mapping — pure functions, test them as free functions).
- CkTests AS AutoTests: `...GameSettings_AudioPack_HandlerReceivesVolume` — **assert at the seam**:
  register the pack with a test-injected handler-spy (or read back via `Get_SettingValue_Float`
  and assert `SetSoundMixClassOverride` route was invoked — if the harness has no audio device,
  seam assertion is the gate; check whether the toolbox boots `-nosound`/null audio FIRST and
  record in PROGRESS). `...GameSettings_VideoPack_ExternalNeverStored` — set `video.vsync`, flush,
  assert the key does NOT appear in `CkGameSettings.ini` (read the file), and
  `Get_SettingValue_Bool("video.vsync")` reads through GameUserSettings.
  `...GameSettings_ResolutionConfirmWindow_RevertsOnExpiry` (short window).

### 2.5 Gate
Same commands + branches as Phase 1.6. Additional `[EDITOR-VERIFY]` items land in VALIDATION.md —
do not attempt them headless.

## Fences

- The packs use ONLY public subsystem API (they are reference consumers; if a pack needs a
  private hook, the public API is wrong — STOP and record the gap in PROGRESS blockers).
- No `sg.*` CVar writes directly — always through `UGameUserSettings::SetXQuality`.
- No shipped uassets (no SoundMix, no SoundClass — game-supplied via config).
- Do not touch `Source/CkAudio` or `Source/CkInput`.
- Benchmark/resolution paths must be no-ops (with a Display log line) under null-RHI/commandlet so
  the test suite stays green headless.

## Exit criteria

1. Phase-2 test names green; Phases 0-1 names still green; full suite delta-zero vs baseline.
2. `rg -n "GetGameUserSettings" Source/CkGameSettings` hits ONLY via `GEngine->GetGameUserSettings()`.
3. `rg -n "SetSoundMixClassOverride" Source/CkGameSettings/Public/CkGameSettings/Packs` → exactly the audio pack.
4. Committed both repos (no push); PROGRESS updated (incl. the audio-device finding).
