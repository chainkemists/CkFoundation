# CkAggro

**Purpose:** Threat tracking + active-target selection for AI, designed for thousands of concurrent
owners. Each Aggro owner maintains a threat table of tracked targets; threat decays analytically,
targets are scored + evaluated in parallel on a staggered clock, and a single active target is
selected with hysteresis. Damage-driven threat is a one-line request from the game.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** NPC combat AI (target selection, retaliation, taunts).

**Authority-only.** No Aggro state replicates. Every processor is
`NetModeRequirement = AuthorityOnly`. **Composing Aggro is the game's authority-side responsibility**
— add it inside authority-gated construction, never on a pure client.

---

## Shape

Two typesafe handles, two feature quartets:

- **`FCk_Handle_Aggro`** (`UCk_Utils_Aggro_UE`) — the owner brain, direct-attached to the AI entity
  (requires a Transform). Holds the threat table (a record of child targets + an O(1)
  `TMap<TrackedEntity, AggroTarget>`), the active target, and the evaluation pacer clock.
- **`FCk_Handle_AggroTarget`** (`UCk_Utils_AggroTarget_UE`) — one child entity per tracked target.
  Holds threat (analytic-decay anchor), score, perception, and identity (owner/instigator/source/
  tracked-entity). Per-game heuristics attach as extra fragments on it.

### Core performance idea — no per-frame per-target work

- Threat **decay is analytic**: computed from a timestamp anchor at evaluation time, never integrated
  per tick.
- The only always-ticking cost is **one chrono tick per owner** (the pacer). All per-target math runs
  in one Evaluate processor (`FProcessor_AggroTarget_Evaluate`), only on targets stamped
  `EvaluationPending` (staggered clock fire, or a threat request marking them dirty).
- **Evaluate is PARALLEL** (`TParallelProcessor`). Because the target is self-sufficient, the per-target
  body reads its OWN params from the view — the owner is touched ONLY for its transform (distance) and
  the deferred re-select stamp (owner+tracked transforms via worker-thread handle reconstruction) —
  plus writes to the target's own Threat/Score fragments; every structural tag flip —
  `WithinRetention` + `PendingForget` + `EvaluationPending` on self, `SelectionPending` on the owner —
  is DEFERRED through the read-only handle's per-task command buffer and flushed single-threaded, so
  no worker touches the registry structurally. `Now` is hoisted to the game thread once per tick
  (`DoTick` override) — a worker-thread `UWorld` time read is neither safe nor needed (one world per
  registry). The decay/score/forget/selection model was proven + tested serially first, then
  parallelized behind the same test suite. Discipline mirror: `CkCrowdAgent_NeighborSync`.
- Reactive bursts (a big hit, a taunt) stamp `EvaluationPending` + `SelectionPending`; the pump drains
  request → evaluate → select in the same frame.

---

## Key API

```cpp
// Compose (authority-side, requires a Transform):
auto Aggro = UCk_Utils_Aggro_UE::Add(Entity, FCk_Fragment_Aggro_ParamsData{});

// Damage -> threat, one line (game binds Resolver's OnAllPhasesComplete and calls this):
UCk_Utils_Aggro_UE::Request_AddThreat(Aggro,
    FCk_Request_Aggro_AddThreat{TrackedEntity, ThreatAmount});   // creates the target on demand

// Read selection:
auto Active  = UCk_Utils_Aggro_UE::Get_ActiveTarget(Aggro);          // FCk_Handle_AggroTarget
auto Tracked = UCk_Utils_Aggro_UE::TryGet_ActiveTrackedEntity(Aggro);

// Taunt (bypasses hysteresis), enable/disable, perception feed:
UCk_Utils_Aggro_UE::Request_SetActiveTarget(Aggro, TrackedEntity);
UCk_Utils_Aggro_UE::Request_EnableDisable(Aggro, ECk_EnableDisable::Disable);   // counted
UCk_Utils_Aggro_UE::Request_MarkPerceived_ByTrackedEntity(Aggro, TrackedEntity, {});
```

Target creation (immediate, no `Request_` prefix):
- `UCk_Utils_Aggro_UE::CreateTarget(Owner, Tracked)` — the accelerant one-shot; stamps the owner's
  `DefaultTargetParams` onto a new child (dedupe + cap/evict + record + map).
- `UCk_Utils_Aggro_UE::CreateTarget_WithParams(Owner, Tracked, Overrides)` — same, with per-section
  (`Threat`/`Spatial`/`Forget`/`Score`/`Lifetime`) override toggles on `FCk_AggroTarget_ParamOverrides`.
- `UCk_Utils_AggroTarget_UE::Add(Handle, Params)` — direct-attach the self-sufficient feature to ANY
  entity (no owner, no record — standalone threat tracking; tracks `Handle` unless Params names a subject).
