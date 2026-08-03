---
name: ck-game-framework-boundary
description: >-
  Use when game code would bypass CkFoundation public APIs, mutate fragments directly, add globals,
  copy internals, or when a game pattern may belong upstream; not for normal feature composition.
---

# ck-game-framework-boundary — the contract between a game and CkFoundation

## Overview

CkFoundation (the Ck plugin suite, consumed as git submodules) and your game are two codebases
with one boundary. The framework's side of the contract is documented in its own doctrine chain
(root `Plugins/CkFoundation/CLAUDE.md` → `Source/CLAUDE.md` → `Script/CLAUDE.md` → the `ck-*`
skills). This skill is the **consumer's side**: the things a game may never do (and why each rule
exists), the ritual to follow when the framework genuinely doesn't support what you need, the
path for promoting a proven game pattern INTO the framework, and the obligations a game carries
back upstream.

The one-sentence version of the whole contract:

> **A game composes the framework through its public surfaces — `utils_*` / `UCk_Utils_*_UE`,
> EntityScripts, requests, signals, providers — and never reaches past them. When a surface is
> missing, the game files a gap; it does not tunnel.**

Jargon used below, defined once (full vocabulary: root CLAUDE.md "Lingo" table): a **fragment**
is an ECS component; a **processor** is an ECS system; a **request** is a deferred mutation
struct queued on a `_Requests` fragment; **Utils** (`UCk_Utils_[Feature]_UE`, reached from
AngelScript as `utils_[feature]::`) is the ONLY public API surface of a feature; an
**EntityScript** (`UCk_EntityScript_UE`) is the data-driven entity logic unit games author.

## When NOT to use this skill

