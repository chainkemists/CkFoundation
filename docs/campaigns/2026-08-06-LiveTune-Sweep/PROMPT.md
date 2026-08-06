# PROMPT — LiveTune coverage sweep

**Campaign:** bring CkFoundation's features onto LiveTune. The transport shipped 2026-08-05 with 3
features registered out of a 95-feature real surface; this campaign closes that gap.
**Your role:** executor. The transport is DONE and audited — you register features against it. You do
NOT redesign LiveTune.
**Written:** 2026-08-06.

---

## 0. Read before any code (in this order)

1. `Plugins/CkFoundation/docs/campaigns/2026-08-06-LiveTune-Sweep/TRIAGE.md` — the bucket analysis
   and per-feature table you are executing. Regenerate with `python .../triage.py` if the tree moved.
2. `Plugins/CkFoundation/Source/CkEcs/Claude.md` §"LiveTune (editor-only change transport)" — the
   three tiers and what each registration shape costs.
3. `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h` — read the
   header comment and the three `TArgs_*` structs in full. The `TRequired<>` slots are
   compile-enforced; omitting one is a compile error, not a runtime ensure.
4. The three reference registrations:
   - `CkTimer/Public/CkTimer/CkTimer_Fragment.cpp` — `Register` with the default Replace + `.PostApply`
   - `CkSpatialQuery/Public/CkSpatialQuery/Probe/CkProbe_Fragment.cpp` — `Register` with a custom `.Apply`
   - `CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.cpp` — `Register_ViaRebuild` + `.Capture`
5. `Plugins/CkFoundation/docs/specs/2026-08-05-LiveTune-design.md` §5 (tiers) and §10 (risks) — the
   design of record. Do not relitigate it.
6. `Plugins/CkFoundation/CLAUDE.md` + `Source/CLAUDE.md` — code style, ensure discipline,
   tri-environment rule.

## 1. Locked decisions — do not relitigate

- **Opt-in stays.** There is no implicit/automatic tier. Every feature is registered by hand because
  the tier is a judgment about how that feature's params reach runtime state.
- **`Link` ENSURES on an unregistered params type and leaves no stamp**
  (`CkLiveTune_Utils.cpp`). So the tunable set is exactly the registered set, and a missing opt-in is
  loud at setup. Do not weaken this to a warning to make a batch pass.
- **`Multiple*` bulk-add containers are out of scope** (TRIAGE.md). They are construction-time arrays
  of another feature's params; retuning one is ambiguous by construction.
- The two registration shapes, the diff cache, the scrub policy, and the authority gate are settled.
  You consume them. Note the registry was collapsed from three shapes to two on 2026-08-06:
  `Register_ViaReplace`/`Register_ViaRequest` are now one `Register` (they always shared the same
  `Apply` slot), and scrub-safety became an explicit `.ScrubPolicy` instead of being inferred from
  the tier. Anything you read describing three tiers predates that.
- **Report back on `.ScrubPolicy`.** Its `Auto` resolution assumes a custom `.Apply` is too expensive
  to preview per drag-frame. If the sweep keeps finding cheap custom applies that deserve
  `DuringScrub`, say so — the default may be wrong.
- Not every field of a registered feature must retune. A feature whose params mix live-read and
  baked fields either fixes up the baked ones (`.PostReplace`), rejects them loudly (the Probe
  `Request_Reconfigure` shape), or documents the bound. Silence is the one unacceptable outcome.

## 2. Scope — batches, not a big bang

Work **bucket A first** (53 live-read features, TRIAGE.md), in batches of ~8. Bucket A is where the
one-line registrations are; finishing it takes coverage from 3/95 to ~56/95.

Per batch:
1. Verify each feature individually (§3). Registering without verifying is the failure mode this
   campaign exists to avoid.
2. Register the ones that check out; record the ones that don't with the reason.
3. One AutoTest per batch (not per feature) that links each feature, simulates an edit, and asserts
   the retune landed. Follow `Plugins/CkTests/Script/CkEcs/CkAutoTest_LiveTune_TimerViaReplace.as`.
4. Toolbox gate green, then commit that batch.

