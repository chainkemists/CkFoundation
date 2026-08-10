# CONTINUATION — CkIntent Phase 10: playground arena, slice 3b verdict + slices 4-5

> **Freshness:** written 2026-08-10 at session end, mid-slice-3b. **Death condition:**
> superseded once Phase 10 closes in PROGRESS.md — trust PROGRESS.md and PHASE_10.md (same
> directory) over this file wherever they disagree; they are the canonical trackers and were
> current at write time.

**One-liner:** you are the orchestrator of the CkIntent campaign, Phase 10 (`PHASE_10.md`):
the playground gym was rebuilt as a Diablo-style combat arena in maintainer-steered slices.
Slices 1-3 are signed-off/installed and PUSHED; slice 3b (chain-feel rework + hold-threshold
retune) is INSTALLED but UNCOMMITTED with its gate possibly orphaned (see First Tasks);
slice 4 (combos) is next and has one design fork to research + present.

## Read first, in order

1. `PROGRESS.md` (this directory) — current-state block at top. The founding rule: trust it
   over memory, over this file, over the compaction summary.
2. `PHASE_10.md` — the maintainer's spec (near-verbatim), rulings [P10-D1..D7], flags, slice
   log with per-slice detail and gate evidence.
3. The four gym sources: `D:\Repos\CkPlugins\Plugins\CkTests\Script\CkInput\CkPlaygroundGym_{Pawn,Shared,Enemy,Kit_Moves_Assets}.as`
   (+ `_PlayerController.as`, `_GameMode.as` — both small).

## Standing rules (maintainer's, verbatim-derived — do not relax)

- Step-by-step slices; the maintainer PIEs and steers between every slice. Delegate big
  slices to **Opus** drafters (drafts to scratchpad, never repo); orchestrator reviews
  (mirror-diff, verify every wrapper signature at file:line), installs, gates. Feel/judgment
  work (like 3b) is orchestrator-inline.