- `UCk_Utils_AggroTarget_UE::Create(Owner, Params)` — new child under Owner + `Add` + record-connect.
  Takes a GENERIC `FCk_Handle` owner: AggroTarget knows nothing of the Aggro type. (`AddTarget` is gone.)

Owner signals (broadcast from serial processors only — the parallel Evaluate must NEVER broadcast a
signal or fire a delegate; those hop to the game thread and are unsafe from a worker): `OnAggroTargetAcquired`,
`OnAggroActiveTargetChanged`, `OnAggroTargetForgotten` (carries `FCk_Aggro_TargetForgottenInfo` — it
outlives the dying entity). Target signal: `OnAggroThreatChanged` (request-driven changes only, never
decay/eval spam).

---

## Tunables — the target is self-sufficient; the owner is the accelerant

Every per-target behavior the `Evaluate` stage reads lives on the TARGET (`FCk_AggroTarget_*`), so a
target created via `AggroTarget::Add` works standalone. The OWNER carries only owner-level concerns +
the default-target template it stamps onto new targets.

**Target params** (`FCk_AggroTarget_*`, aggregated in `FCk_Fragment_AggroTarget_ParamsData`):

| Piece | Governs |
|---|---|
| `ThreatParams` | initial threat, threat multiplier, decay rate, unperceived-decay multiplier, min-tracked floor, clamp range, optional max-override |
| `SpatialParams` | acquisition/retention distance (cm), out-of-range decay multiplier, distance falloff (half-distance + exponent), optional nearby-preference band |
| `ForgetParams` | forget duration, lost-sight grace, optional max age (per-target inactivity/age rules) |
| `ScoreParams` | `_ScoreBias` / `_ScoreMultiplier` — the game-heuristic escape hatch |
| `LifetimeParams` | max lifetime, can-become-active, can-be-forgotten |

**Owner params** (`FCk_Fragment_Aggro_ParamsData`):

| Piece | Governs |
|---|---|
| `DefaultTargetParams` | the `FCk_Fragment_AggroTarget_ParamsData` template stamped onto new targets by `CreateTarget` |
| `FCk_Aggro_SelectionParams` | switch threshold, switch cooldown, current-target bias, min score, min aggro duration (the four hysteresis gates) |
| `FCk_Aggro_CapParams` | target cap mode + max tracked targets + eviction policy (owner-level set management) |
| `FCk_Aggro_EvaluationParams` | evaluation interval + per-rearm jitter (fleet decorrelation) |

`CreateTarget` copies `DefaultTargetParams` onto each new target; `CreateTarget_WithParams` overrides
individual sections. There is no runtime "inherit from owner" — the target owns concrete values.

Scoring is continuous distance falloff + four hysteresis gates. There is deliberately **no hard
"nearest wins" rule** (discontinuous → switch-thrash; priority inversion). Raise
`_DistanceFalloffExponent` to approach nearest-wins smoothly, or enable the bounded `_NearbyPreference`
band for "brawler snaps to whoever's in its face".

---

## Perception feeders

Perception affects **decay rate and memory only, never score** — an unperceived-but-remembered
attacker keeps its earned threat and fades by decay. `FTag_AggroTarget_Perceived` is a **counted** tag:
N independent senses each vote; N `MarkUnperceived` (or one `Request_ResetPerception`) clears it.

The module has **no feeder dependency**. Candidate feeders the game wires up: `CkRaySense`, `CkEqs`,
`CkSpatialQuery`, a future `CkPerception`. Damage→threat: the game binds `CkResolver`'s
`OnAllPhasesComplete` and calls `Request_AddThreat` (receiver veto is receiver-side game code before
the call — no framework veto machinery).

---

## Anti-patterns

1. **No line-of-sight traces in Aggro.** The old distance+LoS design is gone. Reachability and
   visibility are the game's concern — the game attaches its own fragment and drives
   `Request_Forget` / `CannotBecomeActive`. Aggro never touches a `UWorld` trace.
2. **Never sort the record.** The child record is for ownership/teardown/enumeration only. Selection
   is an argmax over the O(1) map for the pending owner, bounded by the target cap — not an all-targets
   sort.
3. **Reachability / "is this target valid to attack" is game-side**, not a module fragment.
4. **Compose authority-side only.** Aggro state does not replicate; composing on a client is a bug.
5. **Don't integrate threat decay per tick.** It is analytic from `_LastDecayTime`. Per-frame decay
   defeats the whole scale design.

---

## See also

- `Source/CLAUDE.md` — module topology + composition ritual.
- `CkTimer/Claude.md` — the canonical serial feature quartet (request-drain pattern).
- `CkCrowd` `CkCrowdAgent_Neighbors_Processor` — the parallel-processor discipline the Evaluate stage follows.
