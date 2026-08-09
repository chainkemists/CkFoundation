# Phase 4 — intent grammar, notation parser, compiled sets, resolution tables

> **Status:** ✅ CLOSED (2026-08-08, same session) — gates 103/103 (`Ck_AutoTest_In`, both
> grammar AS rows by name) + 10/10 (`Ck.Intent.Grammar` C++ battery). One build break
> mid-close (missing `CkInput` link dep in CkTests.Build.cs — first C++ TU constructing a
> ButtonId), root-caused + fixed inline. Comment audit clean. Grammar + button model remain
> RECONSTRUCTIONS flagged for maintainer review ([P4-D1], [P2-D2]). Full-suite delta-zero
> deferred per [P2-D4]. **Depends on:** Phase 3 (✅ — the frame record the
> matcher will scan; this phase only needs its ButtonId/octant vocabulary). **Gate regime:**
> scoped per [P2-D4] — the parser/bake tests are hermetic C++ (their own pattern) + AS
> authoring-surface tests under `Ck_AutoTest_In`.
> **Scope of record:** PROMPT.md phase-index row 4: "Intent grammar + notation parser;
> compiled sets + per-set resolution tables under D7; cycle validation." Governing decisions:
> D7 (deferral from forward ambiguity ONLY), D8 (`FCk_Intent_CompiledSet`, O(1) swap, no
> mid-match rebake), D9 (compact notation IS the authoring surface, one parser shared with
> the test fixture), D11 (charge stays out of the matcher — Phase 5).
> **The original design doc that specified the notation examples is lost** (superseded,
> uncommitted). The grammar below is a RECONSTRUCTION from D9's surviving example
> (`"236+LP w=200 lenient"`) and genre convention — flagged for maintainer review like
> [P2-D2].

## Rulings at phase open

