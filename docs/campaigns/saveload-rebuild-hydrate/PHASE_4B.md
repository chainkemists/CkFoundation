# PHASE 4B — Coverage sweep: Params-mutators, RenderTarget re-author, MontagePlayer rebind, AS smoke matrix

Load `ck-angelscript-interop` + read `Script/CLAUDE.md` before the AS steps.

## Entry criteria
- Phase 4A done; oracle allowlist empty of pending lines; patterns green.

## Steps

### 4B.1 The Params-mutator list (verified census — closed; spec [V4])
For each: add a Save-transport payload (new small `FCk_SaveData_*` USTRUCT + `RegisterLazyTyped` handler with
`Produce`/`Apply`, `Transport = Save` only — these do NOT go on the wire) OR mark declared-out-of-scope in the
oracle annotations file with a one-line reason. Decisions are already made:

| Feature | Mutation site | Decision |
|---|---|---|
| 2dGridCell tags | `Ck2dGridCell_Utils.cpp:151-168` | payload (`FCk_SaveData_2dGridCellTags{TArray<FGameplayTag>}`) |
| Goap/AStar budgets | `CkGoap_Planner_Utils.cpp:722-724` | payload (budget + threshold) |
| Timer re-Add full Params | `CkTimer_Utils.cpp:98` | payload (whole `FCk_Fragment_Timer_ParamsData` + chrono elapsed — labeled timers only; unlabeled = save-transient per ruling 1) |
| Substep Params | `CkSubstep_Utils.cpp:53` | payload (whole ParamsData) |
| WorldSpaceWidget config | `CkWorldSpaceWidget_Processor.cpp:290-348` | payload (the four info structs) |
| CameraLayer post-create config | `CkCamera_Utils.cpp:56-59` | **declared out of scope** — layers are re-established by possession/gameplay (`FTag_CameraLayer_Active` is already save-transient by design); annotate |
| Pmg text | `CkPmg_Utils.cpp:282` | **declared out of scope** — debug/procedural text; annotate |
| MontagePlayer rebind | `CkMontagePlayer_Utils.cpp:91-92` | NOT a payload: post-hydration fixup — call `Request_RebindSkeletalMeshComponent` from the feature's Apply when hydration-scope (the in-code comment at that site already names snapshot-restore as its purpose) |
Annotations file: `docs/campaigns/saveload-rebuild-hydrate/oracle-declared-transient.txt`, consumed by the
OracleParity test alongside the allowlist (same matching, separate list, reason strings mandatory).

### 4B.2 RenderTarget re-author
Its Phase-1 `Produce` already emits the authored-log payload. Verify the Save-transport Apply RE-AUTHORS through
the normal request path on authority (hydration-scope branch, like 4A.1) so the server container refills and
clients repaint — assert via `Ck.Snapshot.Parity.RenderTarget_MPReload` staying green through a v3 load.

### 4B.3 AS smoke matrix (from the 2026-05 review, unbuilt since)
CkTests AS-side (follow `ck-tests-authoring-and-running` for AS test placement): one AS `UCk_EntityScript_UE`
subclass with `UPROPERTY(SaveGame) int`, negation `UPROPERTY() int`, `FString`, `TArray<int32>`; v3 save →
load → assert positive fields restored, negation field default. Script-instance field hydration applies
post-Construct/pre-BeginPlay (spec §3 AS surface — the v3 rebuild spawns the script fresh; its SaveGame fields are
a framework-level payload: add `FCk_SaveData_EntityScriptFields` produced/applied by CkEcs itself serializing the
script instance's SaveGame-tagged fields — design fixed: framework handler, not per-script).
Test names: `"Ck.Snapshot.AS.SaveGameFields_RoundTrip"`, `"Ck.Snapshot.AS.NonSaveGameField_Drops"`.

### 4B.4 Gate + commit
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p4b.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p4b-net.log
```
Expected: delta-zero + new tests green + OracleParity green with only the declared-transient list. Any oracle diff
not in a list = a coverage bug in THIS phase's features. Commits per feature cluster; AS test commit in CkTests.

## Exit criteria
- OracleParity: zero unexplained, zero pending; declared-transient file populated with reasons.
- The two AS tests green by name. PROGRESS updated.

## Fences
- Do NOT put Save-only payloads on the wire (`Transport = Save`).
- Do NOT rename or restructure the 8 features' Params — payloads OVERLAY, construction stays untouched.
- AS work: never write `.as` during a test run; grep the fresh log for `Angelscript: Error` before claiming green.