**STOP after bucket A** and report. Buckets B/C1/C2 are per-feature engineering (a
`Request_Reconfigure` addition or a `ReAdd`/`.Capture` decision each) and should be scheduled against
what people actually want to tune, not swept.

## 3. Per-feature verification — the bar before you register

TRIAGE.md tells you a processor reads the params fragment. That is necessary, not sufficient. For
each feature, confirm:

1. **Which fields are actually consulted after Add.** Read the processors that take the params
   fragment. A field only read in `_Setup` is baked even if the fragment is live.
2. **Whether Add caches derived state.** If Add computes something from params into another fragment
   (a tag, a chrono, a handle), `Replace<Params>` alone will not re-derive it — that is what
   `.PostReplace` is for.
3. **Whether anything external was configured from params at Add** (a Jolt body, a UObject, a
   component). If so the feature is NOT `ViaReplace` regardless of what the fragment view says —
   it belongs in `ViaRequest`/`ViaRebuild`.

**The precedent that defines the bar — Timer.** It is correctly `ViaReplace`: params are live-read,
and `.PostReplace` re-issues `Request_ChangeCountDirection` for the derived tag. But the chrono's
GOAL is baked at Add, so a *Duration* retune silently does nothing, and there is no Timer request to
rebuild it. That bound is written in a comment at the registration site
(`CkTimer_Fragment.cpp:79-82`). **Do the same** — every registration whose params have a
non-retunable field says so at the call site. A slider that moves while the world does not is worse
than an honest refusal, because it burns the designer's trust in the whole feature.

Rows in TRIAGE.md bucket A carried only by an `_EndPlay` or `_Replicate` processor deserve extra
suspicion: those prove retention, not live consultation.

## 4. Testing & verification rules

- **Tests live in CkTests**, not CkFoundation. Follow the `ck-tests-authoring-and-running` skill and
  `Plugins/CkTests/Script/Common/CkAutoTest_CreationSpecification.txt`.
- Build + test via the **Unreal Toolbox only** (`/build-test` skill). Never raw Build.bat or
  UnrealEditor-Cmd. `--build` requires the editor closed. Gate of record: `--test --no-live`.
- **Record the baseline before the first edit** and diff every later claim against it. Run at least
  `--test-pattern LiveTune`; run the full suite before the final report.
- `--discover-fresh` after adding tests, or the toolbox's cached list silently skips them.
- Never edit `.as`/source while a run is in flight.
- The test tuning asset is `UCk_LiveTuneTest_TuningAsset`
  (`Plugins/CkTests/Source/CkTests/Public/CkLiveTune_AutoTest_Utils.h`). Adding a feature to a test
  means adding a member + a `Get_*MemberName()` accessor there. It is a C++ header, so a batch's
  tests need a `--build`.
- A registration that cannot be tested headlessly is labeled `[EDITOR-VERIFY]` with exact click
  steps — do not claim it verified.

## 5. Campaign discipline

- Create `docs/campaigns/2026-08-06-LiveTune-Sweep/PROGRESS.md` at start: baseline numbers, the
  running registered/deferred list with per-feature reasons, gate status per batch. Update before
  ending any session.
- Commit per batch. **Never push.** Stage only files you authored — this machine often has sibling
  sessions with dirty files; enumerate anything dirty you did not touch as "left for owning session".
- Close with the comment audit (root CLAUDE.md): no what-comments, no campaign breadcrumbs in the
  diff. The only comments that survive are constraint comments like Timer's baked-goal note.
- Stuck protocol: two failed attempts on the same feature → defer it with a written reason and move
  on. One awkward feature must not stall a batch.

## 6. Success criteria

1. Bucket A swept: every one of the 53 either registered, or deferred with a recorded reason.
2. Coverage number stated as a fraction of the 95-feature real surface, with the count confirmed by
   re-running `triage.py` rather than by counting your own diff.
3. Every registered feature covered by a batch AutoTest; full suite green via `--test --no-live`
   against the recorded baseline.
4. Every non-retunable field of a registered feature documented at its registration site.
5. PROGRESS.md current; commits local; nothing pushed.
6. Close-out report: coverage before/after, what was confirmed vs inferred, the deferred list with
   reasons, the one claim most likely to be wrong, and the `[EDITOR-VERIFY]` list.
