# PROGRESS — LiveTune coverage sweep

**Started:** 2026-08-06. Executor doc — update before ending any session.

## Baseline (recorded before the first registration)

Full suite via `--test --no-live`: **1022 total / 1022 passed / 0 failed / 0 contaminated**
(`Saved/Logs/Test-Full-Collapse.log`). Any "no regressions" claim diffs against this.

Registry state at start: 3 features registered (Timer, FloatAttribute, Probe) of a 95-feature real
surface. See [TRIAGE.md](TRIAGE.md).

---

## Finding that changes the plan's cost

TRIAGE.md sized bucket A at 53 "live-read, one line each". **That estimate was wrong**, in the
direction PROMPT.md §3 warned about. Scanning every bucket-A feature's `Add()` for params fields read
at construction (`scan_add.py`, kept beside triage.py):

| Add does… | Count | Consequence |
|---|---:|---|
| stores params only | **10** | genuinely a bare `Register<T>({})` |
| reads params fields at Add and bakes them elsewhere | **34** | needs per-field analysis + `.PostApply` or a documented bound |
| no `Add()` found / no `_Utils.cpp` | 9 | manual check required |

Examples of the baked kind: `CrowdAgent(ArrivalRadius, Height, Radius)`, `ShapeBox(InitialDimensions)`,
`FogOfWar(Bounds)`, `Homing(GuidanceSettings, StartingState)`, `VisibleRange(UpdateInterval)`.
Several are identity fields (`Name`, `MarkerName`, `SensorName`, `InteractionChannel`) which are
correctly NOT retunable — but the registration still has to say so rather than let a designer edit
them into silence.

**Second cut, within the clean 10:** 5 add a `FTag_*_NeedsSetup` whose Setup pass derives `Current`
from the params once. Those are NOT bare either — retuning them means re-arming the setup tag from
`.PostApply`, and that is only sound if re-running Setup is idempotent (it creates child entities for
some features). Per-feature judgement, not a sweep.

So the real shape of the work is roughly: **5 trivially bare · 5 setup-tag · 34 per-field ·
21 ViaRebuild · 16 utils-only investigate**, not "53 one-liners".

---

## Finding that changes how EVERY registration is written

The params fragment has **two forms**, and the registry originally handled only one:

| Form | Count | Shape |
|---|---:|---|
| alias | 56 | `using FFragment_X_Params = FCk_Fragment_X_ParamsData` |
| wrapper | **69** | `struct FFragment_X_Params { ParamsType _Params; ... }` |

The default re-apply was `Replace<T_Params>` / `Has<T_Params>`. On a **wrapper** feature that writes a
component the entity does not carry and reports `HasFragment == false`, so `Link` REFUSES a
correctly-registered feature. It would have hit ~69 of the ~92 features in this sweep.

Why a full green suite never revealed it: none of the three shipped pilots exercises the path. Timer
is alias-form, Probe uses a custom `.Apply`, FloatAttribute is `ViaRebuild`. The tiers were
generalised from pilots that all happened to sit on one side of a distinction nobody had named.

Fixed at the registry, not at 69 call sites: `Register<T_Params, T_Fragment = T_Params>`. Alias
features are untouched; wrapper features name the fragment:

```cpp
Register<FCk_Fragment_AutoReorient_ParamsData, ck::FFragment_AutoReorient_Params>();
```

`T_Fragment{params}` covers both — a copy for the alias form, the wrapper's `CK_DEFINE_CONSTRUCTORS`
otherwise. **Check which form a feature uses before registering it**; the compiler only catches it
when a test shim also touches the fragment.

## Batch log

### Batch 1 — trivially bare (no derived state, no setup tag)

Verified by reading each `Add()` and the processors that consume the params fragment.

| Feature | Verdict | Note |
|---|---|---|
| AutoReorient | **REGISTERED + TESTED** | wrapper-form. `Add` stores params only; `_OrientTowardsVelocity` reads live. All fields retune. Pinned by `CkAutoTest_LiveTune_AutoReorient.as`. |
| ResolverSource | REGISTERED | wrapper-form. `Add` stores params only; `_HandleRequests` reads live. All fields retune. **Test still owed** — its only field is an array of resolution phases, so the AS test needs a meaningful two-value fixture. |
| DialogEmitter | pending | 4 `Add` calls — needs the same read-through before registering |
| RaySense | pending | `Add` claims Chaos physics ownership; confirm a retune does not need a re-claim |
| BallisticMotion | DEFERRED | header-only fragment, no `_Fragment.cpp` to host the registrar; adding a TU is its own change |

**Outstanding for batch 1: the per-feature AutoTest.** The user's bar is one LiveTune autotest per
registered feature. The two registrations above are verified by inspection and compile+suite-green,
but are NOT yet runtime-pinned. Next session: add `_AutoReorientParams` / `_ResolverSourceParams`
members to `UCk_LiveTuneTest_TuningAsset` (+ `Get_*MemberName()` accessors) and author one AS test per
feature following `Script/CkEcs/CkAutoTest_LiveTune_TimerViaReplace.as`.

---

## The decision table — start every remaining feature here

`python decision_table.py` -> `decision_table.tsv`. One row per feature resolving the three facts
that have each already cost this campaign a defect when guessed: the fragment FORM (alias vs
wrapper), whether `Add` bakes params fields, and which non-Setup processors read the fragment.

| Status | Count | What it means / what it costs |
|---|---:|---|
| `BARE_OK` | **8** | `Register<T>()` (+ `T_Fragment` if wrapper). Cheapest. |
| `PER_FIELD` | **34** | `Add` bakes params fields — split retunable vs baked, `.PostApply` or document. The bulk. |
| `VIA_REBUILD` | **21** | No params fragment retained. `ReAdd` + capture decision each. Most expensive. |
| `SETUP_BAKED` | **19** | Params read only by Setup — needs a re-arm or a request path. |
| `MANUAL` | 9 | No `Add()` found; read by hand. |
| `OUT_OF_SCOPE` | 19 | `Multiple*` containers. |
| `REGISTERED` | 5 | Timer, FloatAttribute, Probe, AutoReorient, ResolverSource. |

Forms across the in-scope set: **49 alias, 25 wrapper**. Check the FORM column before writing any
`Register` line.

`BARE_OK` today: AudioDirector, BallisticMotion, Compass, DialogEmitter, JoltCharacter, Minimap,
RaySense, VoiceTalker — all alias-form. Note several of these ALSO carry a `NeedsSetup` tag whose
Setup pass derives `Current` from params: a live-read field retunes, but a Setup-only field will not.
`BARE_OK` means "the bare registration is sound for the live-read fields", not "every field retunes"
— state the bound at the call site, per the Timer precedent.

## Remaining

- Finish batch 1's tests, then the other 3 batch-1 candidates.
- The 34 per-field features: each needs its retunable/baked split decided and documented at the call
  site (the Timer precedent).
- The 5 setup-tag features: decide per feature whether re-running Setup is idempotent.
- C1 (21 real) `ViaRebuild`: `ReAdd` + capture decision each. FloatAttribute took three iterations —
  budget accordingly.
- C2 (16) utils-only: investigate individually.
- `Multiple*` (19): out of scope by construction, see TRIAGE.md.