- Gate of record: `& D:\Repos\CkPlugins\CkAuto\UnrealToolbox.exe --test --test-pattern
  "Ck_AutoTest_In" --discover-fresh --parallel 1 --output=D:/Repos/CkPlugins/Saved/Logs/<name>.log
  --project=D:/Repos/CkPlugins/CkPlugins.uproject` — PASS = **123/123**, verdict from the
  trailing `=== Test summary ===` block, never the exit code alone. Exit 76 = AS compile
  failed, results invalid. Editor-lock preflight before every run:
  `[IO.File]::Open('D:/Repos/CkPlugins/Saved/Logs/CkPlugins.log','Open','Write','None')` —
  run only when FREE (maintainer feedback: wait, don't coexist).
- No `.as` edits while a toolbox run is in flight. Commit only when the maintainer invokes
  /commit or /commit-push; push ONLY via /commit-push (both were used this session — they are
  maintainer-invoked, never self-initiated).
- Foreign dirt NEVER staged: CkFoundation `Content/CkUsf/GeneratedLooks/*.uasset` (~80 files,
  test-run byproduct), `docs/reviews/2026-05-08-CkNavigation-CTO-review.md`,
  `docs/superpowers/`, `docs/campaigns/request-completion-delegates/CONTINUATION_PROMPT_*`;
  superproject: everything (gitlinks span other sessions). During a CkFoundation rebase the
  GeneratedLooks dirt blocks it — `git rebase --autostash` carries it through (verified twice
  this session; check remote commits don't touch those paths first).

## Repo state (2026-08-10 session end)

| Repo | Branch | Pushed tip | Uncommitted |
|---|---|---|---|
| CkTests | dev | `9fecb8f0` (playground + dummy/block all public) | **Slice 3b**: `CkPlaygroundGym_Pawn.as` + `CkPlaygroundGym_Kit_Moves_Assets.as` (in-flight work, commit after maintainer sign-off) |
| CkFoundation | dev | `e317e35b4` (CkInput + CkIntent module + campaign docs public) | `PROGRESS.md` + `PHASE_10.md` edits (rulings D6/D7, slice-3b log) + this file; plus the foreign dirt above |
| CkGameplayDebugger | `feature/debugger-qol-campaign` | NOT pushed (16 local commits, other session's) | do not touch, do not switch branches |
| CkPlugins (super) | dev | untouched | pointer bumps deliberately not done — safe to do for CkTests/CkFoundation now that tips are public, but leave to maintainer/ship |

Two rebase conflicts were resolved during the push (maintainer-ruled): `Source/CkInput/Claude.md`
(took our rewrite + grafted the UI session's `Get_BrushForKey` miss-caveat) and
`Source/CLAUDE.md` (union of both sides' decision-tree rows). Verified marker-free post-push.

## FIRST TASKS for the new session — the tree is BUILD-BROKEN, cross-repo

1. **Unblock the build (blocks everything).** The remote commit `3d1b68b26` (CkFoundation)
   extracted `CkWorldSpaceWidget` out of CkUI into its own module. The single-shot
   `--build --test` run at session end FAILED (`Test-Phase10Slice3b-BuildTest.log`,
   `=== Build FAILED ===`): **CkGameplayDebugger**'s `CkEcsDebugger` module still includes
   `CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment.h` (2 hits: `Inspectors/CkInspector_UI.cpp:5`,
   `FeatureFlags/CkEcsDebugger_FeatureFlags.cpp:53`) — fatal C1083, no such file. That repo
   sits on `feature/debugger-qol-campaign` (another session's 16 local commits — do NOT
   switch branches or fix uninvited). Resolution order:
   a. `git -C Plugins/CkGameplayDebugger fetch origin` and check whether the teammate who
      extracted the module already fixed the consumers on that repo's `dev` or the feature
      branch's upstream — if yes, the maintainer decides how to take it (their branch).
   b. Otherwise ASK THE MAINTAINER: the mechanical fix is retargeting the two includes to the
      new module path + adding `CkWorldSpaceWidget` to `CkEcsDebugger.Build.cs` — but it
      lands on the debugger campaign's branch, so it is their call, not yours.
   Editor boot without a rebuild also fails ("module 'CkWorldSpaceWidget' could not be
   found" — that killed the plain `--test` attempt, `Test-Phase10Slice3b.log`).
2. **Then settle the slice-3b gate** — single-shot `--build --test` (editor closed), PASS =
   123/123 from the trailing summary block. NOTE: the 3b AS edits have NEVER compiled (both
   gate attempts died before AS compile), so an exit-76 here is plausibly OUR typo — read
   the AS error block before assuming anything.
3. **Get the maintainer's PIE verdict on slice 3b** (they said "I will test this"). What
   changed and what to expect is in the branch table below.
4. On sign-off: offer /commit for the 3b files, then open **slice 4** (see below).

## Slice 3b — what was just built (rulings [P10-D6] + [P10-D7])

Maintainer's complaints, verbatim: *"Why is it that I cannot easily chain the Light and Heavy
attacks 1,2,3? … I would have a wind-up, the attack, and then a wind-down. It's during the
attack and wind-down that I can chain"* and *"usually the hold threshold is around 70-80ms so
that the non-hold attack doesn't feel sluggish."*

The three defects fixed: (1) chain input was graded on tap COMPLETION (release) not press;
(2) no wind-down — the raw attack duration was the whole chain window; (3) a late tap reset
to step 1. Implementation (all in the pawn):

- 20% wind-up per chain step (`k_Phase_WindUpFraction`); swing spawns where wind-up ends
  (`DoTrySpawnPendingSwing`), hit-test at the strike; mid-wind-up charge cancels the strike
  free (pending swing never spawns).
- Buffer answers the PRESS row off the record (`DoAdvance_ChainPressIntent`, gated on
  `_WindUpEndFrame`); a charge verdict landing later clears it (order: press-read BEFORE
  completions in `DoAdvance_CombatKit`). Completion path (`DoOnTapLanded`) is the back-stop,
  same wind-up gate via the attempt's `PressFrame`.
- 10-frame grace window after unbuffered expiry (`_GraceUntilFrame/_GraceFamily/_GraceNextStep`)
  — a same-family tap in Idle inside it CONTINUES the chain.
- `hold=45` → `hold=5` in the move table (~83ms verdict point); `k_ChargeHoldFrames=5` there;
  the pawn's NEW `k_ChargeFullFrames=45` is display-only (sphere saturation + counter
  denominator). Verdict threshold ≠ charge ripeness — two numbers, deliberately split.

## Branch table — maintainer's 3b PIE symptoms

| If they report | Probably | Knob / file |
|---|---|---|
| still can't chain by deliberate clicking | grace/window too tight, or press-intent not firing (check press frames on the input readout line) | `k_ChainGraceFrames` (10), `k_Phase_WindUpFraction` (0.20), pawn |
| accidental charges from normal clicks | 5f verdict too tight for their click style | `hold=5` in `Kit_Moves_Assets.as` + `k_ChargeHoldFrames` (try 7-8 ≈ 120ms) |
| attack feels delayed after press | that's the wind-up made visible (by design) — or fraction too big | `k_Phase_WindUpFraction` |
| chain fires a step they didn't want after charge | press-buffer vs charge-clear ordering race (charge should win — verify `DoAdvance_ChainPressIntent` runs BEFORE `DoTryRecordAttempts`) | pawn tick order |
| gate exit 76 | AS compile error in the 3b edits (untested at session end if the gate died) | read the log's AngelScript error block |
| editor-boot fail `CkWorldSpaceWidget` | binaries stale — run `--build --test` | n/a |
| taps do nothing at all | regression of the proven path — read input line: `L p-1` while `row` climbs = button not arriving (was PROVEN working pre-3b) | `Shared.as` composition |

## Critical files

- `CkPlaygroundGym_Pawn.as` (~1750 lines) — everything player-side: cursor aim, movement
  (MaxSpeed 600), camera (fixed `UCk_CameraLayer_TopDown`, output sink = pawn's
  `UCk_CameraComponent` — REQUIRED param since CkFoundation `34d89aa91`), the combat machine
  (11 states + phases), block plate, floor readouts, enemy hit-test.
- `CkPlaygroundGym_Kit_Moves_Assets.as` — 4 moves: `"L"`/`"L hold=5"` (900/600),
  `"H"`/`"H hold=5"` (890/590). `Declare_Move` idiom from `CkIntent_Moves_Assets.as`.
- `CkPlaygroundGym_Shared.as` — key ledger (L=LMB, H=RMB, B=Q minted no-move; WASD/Mouse2D/Tab
  claimed-unminted), source composition (bias default + button map + sampler ring 240, NO SOCD
  quad), record readers, `TryGet_Matcher` via layer priority 250.
- `CkPlaygroundGym_Enemy.as` — `UCk_GenericEntityScript_UE`, spawned by pawn via
  `FCk_Gym_TransformSpawnParams` name-match (`InitialTransform` ExposeOnSpawn — a NEW class
  cannot reference its own generated params struct, first-compile chicken-and-egg), PMG body,
  3s telegraphed projectile, `Request_TakeHit`, HITS floor counter.
- `CkPlaygroundGym_PlayerController.as` — thin: cursor visible, source-composition tick,
  `Ck_GymPlayground_Status` / `_Diagnostics` execs, legend Print.
- Generated wrappers consumed (verify signatures here before ANY new call):
  `CkFoundation/Script/Generated/utils_{intent_matcher,intent_sampler,intent_grammar,input_button_map,input_layer,pmg_basic_shapes,pmg_flat_shapes,pmg_text_shapes,pmg_debug_shape,transform,debug_draw,entity_script,entity_lifetime}.as`
  (`utils_timer::Create_Tick` is hand-authored: `CkUtils_Timer.as:5`). `rg --no-ignore` —
  Grep tool is blind under `Script/`.

## Ruled out / do not re-litigate

- Release-fire for tap-with-hold-sibling is CORRECT ("it _has_ to fire on release") — the
  fix was press-BUFFERING, not changing the verdict law. [P10-D5] records the library
  backlog (matcher candidate-lifecycle signals) — fenced out of this phase.
- Floor text recipe is SOLVED after two failed attempts (vertical+crushed, then mirrored):
  YZ plate, `FRotator(-90, CameraYaw, 0)`, camera-backward offset, floor+3cm. Wireframe
  glyphs read from exactly one side; the tilt-derivation cannot flip. Do not re-derive.
- Mouse buttons through the Slate input source are PROVEN (debugger timeline showed
  Kit_Light_* spans). The input readout line is the discriminator if regressed.
- `ck::ToEntity(this)`, `Math::Cos`, `.DotProduct` — all attested in corpus, fine in AS.
- Stations/zones/arcade-freeze: deleted per [P10-D1]; archive at scratchpad
  `phase10_station_archive/` is GONE with the session scratchpad — recover from git
  (`03b977ca` has the old text gyms; the station files died uncommitted, but every pattern
  they carried now lives in the arena files or the deleted-code history of PHASE_9/10 docs).
- [P10-F1] deferred: CkIntentDebugger timeline scrub UX + axis "s"-suffix (maintainer OK
  deferring; CkGameplayDebugger repo, debugger-qol branch).

## Slice 4 (next) — combos, with ONE fork to research then present

Spec item 5: **LMB+RMB is a combo, RMB+LMB is a DIFFERENT combo, W(forward)+LMB is a combo.**

- LMB+RMB vs RMB+LMB as distinct moves: the grammar has two-button chord terminals (the only
  other deferral cause besides hold=). RESEARCH FIRST in `Source/CkIntent/Claude.md`: chord
  window semantics (bake default 3 frames), and whether ORDER (L then H vs H then L) is
  expressible in notation or must be game-side sequencing. Do not assume.
- W+LMB: fork to present to the maintainer — (a) mint W as a button and use a grammar chord
  `W+L` (exercises the chord deferral path nothing demos, but W is held for LOCOMOTION —
  check whether a long-held W can even satisfy chord simultaneity; if chords require
  near-simultaneous presses, walking+LMB will never chord and (a) is dead on arrival), vs
  (b) game-side read: W held (or pawn moving camera-forward) at completion time modifies the
  attack. Research the chord semantics BEFORE presenting; recommendation will likely be (b)
  for W specifically and real chords for LMB+RMB / RMB+LMB.
- Chord deferral cost warning for the maintainer: adding a chord on L and H terminals makes
  EVERY bare L/H tap wait the chord window (~3f) on top of the hold verdict — anti-pattern-22
  territory again; small but state it up front.
- Slice 5 after: gamepad parity (if wanted), Shared/ledger prune (dead `Format_Bool` callers
  etc.), registry check, PHASE_10 EDITOR-VERIFY drive script, phase close.

## Gotchas (hard-won this session)

- PMG: duration 0 = first-tick destroy, >0 timed, <0 persist; NO size mutation (transform-
  drive); NO parenting (re-anchor per tick); `Request_SetColor/SetText` on shapes; text via
  `Create_Text` 12-param.
- AS: no NOT macro; never name a local `Cast`; delegates match `const X &in` exactly;
  f-strings can't format handles; ASCII only in authored strings; `ck::IsValid(handle)` not
  `.IsValid()`; enum shifts need `int32(Enum)`; `FVector.Rotation()` IS bound.
- Timer delegate: `(FCk_Handle_Timer, FCk_Chrono, FCk_Time)`; `InDeltaT.Get_Seconds()`.
- The sampler cadence is 60Hz; `k_SamplerHz` converts phase seconds → record frames.
- `Script/CLAUDE.md:139,:509` teach nonexistent `utils_entity_script::TryGet_EntityScript`
  (doc-drift flag, still unfixed — don't copy examples from there).
- `CkCameraGym_Pawn.as:89` still carries the pre-`34d89aa91` camera Add (latent ensure at
  PIE) — flagged to maintainer, NOT fixed, out of scope.

## Suggested first message (for the maintainer to paste)

> I'm continuing the CkIntent Phase 10 playground arena. Read
> `D:\Repos\CkPlugins\Plugins\CkFoundation\docs\campaigns\2026-08-07-CkIntent\CONTINUATION_PROMPT_ArenaSlices4Plus.md`
> fully, then PROGRESS.md and PHASE_10.md in the same directory, before doing anything.
> First: settle the slice-3b gate (the build+test may have died with the old session), then
> take my PIE verdict on the chain feel. Same rules: Opus dispatches with orchestrator
> review, scoped gates, ask before commits, push only on /commit-push.
