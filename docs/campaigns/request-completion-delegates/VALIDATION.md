# VALIDATION.md — definition of done (campaign acceptance protocol)

> Run at Gate 09. Every check names its command and expected observation. All runs via the
> `/build-test` skill (UnrealToolbox) — never Build.bat / UnrealEditor-Cmd directly.

## 1. Structural sweeps (greps; run with `rg --no-ignore` from `Plugins/CkFoundation`)

| Check | Command | Expected |
|---|---|---|
| No dead request-handle destruction | `rg -n 'Get_IsRequestHandleValid' Source --glob '*_Processor.cpp'` | Every hit is inside a guard/cancel path, not the old silent-destroy epilogue (0 of the 55 recon-era dead sites remain) |
| No delegate-in-struct completion | `rg -n '_OnComplete' Source/CkEqs Source/CkDialog` | 0 hits in request structs (G0-D4) |
| Every Request_* UFUNCTION has the delegate param | per-module spot greps `rg -A6 'Request_' Source/<M>/Public --glob '*_Utils.h' \| rg -c 'InDelegate'` | count matches the module's Request_* count (census-reconciled) |
| Bind always IsBound-gated | `rg -B2 'CK_SIGNAL_BIND_REQUEST_FULFILLED\(ck::UUtils_Signal_RequestCompleted' Source` | every site preceded by an `IsBound()` gate (G0-D8) |
| Cancel helper adoption | `rg -ln 'FireCancelledForPending' Source` | every module with a `_Requests` fragment appears (or a recorded bespoke-cancel note, e.g. CkInventory's DispatchCancel) |

## 2. Build + test gate (the real gate)

1. `--build` all configs the toolbox drives by default → clean.
2. `--test --discover-fresh --no-nullrhi` (no `--config` on test-only) → full suite.
   - Expected: Total = Gate 00 baseline + (all new completion tests added across gates —
     running count kept in PROGRESS.md per gate).
   - Delta-zero: the failing set is EXACTLY the baseline's pre-existing red set (crowd
     workstream reds), no additions, no silent removals.
3. Startup-log inspection: zero new ensures, zero AS compile errors on editor boot
   (non-negotiable #3's startup-log check).

## 3. Behavioral acceptance (per-feature test classes, added incrementally per gate)

Every rollout gate ships, per module batch, at least:
- one `SucceedsOnDrain` test (delegate fires once, `Succeeded`, after observable state change),
- one `FailedNotEnqueued` test (sync rejection fires synchronously),
- one `CancelledOnTeardown` test (destroy owner with queued request → `Failed_Cancelled`),
for a representative feature of the batch. Gate 09 re-runs all of them in the full suite (§2).

## 4. Three environments (non-negotiable #4)

- **C++**: covered by compilation + C++ call sites in tests.
- **AngelScript**: every batch's tests are AS autotests (calls + delegate binds from AS);
  Gate 00 step 6 evidence that the delegate param is omittable (or the recorded ruling if not).
- **Blueprint**: `[EDITOR-VERIFY]` — open any BP, place a `Request_*` node from a Gate-07 module:
  the delegate pin appears as an optional event pin (AutoCreateRefTerm); wire it to a custom
  event, PIE, observe the event fire with `Succeeded`. Exact clicks: add node
  `[Ck][Timer] Request Pause` → drag from `InDelegate` → "Create Event" → bind custom event →
  print `InResult`. Expected: prints `Succeeded` once.

## 5. Docs & doctrine

- Root `CLAUDE.md` §Requests documents the contract; `Source/CkEcs/CLAUDE.md` documents the
  shared signal + cancel helper; per-module `Claude.md`s updated where behavior notes exist.
- FEATURE_CENSUS.md tombstoned or updated to final state; PROMPT.md death condition evaluated.
- `ck-macros-and-codegen` skill §request recipe updated to include the completion contract.

## 6. Ship

Route through `ck-change-control` (framework-invariant class → maintainer review) and
`ck-ship-dev` (or `ck-ship-pr` per maintainer preference). Commit/push only on explicit
maintainer go-ahead — the orchestrator stops for a yes.