- **[P4-D1] The grammar, v1 (reconstruction — maintainer review flag):**
  - **Direction atoms:** numpad digits `1`-`9` (`5` = neutral) mapping onto Phase 3's
    `ECk_Intent_Octant` vocabulary (e.g. `6` = E, `8` = N, `2` = S; diagonals accordingly).
    A run of digits is a SEQUENCE of direction steps (`236` = three steps).
  - **Button atoms:** names resolved against ButtonId names at BAKE time (tier-agnostic —
    the notation says `LP`, the bake asks the map/definitions which ButtonId that is via a
    name→ButtonId table supplied to the bake; unknown names are a bake REJECTION, not a
    runtime surprise).
  - **Chord:** atoms joined by `+` are simultaneous within the chord window (`6+LP`). A
    chord is ONE step.
  - **Sequence:** whitespace-separated steps (a digit-run expands to per-digit steps first).
    The LAST step is the terminal — the press that completes the intent (D7's backward scan
    anchors here).
  - **Modifiers (trailing, whitespace-separated, order-free):** `w=<frames>` — the whole-
    sequence window in LOGIC FRAMES (Hz(60) — frames, not ms: frame-determinism is the
    campaign's spine; D9's `w=200` reads as frames under this ruling). `lenient` — the
    matcher may skip transient unmatched directions BETWEEN sequence steps (standard motion
    lenience; exact matcher semantics are Phase 5's — the flag parses and bakes now).
    `hold=<frames>` — terminal must be held N frames (creates D7 forward ambiguity; parsed
    + baked now, matched in Phase 5).
  - Grammar is FLAT: no macros, no intent-references-intent, no conditionals (PROMPT's D9
    revisit clause: "if the notation grows conditionals — forbid that instead").
- **[P4-D2] The definition model:** `FCk_Intent_Definition` = name (FName), gameplay tag
  (the `Intent.*` namespace per 0I — carried, not enforced yet), the parsed step list, the
  modifier set, and an int32 priority (explicit, CkInput-layer precedent; ties broken by
  bake REJECTION, not silently — arbitration order must be unambiguous). Definitions are
  produced ONLY by the parser (one parser, D9 — the test fixture calls the same entry
  point).
- **[P4-D3] The bake:** `FCk_Intent_CompiledSet` = the validated definitions + a per-
  terminal-ButtonId **resolution table** (which intents' backward scans a press of that
  button triggers, priority-ordered) + a per-ButtonId **deferral verdict** computed under
  D7: deferral exists ONLY where a hold sibling or a chord-membership forward-ambiguity
  exists on that button; sequence-suffix membership NEVER defers (this is success criterion
  2's law, baked as data the matcher just reads). Baking is a pure function
  (definitions + name→ButtonId table) → compiled set; it performs ALL validation and
  returns rejection reasons; an invalid set is never partially usable (atomic, house law).
- **[P4-D4] "Cycle validation" (reconstruction):** with a flat grammar the only cyclic
  structure the bake can produce is the DOMINANCE relation used for arbitration (intent A
  outranks B on shared terminals). The bake validates that the priority relation over each
  resolution table is a strict total order — equal priorities on a shared terminal are the
  "cycle" and are rejected with both intent names in the reason. If the lost design meant
  something else by cycle validation, this ruling is the falsifiable stand-in.
- **[P4-D6] (ruled at 4-2 dispatch)** The chord simultaneity window is a **BAKE parameter**
  (`ChordWindowFrames`, uniform per compiled set, default 3 logic frames ≈ 50 ms of human
  "simultaneous"), not per-intent notation — the grammar has no chord-window syntax and D9's
  revisit clause resists growing it. It feeds the D7 chord-membership deferral verdicts.
  *Revisit if a game needs per-move chord timing — that becomes a notation modifier then.*
- **[P4-D7] (ruled at 4-2 review)** Duplicate rows in the bake's name→ButtonId input:
  same name → SAME ButtonId is idempotent (accepted, one entry); same name → DIFFERENT
  ButtonId is two answers to one question and REJECTS (`ConflictingButtonRow`, naming the
  button name and both ButtonIds). First-match-wins-silently — the unit's interim behavior —
  violated non-negotiable #3 and is dead.
- **[P4-D5] Where it lives:** all of this is pure data + free functions in CkIntent
  (`CkIntentGrammar_*` files or similar) — NO fragments, NO processors, NO entities this
  phase. The compiled set becomes ECS state in Phase 5/6 (an SM activates a set as an O(1)
  swap per D8). AS/BP surface: a `UCk_Utils_IntentGrammar_UE` with `Parse` (string →
  definition + error), `Bake` (definitions + name→ButtonId rows → compiled set + errors) —
  non-negotiable #4 says all three environments exercise it.

## Units (sequential — 4-2 consumes 4-1's model)

**4-1:** grammar model + parser + parse-time validation + hermetic C++ parse tests
(round-trips, every malformed-input class rejects with a reason, the D9 example parses to
the documented shape) + the AS-visible `Parse` surface + one AS AutoTest proving the AS
authoring path (notation strings in an asset-shaped container parse identically).

**4-2:** bake + resolution tables + D7 deferral verdicts + [P4-D4] validation + hermetic
C++ bake tests (deferral verdicts: button with hold sibling defers, sequence-suffix-only
button does NOT — success criterion 2's edge; priority-tie rejection names both intents;
unknown button name rejects atomically) + AS `Bake` exercise.

## Exit criteria

- [ ] Scoped gates green: the new C++ pattern + `Ck_AutoTest_In` (all 101 + new AS rows)
- [ ] `CkIntent/Claude.md` extended: notation grammar reference (the authoring doc a
      designer reads), bake contract, D7-as-data explanation
- [ ] PROGRESS decision log + dated entries current; comment audit run
- [ ] Full suite NOT run (deferred, [P2-D4])

## NOT in this phase

No matcher/backward scan execution (5), no charge accumulators (5, D11), no arbiter
runtime (5), no intent fragments/signals/phases (6), no ECS state at all ([P4-D5]), no
per-move struct authoring in AS (D9 — the notation string is the surface), no `Intent.*`
tag registration (0I's namespace decision is still open — definitions carry a tag field,
nothing mints tags).
