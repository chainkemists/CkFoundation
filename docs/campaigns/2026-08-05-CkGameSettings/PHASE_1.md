# PHASE 1 — Persistence, apply, pending-changes

**Goal:** values survive restart via the default ordered-ini provider; boot replay applies stored
values onto CVar/handler bindings with the two "late" cases handled distinctly; pending-changes
session works; PIE rule implemented and verified.

## Entry criteria

1. Phase 0 exit criteria all hold (re-verify: the seven Registry tests green, tree clean on
   `feature/game-settings`).
2. Skills: `ck-change-control`, `ck-tests-authoring-and-running`; re-read design doc §3.2 —
   it is the spec for this phase.

## Steps

### 1.1 `Storage/CkGameSettings_StorageProvider.{h,cpp}`
```cpp
USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_GameSettings_StoredValue { /* _Key (FName), _Value (FString); CK_DEFINE_CONSTRUCTORS both */ };

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKGAMESETTINGS_API UCk_GameSettings_StorageProvider_UE : public UObject
{
    // BlueprintNativeEvents (AS/BP-implementable — this is the seam's whole point):
    // Get_StoredValues(ECk_GameSettings_Scope InScope, int32 InPlatformUserId) -> TArray<FCk_GameSettings_StoredValue>
    //     (ORDERED — the contract is that iteration order == application order)
    // Request_StoreValue(ECk_GameSettings_Scope, int32 InPlatformUserId, FName InKey, const FString& InValue)
    // Request_RemoveValue(ECk_GameSettings_Scope, int32 InPlatformUserId, FName InKey)
    // Request_Flush()
};
```
`int32 InPlatformUserId` at the reflected boundary carries `FPlatformUserId::GetInternalId()`
(`FPlatformUserId` itself is not reliably BP-exposable — verify once; if it IS BlueprintType on
5.7.4, prefer it and note in PROGRESS).

### 1.2 Default provider — `Storage/CkGameSettings_IniStorageProvider.{h,cpp}`
`UCk_GameSettings_IniStorageProvider_UE : UCk_GameSettings_StorageProvider_UE`. Hand-serialized
file at `FPaths::GeneratedConfigDir()` / platform / `CkGameSettings.ini`:
- Format: `[Machine]` and `[Player.<Id>]` sections; `Key=Value` lines; **file order = application
  order**; unknown lines preserved verbatim on rewrite (forward-compat).
- Load once at provider init into ordered per-section arrays; `Request_StoreValue` updates in
  place (existing key keeps its position; new key appends) and dirty-marks; `Request_Flush`
  rewrites the whole file iff dirty.
- **Fence: zero `GConfig`, zero `FConfigFile` registration with the cache.** (A standalone
  `FConfigFile` object is permitted but hand-writing `Key=Value` lines is simpler and preferred —
  the format above is trivial.)
- Escaping: values are written raw except newlines are rejected at the Set boundary (ensure) —
  a setting value with an embedded newline is invalid input, not an escaping problem.

