# Phase 0 — the red executable spec

## Entry criteria
- `PROMPT.md` status line reads `APPROVED`.
- CkFoundation contains the `LastPushedTransform` fix (`git log --oneline --grep="pump-drained one-shot"`
  returns a commit). BusterBlock contains
  `Plugins/BusterBlockTests/Script/Tests/UnrealComponent/BB_AutoTest_UnrealComponent_OneShotPushReachesComponent.as`.
  If either is missing → STOP, record in PROGRESS.md blockers (the PRs may not be merged yet).
- No `BusterBlockEditor.exe` running (`Get-CimInstance Win32_Process -Filter "Name='BusterBlockEditor.exe'"` empty).

## Steps

1. Copy `BB_AutoTest_UnrealComponent_OneShotPushReachesComponent.as` to
   `BB_AutoTest_UnrealComponent_OneShotPush_SettlesSameFrame.as` in the same folder; rename the
   class accordingly. ONE class per file; the wrapper is auto-generated.
2. Reduce it to: Arrange (same entity + `UStaticMeshComponent` composition, pose A) → settle 3
   frames → seed check (component == A) → `Step_IssueB` (one one-shot
   `utils_transform::Request_SetLocation` to B = A + (500,0,0)) → **immediately next step**
   `Step_CheckB_SameFrame`: read the live component world location; if within 1uu of B →
   `FinishSuccess()`; else `FinishFailure` with
   `f"same-frame settle missing: component={...} expected={...}"`. NO settle steps between
   IssueB and the check — the zero-gap is the point (each `Add_Step` action consumes exactly
   one tick; the check runs on the tick after the write's pump-drain, BEFORE that tick's
   PostTransform slot).
3. Run: `${env:UE-CmdLineArgs} = '-DisablePlugins=RiderLink'` then
   `./CkAuto/UnrealToolbox.exe --test --discover-fresh --test-pattern SettlesSameFrame`
   (from the BusterBlock root; `--discover-fresh` is required for a brand-new test).

## Decision gate

- **Expected: FAIL** with the `same-frame settle missing` message and component still at A —
  paste the exact failure line into PROGRESS.md as the spec baseline. → continue to Phase 1.
- If exit 76: AS authoring error — read `Saved/Logs/BusterBlock.log` tail, fix your own syntax
  only, rerun. (Adjacent f-string literals do not concatenate in AS — known first-authoring trap.)
- **If it PASSES**: the write step main-drained instead of pump-draining (harness timing shifted)
  → STOP, record in PROGRESS.md blockers with the log's drain context. Do NOT invent a new
  write mechanism — the coordinator must re-pin how to force a pump-drain.
- Anything else → STOP, blockers.

## Exit criteria
- New test exists, discovered (Total: 1), RED with the expected message, failure line recorded.
- The original `OneShotPushReachesComponent` still passes (`--test-pattern OneShotPushReaches`):
  Total 1, Passed 1.
