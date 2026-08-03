---
name: ck-game-debugging-playbook
description: 'Use when CkFoundation gameplay compiles but misbehaves at runtime: missed discovery or signals, ignored requests, invalid handles, interaction stalls, or PIE divergence.'
---

## Overview

Symptom-first triage for the layer ABOVE build failures: the game compiles, the editor boots, and
your feature *silently does nothing*. The framework's `ck-debugging-playbook` owns everything below
(linker/UHT/AS-compile walls, packaged 0xC0000005 crashes, DLL locks) — this skill owns "the code
runs but the behavior is wrong", plus consumer-side usage of the CkGameplayDebugger tooling.

Method: find your symptom in the triage table, run the **discriminating experiment** before
touching code. Nearly every symptom here has 2–3 plausible causes; the observed base rate across
the corpus (BusterBlock, ~7000 commits; incident taxonomy verified 2026-07-03) says start with
**timing** — the entity/tag/binding you relied on did not exist yet at the moment you looked. That
is the single most common failure mode, and the one agents most often skip past.

Jargon, once: **Entity/Handle** — `FCk_Handle`, the typed reference to an ECS entity.
**Fragment** — an ECS component on an entity. **Request** — a deferred mutation (`Request_*`
functions queue; a processor consumes later — root `Plugins/CkFoundation/CLAUDE.md` non-negotiable
#5). **Signal** — the framework's bindable event (`BindTo_OnX` / `UnbindFrom_OnX`).
**EntityScript** — the AngelScript class that composes an entity (the placeable unit).
**Composition** — a feature's `utils_<feature>::Add(...)` stamping fragments/tags/children.
**Ensure** — `CK_ENSURE_IF_NOT`, the house validation macro (dialog in editor, log-only or silent
elsewhere — see §7).

## When NOT to use this skill

| You actually have | Load instead |
|---|---|
| Build/linker/UHT error, AS compile wall, editor won't boot | `ck-debugging-playbook` (framework) |
| Packaged client *crashes* (0xC0000005, GC pool) | `ck-debugging-playbook` §5 |
| "Is this slow?" / profiling / pump-limit warnings | `ck-performance-and-analysis` |
| Writing an AutoTest/Gauntlet test to pin the bug you found | `ck-game-testing-discipline` |
| "Has this failure been seen before?" — incident history | `ck-failure-archaeology` |
| Designing replication topology (not debugging a rep symptom) | `ck-game-replication-patterns` |
| AS language/idiom questions, hot-reload loop | `ck-game-angelscript-gameplay` |
| Understanding WHY the contract is shaped this way | `ckecs-architecture-contract` |

## The triage table — "my feature doesn't fire"

Ordered by observed frequency in the corpus. Start at row 1 even if a later row "feels" right.

| # | Symptom | Most likely cause (ranked) | Discriminating experiment | § |
|---|---|---|---|---|
| 1 | Discovery/binder/scan finds nothing, or misses SOME instances; works when you place things earlier | Target composed after your construct-time scan; tag stamped before composition finished; binder bound after the only broadcast | Log the entity handle + timestamp at compose AND at consume; diff the order. Inspect fragments live via `ck.EcsDebugger` (§6) | §1 |
| 2 | `Request_*` called, nothing happens | You read the result same-tick (deferred contract); target handle invalid at consume time; no processor consumes it | Read one tick later (`WaitOneFrame` in tests / a one-shot timer in game code); check `ck::IsValid(Handle)` at the call site; `stat CkProcessors` shows the consuming processor | §2 |
| 3 | Signal callback never runs (or runs after the entity died) | Wrong binding policy (bound after the broadcast, no replay); producer never fired; unbind leak invoking a dead script; destroy-mid-interaction | Log at broadcast site and at bind site with tick/frame numbers; check the bind's `ECk_Signal_BindingPolicy` | §3 |
| 4 | Handle invalid where "it must be valid"; `ck::ToActor` ensure fires | Stale handle (entity destroyed/pending-kill); entity never had an OwningActor | Walk the validity ladder; use `TryGet_EntityOwningActor` for pawn-less entities | §4 |
| 5 | Interactable focuses (highlight/prompt shows) but interaction never starts | Interaction channel mismatch between source and target; interactable lives on a child probe-node, not the owner | Enumerate the target's channels; compare against the source's channel | §5 |
| 6 | Works on server/host, wrong or blank on clients; value "arrives" pre-filled and callback never fires | Rep-notify bound after the one-shot initial dispatch; replicated spawn under a non-replicating owner; client wrote a server-authoritative value | See routing in §6 → `ck-game-replication-patterns` | §6 |
| 7 | AS change "took" but old behavior persists; types vanish; f-string throws at PIE start | Hot-reload didn't apply; stale `DynamicHandleTypes.json`; runtime-only AS errors | Route: `ck-game-angelscript-gameplay` + `ck-angelscript-interop` §2 | §6 |
| 8 | Behavior differs between PIE and packaged (no crash) | Ensure silence, cook stripping, define gate | §7, then `ck-debugging-playbook` §6 | §7 |
| 9 | Crash/ensure storm on PIE stop, level switch, or entity destroy | Teardown order: bound signals or queued requests outliving the entity | §8 | §8 |

---

## §1. Composition & discovery timing — check this FIRST

**The #1 failure mode.** Entities on this framework compose asynchronously and in stages: an
EntityScript's `DoConstruct` may return `Continue` and finish frames later; child entities spawn
deferred; discovery tags are (correctly) stamped LAST; replicated entities exist on clients before
their values arrive. Any code that scans, binds, or reads "at construct time" silently misses
everything that isn't there *yet* — and everything that arrives *later*.

The full prevention rules (stamp the discovery tag last, delta-gate tag-query results, defer-rescan
for late spawns) are owned by `ck-game-feature-recipe` and `ck-game-driver-architecture`. This
section is the *diagnosis* side.

**Discriminating experiments, in order:**

1. **Log both ends with frame numbers.** At the compose site (end of `utils_<feature>::Add` or the
   EntityScript's construction-finished point) and at the consume site (your scan/bind), log the
   handle and `GFrameCounter`-equivalent context:

   ```angelscript
   // compose side (feature Utils, after the discovery tag is stamped)
   ck::feature::Log(f"[<Prefix>Feature] composed {InHandle.ToString()}");
   // consume side (driver/binder)
   ck::feature::Log(f"[<Prefix>Driver] binding {Found.ToString()}");
   ```

   If the consume line prints before the compose line for the missing instance — timing confirmed.
   (Raw handles don't format in f-strings; always `.ToString()` — `ck-angelscript-interop` §2.)

2. **Inspect the entity live at the failing moment.** `[EDITOR-VERIFY]` In PIE, open the console
   (`` ` ``) and run `ck.EcsDebugger` to toggle the CK ECS Debugger tab (also under Tools → Debug).
   Find the entity, read its fragment list: is the feature's marker fragment there? The discovery
   tag? The State fragment's child handles? A half-composed entity (marker present, tag absent) is
   the "tag stamped after composition" race made visible. Console surface reference:
   `ck-gameplaydebugger-extension` Runbook B "Verify".

3. **Ask "who re-scans?"** A one-shot construct-time scan is only correct if nothing relevant can
   spawn later. In any real level something always does. Corpus examples (BusterBlock, all
   verified 2026-07-03): `66ac804db` "defer-rescan tag discovery so late-composing entities still
   bind"; `1ca589b0d` "reliably discover async-composed gondolas"; `287ee6601` scoping discovery so
   a *second* matching entity elsewhere in the world isn't claimed by the wrong owner. If your scan
   has no late-spawn path, that is the bug regardless of what today's repro looks like.

4. **Bound before the broadcast?** A signal with a future-only binding policy connected one frame
   after the producer's only fire hears nothing, forever (see §3). For replicated values the
   analogue is `d6f9785a4`: on clients the initial replicated values were applied by the one-shot
   rep-notify dispatch *before* the consumer bound — values sat there pre-filled, the callback
   never fired, panels stayed blank. The fix pattern: after binding, do one explicit reconcile read
   of the current state. Never rely on "the first broadcast will initialize me."

**Anti-pattern flag:** if your fix is "add a 0.5 s timer and scan again", you are re-implementing —
badly — the readiness contract the feature should expose (`Promise_OnConstructed`, a
`Promise_OnReady`-style acquire, or delta-gated tag-query updates). Timers hide the race; they
don't close it. (Maintainer-ranked worst-debt category: band-aid bootstraps.)


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Symptom branches §2-§9 | `references/symptom-branches.md` |

## §10. Evidence discipline

- **Reproduce before you claim cause.** One observation ranks hypotheses; it doesn't pick one.
  The corpus flake studies (§11) each required multiple runs before the real cause separated from
  the coincidental one.
- **The stale-green trap.** A green run that predates your latest edit — or ran against stale
  binaries/old AS — proves nothing. Rebuild/recompile, rerun, then claim. Canonical telling:
  `ck-debugging-playbook` "Common mistakes → Stale-green"; the gating discipline is
  `ck-change-control` / `ck-methodology`.
- **Read the actual verdict, not a proxy.** A wrapper script's exit banner, a "completed"
  notification, or a grep narrowed to your own file is not the result. Open the fresh log; find
  the harness's own verdict line and the error/warning summary. For AS: the reload verdict lands
  seconds after save — check it before running anything (a test can't fail "for your reason" if
  your class never compiled).
- **Isolate own-change vs pre-existing.** Before debugging a red state, stash your change and
  re-run once. Inherited breakage debugged as if you caused it is the most expensive wrong turn
  in this playbook.

## §11. Worked examples — three root-caused flakes (the method in action)

From the BusterBlock Gauntlet corpus (`Source/BusterBlock/Tests/Gauntlet/README.md`,
`PlayerOperatesCheckout` row; verified 2026-07-03). Corpus examples — the *method* generalizes,
the specifics are BB's:

1. **MoveDirection drift.** Intermittent: the pawn engaged a station, then "randomly" lost the
   engage trace. Ranked causes: trace flake? station bug? Two-point observation showed the nav
   layer kept pushing a movement input every tick after arrival — the pawn slowly drifted off the
   use-spot. Fix: zero movement each tick while engaged. Lesson: the flake lived in an *adjacent
   system still running*, not in the failing system — ask "what else writes this state every tick?"
2. **Nondeterministic construction order.** A sequence of spawned sub-entities was indexed by
   position in a values array; construction order was nondeterministic, so 2 of 3 runs picked the
   wrong one. Fix: identify each host by a **binding fragment** stamped on it
   (`FBb_Fragment_CheckoutSettle_Binding` on the host root), never by index. Lesson: any code
   indexing into "the order things constructed" is a latent flake — key by identity fragment/tag.
3. **Alignment-gated clicks.** Injected clicks during a camera pan intermittently landed on a
   *neighboring* probe. Fix: gate the click until aim is within 4° of the target. Lesson: when an
   input-shaped action "sometimes hits the wrong thing", instrument WHERE it landed before
   suspecting the handler.

Common thread: each flake was closed by **observing the intermediate state** (position each tick,
which host got picked, where the trace landed) — not by retrying until green or padding delays.

## Common mistakes

1. **Jumping to row 3–6 when row 1 is the cause.** Timing first, always. If placing the target
   earlier / spawning it before the consumer "fixes" it, you've *confirmed* a §1 bug — now fix the
   contract (readiness promise / delta-gated rescan), not the placement.
2. **Fixing a race with a timer/delay.** Hides it in PIE, resurfaces packaged or under load. §1
   anti-pattern flag.
3. **Reading a request's effect same-tick** — then concluding the feature is broken. §2.
4. **Suspecting the producer before checking the bind's policy argument.** §3a — one enum, minutes
   to check, the most common signal bug.
5. **Binding cross-entity signals without an EndPlay unbind** — works until the first mid-session
   destroy, then §3c. Grep your EntityScripts: every cross-entity `BindTo_` should pair with an
   `UnbindFrom_` or a stated reason it can't leak.
6. **Trusting "it focuses" as "interaction works".** Focus and engage are different channels. §5.
7. **Debugging a client-side rep symptom as local logic.** Blank client value + callback never
   fired = bind-after-dispatch (§1.4/§6), not a broken feature.
8. **Claiming "no ensure fired" from a Test/Shipping/`-unattended` run** without grepping the log.
   §7.1.
9. **Trusting a green run that predates the edit, or a wrapper's exit banner.** §10.
10. **Adding prints when the feature already has a log function and the debugger can show the
    fragment live.** Turn up the existing surface first — it's already load-bearing and stays
    useful after you're done.

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock superproject detached HEAD `52a75e13d` (dev tip),
CkFoundation framework skills dated 2026-07-02. Venus corpus at `D:/Repos/Venus`
(`feature/state-machine`, CkFoundation pinned 2026-03-25). Framework citations are by skill name +
section heading (stable); commit exhibits are BusterBlock superproject SHAs.

Re-verification (Git Bash, cwd = BusterBlock superproject root; NOTE: repo-root `.ignore` hides
`Script/`, plugin `Script/`, `docs/`, `Content/` from the agent Grep/Glob tools — use
`rg --no-ignore` there, always re-check a zero-match):

- Commit exhibits still real:
  `git show --stat 05d2dc527 de3099e1c e0de34899 126ab3ac6 d6f9785a4 66ac804db 1ca589b0d 287ee6601 585fd6035 3688dc72b a31e46f13 742867535`
- Binding-policy enumerators verbatim: root `Plugins/CkFoundation/CLAUDE.md` Signals block, or
  `rg -n 'enum class ECk_Signal_BindingPolicy' -A 10 Plugins/CkFoundation/Source/CkEcs`
- Debugger console surface: `ck-gameplaydebugger-extension` SKILL.md tables;
  `rg --no-ignore -n 'ck\.DebugOverlay|ck\.EcsDebugger' Plugins/CkGameplayDebugger/Source`
- Ensure watermark: `rg --no-ignore -n 'Get_EnsureCount' Plugins/CkFoundation/Source/CkWatermark`
  (widgets `CkWatermarkStat_EnsureCount_Widget` / `_EnsureCountUnique_`); enable/rows are
  project-settings driven — `[EDITOR-VERIFY]` in Project Settings → Watermark
- Gauntlet flake rows: `grep -n -i 'MoveDirection\|nondeterministic\|alignment' Source/BusterBlock/Tests/Gauntlet/README.md`
- Venus DebugSettings exemplar:
  `sed -n '125,165p' D:/Repos/Venus/Script/ECS/Targeting/Vns_Targeting_Feature.as`
- Breadcrumb-constant exemplar:
  `rg --no-ignore -n 'k_Breadcrumb' Script/ECS/StoreManager/BB_StoreManager_Feature.as`
- CkTimer PostFire drop: framework `Plugins/CkFoundation/.claude/reports/DECISIONS.md` §36
- Destroy-mid-interaction defect status: `ck-lifecycle-teardown-campaign` (a live campaign — if it
  has concluded, soften §3d/§8.3 to its outcome)

Volatile facts: the frequency ordering of the triage table reflects the 2026-07 BusterBlock
incident corpus; re-rank if a future consumer's incident mix differs. The `Script/Dev/` staging
convention (§7.2) was BusterBlock's fix shape at `e0de34899` and later evolved (tests moved into an
editor-only plugin) — the invariant to teach is "dev-only AS must not stage into Shipping", not the
directory name.