### 1.3 Subsystem: wire persistence + apply (extends Phase 0 subsystem)
- Provider selection: project settings gains `_StorageProviderClass`
  (TSoftClassPtr<UCk_GameSettings_StorageProvider_UE>, default = the ini provider). Instantiated
  in `Initialize` (`NewObject` outer'd to the subsystem).
- **Boot sequence in `Initialize`:** load stored values → for each stored value: definition
  registered? apply now (see 1.4) : retain as **orphan** (silent, indefinite). Then registration
  (`Request_RegisterSetting*`) consumes any matching orphan as the initial value (applying it)
  before falling back to `_DefaultValue`.
- **Apply (1.4 semantics):** `Provider`-policy value set → store in memory, `Request_StoreValue`
  (dirty), then route by binding: `CVar` → `UCk_Utils_CVar_UE::INTERNAL_Set_<Type>` if
  `IsRegistered`, else enqueue on the **deferred-apply queue**; `Handler` → fire the registered
  typed handler; `None` → nothing. `External`-policy → route to handler ONLY (never stored, never
  read from provider; `Get_SettingValue_*` for External keys reads through a registered typed
  GETTER handler — add `Request_RegisterExternalAccessors_Bool(FName InKey,
  const FCk_Delegate_GameSettings_ExternalGetter_Bool& InGetter,
  const FCk_Delegate_GameSettings_ApplyHandler_Bool& InSetter)` (+ `_Int32/_Float/_String`
  suffixed variants; declare the getter delegates in `CkGameSettings_Common.h`:
  `DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FCk_Delegate_GameSettings_ExternalGetter_Bool);` etc.)).
- **Deferred-apply queue:** retried via a timer/ticker (an `FTSTicker` at ~1s cadence is fine —
  do NOT make the subsystem tickable per-frame); on success, removed; past
  `Get_DeferredApplyTimeoutSeconds()`, dropped LOUDLY — `CK_ENSURE_IF_NOT` naming key + CVar
  (non-negotiable #3 shape), entry removed, value retained in store (not deleted).
- **Handler registration:** `Request_RegisterApplyHandler_Bool(FName, FCk_Delegate_GameSettings_ApplyHandler_Bool)`
  (+ Int32/Float/String; suffixes, no overloads). Registering a handler for a key with a pending
  orphan/deferred value applies it immediately.
- **Flush points:** `FTSTicker` debounce (~2s after last dirty), `Deinitialize`,
  `FCoreDelegates::OnEnginePreExit`, and application-deactivate
  (`FCoreDelegates::ApplicationWillDeactivateDelegate`) — each guarded, each idempotent.
- **Listener hygiene:** every CVar binding handle and every core-delegate handle stored and
  removed in `Deinitialize`. `rg` check in exit criteria.
- **PIE rule (provisional, PROMPT fence 10):** when the owning world context is PIE
  (`GetGameInstance()->GetWorldContext()` — verify `WorldType == EWorldType::PIE` against engine
  source and record the exact mechanism in PROGRESS), values load and `Get_SettingValue_*` is
  correct, but CVar-bound apply (boot replay AND live sets) is skipped; handler-bound apply still
  runs (handlers are per-GameInstance, not process-global). Log one Display line saying so.

### 1.4 Pending-changes session
Subsystem members: `Request_BeginPendingChanges() -> bool` (false + ensure if already active),
`Request_ApplyPendingChanges() -> int32`, `Request_RevertPendingChanges() -> int32`,
`Get_HasPendingChanges() -> bool`, `Get_HasUnappliedChange(FName) -> bool`.
While active: `Request_SetSettingValue_*` applies the value LIVE (preview) and records the prior
value; `Revert` restores priors (and re-applies them); `Apply` commits (clears priors, persists);
`Deinitialize`/world-teardown with an active session auto-reverts. Change delegates fire on every
actual value change including preview and revert.

### 1.5 Tests
In-module specs (pure logic): `Ck.CkGameSettings.Store.RoundTripPreservesOrder`,
`Ck.CkGameSettings.Store.UnknownLinesSurviveRewrite`, `Ck.CkGameSettings.Store.NewlineValueRejected`.
CkTests AS AutoTests (world/GameInstance path — branch `feature/game-settings-tests` in CkTests;
follow `ck-tests-authoring-and-running` for file placement/naming there):
- `...GameSettings_PersistRoundTrip` — register, set, flush, read store file content matches.
- `...GameSettings_OrphanValueAppliedOnRegistration` — store a value for an unregistered key via
  the provider, init, register the definition, assert value == stored (NOT default), assert NO
  ensure fired.
- `...GameSettings_DeferredCVarTimesOutLoudly` — definition bound to a never-registered CVar with
  a short timeout; assert the ensure fires (use the harness's expected-ensure mechanism per the
  skill) and the stored value survives.
- `...GameSettings_PendingChanges_RevertRestoresLiveValues`.
- `...GameSettings_ResetAll_PersistsDefaults`.

### 1.6 Gate
```
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.CkGameSettings" --discover-fresh
```
then the CkTests pattern per its naming, then full suite `--test --no-live` delta-zero.
**Decision gate branches identical to Phase 0.8** (fix in phase / exit-76 = your signature broke AS /
anything unexplained → STOP + PROGRESS blocker).

## Fences

- The two late cases stay distinct (PROMPT glossary). An orphan value must NEVER warn or expire.
- No `GConfig`/`SaveConfig` for VALUES (the project-settings CLASS still uses its inherited
  DeveloperSettings persistence for its own config properties — that is fine and expected).
- Do not delete a stored value on deferred-apply timeout.
- Do not make the subsystem `FTickableGameObject`.
- Flush rewrites the file atomically (write temp + move) — a crash mid-flush must not zero the file.

## Exit criteria

1. All Phase-1 test names green (both repos), plus the seven Phase-0 names still green.
2. Full suite delta-zero vs baseline.
3. `rg -n "GConfig|SaveConfig" Source/CkGameSettings` → hits only in `CkGameSettings_Settings.cpp`
   if any (and zero for value persistence paths); justify each hit in PROGRESS or drive to zero.
4. Manual restart smoke recorded in PROGRESS: run any Phase-1 AutoTest twice; second run's log
   shows values loaded from the ini (grep the Display line).
5. Both repos committed (no push); PROGRESS updated including the verified PIE-detection mechanism.