| You are actually trying to… | Load instead |
|---|---|
| Compose an existing framework feature into your game (the normal path) | `ck-game-feature-recipe`; module lookup = `Source/CLAUDE.md` "Finding the right module — 'I need to…'" |
| Structure entities/lifetimes/archetypes within the contract | `ck-game-entity-composition-patterns` |
| Modify the framework itself (you've decided the change belongs there) | `ck-change-control` (gates) + the relevant framework skill |
| Decide what the framework should build/harden next | `ck-feature-frontier` |
| Diagnose a failure that might be a framework defect | `ck-game-debugging-playbook` first; this skill only tells you where the report goes afterward |
| Replicated-spawn ownership mechanics (ActorRelay channels) | `ck-game-replication-patterns` |

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The MAY-NEVER list | `references/may-never-list.md` |

## 2. The framework fights you: workaround vs gap

Most "the framework can't do X" moments are wrong-module choices or timing misuse, not gaps.
Run this ritual in order; do not skip to (d).

**(a) Re-read before concluding.** The target module's `Source/<Module>/Claude.md`, then
`Source/CLAUDE.md`'s "I need to…" decision tree (~70 rows — the capability usually exists under
a name you didn't guess). For AS-surface confusion ("the function exists but won't resolve"),
that's `ck-angelscript-interop` §1.2/§1.4, not a gap.

**(b) Check history.** Load `ck-failure-archaeology`: has this been tried, rejected, or already
worked around deliberately? A "gap" that matches a fenced branch or a recorded revert is a
settled NO with reasons — cite the entry instead of re-litigating.

**(c) If it's real: file the gap.** The framework's standing intake (root non-negotiable #6:
"ask, or add the fork to ADJUDICATIONS.md"):

1. **Ask the maintainer** — their recorded standing instruction is "the agent just needs to ask
   me" (ADJUDICATIONS.md header).
2. **Record it durably** in the framework repo — a docs-only change, the lightest
   `ck-change-control` class:
   - Convention fork / unwritten norm → append an entry to
     `Plugins/CkFoundation/.claude/reports/ADJUDICATIONS.md` (both sides, evidence, interim
     stance — match the existing entry shape).
   - Missing capability / feature-sized ask → a candidate against `ck-feature-frontier`'s
     portfolio shape (problem, evidence, ranked robustness > perf > tooling).

   A gap report is only actionable with: **minimal repro** (smallest AS/autotest demonstrating
   the missing capability) + **desired-API shape** (the `utils_*` call you wish existed, with
   in/out types). No dedicated GAPS.md/issue-tracker convention exists in the corpus (verified
   2026-07-03 — zero hits for any gap-tag convention across game + framework docs); the
   ask-list + frontier are the channels until the maintainer creates one.

**(d) Interim: the least-entangled local workaround.** While the gap is open, prefer (in order):
compose around it with public surfaces > a game-side wrapper namespace that isolates the ugly
call to ONE file > a game-side reimplementation. Never a framework edit, never an internals
copy-paste. Mark every workaround site with a standard tag so it is findable when the gap
closes:

```angelscript
// CK-GAP(2026-07-03): utils_probe has no <needed capability>; workaround = <one line>.
// Remove when <ADJUDICATIONS/frontier entry> lands.
```

**[PROPOSAL — not corpus practice]** The `// CK-GAP:` tag is proposed by this skill (verified
2026-07-03: zero existing occurrences). Until adopted, its value is that `rg --no-ignore
'CK-GAP'` enumerates your debt; adjust the spelling if your project rules differently, but keep
*some* greppable tag.

---

## 3. Promotion INTO the framework

The boundary is a membrane, not a wall — proven game patterns should migrate up. The framework's
own test harness demonstrates the end-state: the gym/autotest machinery is project-agnostic in
`Plugins/CkTests/Script/Common/` while each game registers its own content
(`CkGym_Cycler::RegisterProjectGym(...)` calls in the game's registry file — corpus example:
`Plugins/BusterBlockTests/Script/Gyms/BB_Gyms_Registry.as:19+`, verified 2026-07-03). That
framework-owns-the-mechanism / game-owns-the-content split is what a successful promotion looks
like. ActorRelay shows the incident-driven route: consumer-visible replication-ownership failures
drove a framework rearchitecture, and ActorRelay channels are now "the house answer" to
replicated-thing ownership (`ck-failure-archaeology` entry 7).

**Nomination criteria — all three, or don't nominate:**

1. **Proven by reuse:** used by ≥2 features in one project, or by ≥2 projects.
2. **No game-specific data:** the mechanism survives with every `<Game>` type/tag/asset factored
   out into registration or params (the gym-harness test: could another game adopt it by writing
   only a registry file?).
3. **Framework values compatible:** friend-gated encapsulation with a Utils surface, deferred
   requests, loud errors, three-environment verifiability, measurable (no perf claim without a
   benchmark — root non-negotiable #7).

**Nomination path:**

- Small/medium (a utility, a pattern, a harness extension) → file it exactly like a gap (§2c):
  ask + a frontier-shaped candidate write-up (problem, evidence of reuse, proposed API,
  robustness/perf/tooling class). `ck-feature-frontier` is the framework's intake and ranking
  frame for "what should the framework build".
- Large (a new module, an architectural pattern) → additionally run the CTO-review pattern from
  `ck-methodology` §8: a brief with the decision points surfaced, answered before code moves.
- Either way, the code MOVE itself is a framework change: `ck-change-control` gates apply, and
  the game keeps a thin compatibility shim only for the deprecation window the maintainer sets.

---

## 4. What games OWE the framework

The contract runs both ways. A consuming game owes:

1. **Incident reports upstream.** A game-side crash, flake, or silent misbehavior whose root
   cause is a framework defect belongs in the framework's failure archaeology, not just your
   game's postmortem channel — file it with symptom → root cause → evidence (commit/file:line) →
   status, matching `ck-failure-archaeology`'s entry shape. The framework's binding-policy
   enums and teardown campaign both exist because consumer incidents were reported, not absorbed.
2. **Current, pushed submodule pointers.** Advance your gitlinks regularly (a months-stale
   pointer makes every gap report suspect — the gap may already be fixed), and never publish a
   superproject commit whose gitlink references an unpushed framework SHA. Corpus example
   (BusterBlock, host-specific — your project's guards will differ): `CkAuto/` batch scripts for
   suite-wide submodule pull/push, plus a pre-push containment check that framework `HEAD ⊆
   origin/dev`.
3. **No silent forks.** If you must carry a framework patch locally (emergency fix ahead of
   upstream), it is a loan: branch the submodule, mark the superproject clearly, file the change
   upstream immediately, and drop the local branch when it lands. A submodule that quietly
   diverges from upstream is the worst of both worlds — you pay merge cost forever and the
   maintainer debugs a codebase that isn't theirs.
4. **Doc-drift reports.** Per-module `Claude.md` files can go stale (framework DECISIONS.md §15
   records known cases); when code contradicts a framework doc you relied on, say so in the same
   channels (§2c) — "trust code over doc on conflict and note the drift" is the framework's own
   standing rule (root CLAUDE.md "Where things live").
5. **Skill/doc sync.** Consume framework skills via the provided junction script
   (`Plugins/CkFoundation/.claude/scripts/sync-skills.ps1`) rather than copying them — copies
   are silent forks of documentation.

---

## 5. The boundary in numbers (the contract works)

Evidence from the largest corpus consumer (BusterBlock, ~186k lines of hand-written game
AngelScript; all searches run 2026-07-03 with `rg --no-ignore` — plain Grep/Glob are blind under
`Script/` dirs; commands in Provenance):

| Boundary property | Measured |
|---|---|
| Game-authored C++ fragments (`struct FFragment_` / `ck::FFragment_` in game C++) | **0** |
| Game-authored C++ processors (`CK_REGISTER_PROCESSOR` in game C++) | **0** (135 processors exist — all AS `UCk_Processor_Script_Base_UE` subclasses, the sanctioned path) |
| Direct `_Requests` container manipulation in game code | **0** in code — 10 textual hits are all comments naming the game's OWN AS request fragments (e.g. `FBb_Fragment_Trashcan_Requests`), i.e. correct game-side composition |
| Raw registry access (`entt::`, `Get_Registry`, `.storage<`) in game code | **0** |
| `friend` declarations added in game C++ | **0** |
| Stock `ensure`/`ensureMsgf`/`check` in game C++; stock `check(` in game AS | **0** and **0** — vs 221 `ck::EnsureIfNot`/`ck::Ensure` uses in game AS |

CONFORMANCE_BACKLOG from these searches: **none** — no friend-boundary or request-pipeline
breaches found. A ~186k-line game shipped inside the contract with zero tunneling; the escape
hatches were never needed because composition + gap-filing covered every case.

## Common mistakes

1. **"It's just one direct write, the request round-trip is overkill."** The request pipeline is
   root non-negotiable #5, and the deferral is load-bearing (iteration safety, replication,
   ordering — `ckecs-architecture-contract` §3). If you need the value this frame, the answer is
   a signal payload or a settle, not a bypass.
2. **Declaring a gap after one failed call.** ~All "gaps" die at ritual step (a) — the wrong
   Utils namespace, a mixin call-form issue, or a timing misuse (composing before discovery
   settles — see `ck-game-feature-recipe`'s timing warning). File gaps only after (a) and (b).
3. **Fixing the framework in-place during a game session** because the defect blocked you.
   Stop, switch contexts, load `ck-change-control`, make the change in the submodule repo under
   its gates, push, then bump the gitlink. The five-minute "quick fix" that skips this becomes an
   unpushed-SHA gitlink or an unversioned plugin binary.
4. **Copying a framework processor/fragment into game code "temporarily."** The copy compiles
   forever and drifts forever. That impulse is the exact trigger for §2 — report and wrap
   instead.
5. **Promoting too early.** A pattern used once, in one feature, in one game is a
   `[SINGLE-EXEMPLAR]` — record it and wait for the second consumer before nominating (§3
   criterion 1).
6. **Log-and-continue as "defensive coding."** A `Warning` where validation failed is a review
   rejection framework-side and should be game-side too: logs get ignored, ensures do not
   (root #3).

## Provenance and maintenance

Authored 2026-07-03 against CkFoundation framework docs self-dated 2026-07-02 (root CLAUDE.md,
Source/CLAUDE.md, ck-change-control, ckecs-architecture-contract, ck-failure-archaeology,
ck-feature-frontier, ck-methodology) and the BusterBlock corpus at superproject HEAD `52a75e13d`.
Maintainer rulings dated 2026-07-03 (AS-first, no-Actors, global-access-is-worst-debt, driver
endorsement) were provided as settled doctrine for this campaign.

Re-verify the volatile claims (Git Bash from the game repo root; `--no-ignore` is mandatory —
the superproject `.ignore` hides `Script/` dirs from ripgrep-based tools):

```bash
# §5 numbers — expect zeros (adjust game paths for your project):
rg -n 'struct\s+FFragment_|ck::FFragment_|CK_REGISTER_PROCESSOR' Source/<Game> Plugins/<Game>*
rg --no-ignore -n '_Requests\.' Script Plugins/<Game>Tests/Script -g '*.as'   # code hits, not comments
rg --no-ignore -n 'entt::|Get_Registry|\.storage<' Script Source/<Game> -g '*.as' -g '*.h' -g '*.cpp'
rg -n '\bfriend\b' Source/<Game>
rg --no-ignore -n '\b(ensure|ensureMsgf|check)\(' Source/<Game>; rg --no-ignore -n '\bcheck\(' Script -g '*.as'

# Doctrine anchors still where cited:
rg -n 'runtime code must NEVER depend' Plugins/CkFoundation/Source/CLAUDE.md
rg -n 'logs get ignored' Plugins/CkFoundation/CLAUDE.md
rg -n 'the agent just needs to ask me' Plugins/CkFoundation/.claude/reports/ADJUDICATIONS.md

# CK-GAP adoption status (this skill's §2d proposal — promote from PROPOSAL if hits appear):
rg --no-ignore -in 'CK-GAP' Script Source Plugins

# Gym harness split (promotion exemplar):
ls Plugins/CkTests/Script/Common/ | grep -i gym
rg --no-ignore -n 'RegisterProjectGym' Plugins/<Game>Tests/Script -g '*.as'
```

Staleness triggers: a maintainer ruling on ADJUDICATIONS A1/A2/A4 (this skill cites none as
settled, but sibling skills do); creation of a dedicated framework gap/issue file (§2c channels
would change); adoption or rejection of the `// CK-GAP:` tag; any nonzero result in the §5
re-verification (that's a new CONFORMANCE_BACKLOG item, and this skill's "zero breaches" claim
must be amended).
