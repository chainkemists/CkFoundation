# Gate 3 — Separation

> **Status:** ⏳ Pending
> **Day target:** D3 (afternoon) → D4
> **Parallelizable:** Yes — Sub-task 3A and 3B are independent processors; Sub-task 3C depends on 3A.
> **Depends on:** Gate 0 ✅, Gate 1 ✅, Gate 2 ✅

## Goal

Multiple crowd agents avoid each other via **separation forces** sourced from neighbor
queries against `CkSpatialQuery`. After this gate, 5–10 agents can converge on overlapping
goals and they push each other apart smoothly instead of clipping through.

This is the "feels alive" gate. Without it, a crowd is just N independent walkers that
ghost through each other.

## Acceptance criteria

1. ✅ Each crowd agent has a `Probe` child entity with a Cylinder shape (radius = `_Radius + _SeparationLookahead`, default 200 cm) tagged for crowd-agent-only filtering.
2. ✅ A neighbor-cache fragment populates per frame from probe overlaps. List of `(NeighborHandle, RelativeOffset, RelativeVelocity)`.
3. ✅ Separation force is computed: for each neighbor inside `_SeparationRadius`, contribute `force = (selfPos - nbrPos).normalized() * (1.0 - distance / _SeparationRadius)^2 * _SeparationWeight`.
4. ✅ The steering processor sums path-follow + separation, normalizes against `_MaxSpeed`, writes to desired velocity.
5. ✅ When two agents head toward each other on a corridor, they pass without clipping (verified visually + by min-separation logging in AutoStation).
6. ✅ When 5 agents converge on the same goal, they form a cluster and don't pile into the same point.
7. ✅ Debugger Agent Detail "Neighbors" section matches [mockup §1](Debugger_Mockup/01_main.html#L120): list of ID + sep-distance + force vector.
8. ✅ Debugger Agent Detail "Steering Forces" section now shows non-zero `Separation` row when neighbors present.
9. ✅ AutoStation `UCk_AutoTest_Crowd_Separation_HeadOnPass` runs: 2 agents on collision course, assert min-separation never drops below `_Radius * 1.5`.
10. ✅ AutoStation `UCk_AutoTest_Crowd_Separation_Convergence` runs: 5 agents request move to overlapping points within 100cm, assert no two end positions are within `_Radius * 2`.

## Sub-tasks (parallelizable where noted)

### Sub-task 3A — Probe + neighbor query (sequential)

**Files:** New fragments + a setup processor.

```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/
    CkCrowdAgent_Neighbors_Fragment.{h,cpp}
        FFragment_CrowdAgent_NeighborCache    # Per-frame cache of (handle, relPos, relVel, distance)
        FTag_CrowdAgent_HasProbe              # Marks setup completion
    CkCrowdAgent_Neighbors_Processor.{h,cpp}
        FProcessor_CrowdAgent_ProbeSetup      # On NeedsSetup → spawn child Cylinder + Probe
        FProcessor_CrowdAgent_NeighborSync    # Per-frame: read probe overlaps, populate neighbor cache
```

Setup creates a child entity with:
- `UCk_Utils_Cylinder_UE::Add(Child, FCk_Fragment_Cylinder_Params{Radius=_Radius+_SeparationLookahead, Height=_Height})`
- `UCk_Utils_Probe_UE::Add(Child, ProbeParams)` — filter on `Crowd.Agent` tag, context-policy `Any`

NeighborSync reads `FFragment_Probe_Current._CurrentOverlaps`, transforms each overlap into a `FCk_Neighbor` entry (excluding self), stores sorted by distance ascending in `_NeighborCache`. Trim to `_MaxNeighborsForSteering` (default 6) — this is the key perf knob; we don't need to consider 20 neighbors.

### Sub-task 3B — Separation force solver (parallel with 3A — needs 3A's fragments declared but not implemented)

**File:** `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Separation_Processor.{h,cpp}`

```cpp
class CKCROWD_API FProcessor_CrowdAgent_Separation : public ck_exp::TProcessor<
    FProcessor_CrowdAgent_Separation,
    FCk_Handle_CrowdAgent,
    ck::TReadOnly<FFragment_CrowdAgent_Params>,
    ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
    ck::TReadWrite<FFragment_CrowdAgent_SeparationForce>,
    CK_IGNORE_PENDING_KILL>
{
public:
    using Group = FGroup_Physics;
    using RunAfter = TDepList<FProcessor_CrowdAgent_NeighborSync>;
    using RunBefore = TDepList<FProcessor_CrowdAgent_Steering>;
    // ...
};
```

The separation force is stored in its own fragment so the steering processor can read it
read-only. Steering combines path-follow + separation (Gate 4 will add pierce + player-yield
to this combination).

Force formula (start simple, tune in Gate 6):

```cpp
FVector Force = FVector::ZeroVector;
for (const auto& Nbr : NeighborCache.Get_Neighbors())
{
    const float D = Nbr.Get_Distance();
    if (D >= Params.Get_SeparationRadius()) continue;
    const FVector Push = -Nbr.Get_RelativeOffset() / FMath::Max(D, 0.01f); // normalized away
    const float Falloff = FMath::Pow(1.0f - D / Params.Get_SeparationRadius(), 2.0f);
    Force += Push * Falloff;
}
SeparationForce._Force = Force * Params.Get_SeparationWeight();
```

### Sub-task 3C — Steering integration (depends on 3A complete)

Update `FProcessor_CrowdAgent_Steering` to add the separation-force read dep and combine forces:

```cpp
const FVector Combined = PathFollowDir * DesiredSpeed + SeparationForce.Get_Force();
const FVector Clamped = Combined.GetClampedToMaxSize(Params.Get_MaxSpeed());
DesiredVelocity._Velocity = Clamped;
```

## Tunables added to `FCk_Fragment_CrowdAgent_ParamsData`

```cpp
// Gate 3 additions:
float _SeparationRadius = 100.0f;          // cm — agents start pushing each other at this distance
float _SeparationLookahead = 100.0f;       // cm — probe radius extends past separation radius for predictive nudges
float _SeparationWeight = 2.0f;            // multiplier on force magnitude
int32 _MaxNeighborsForSteering = 6;        // trim sort cap
uint32 _CollisionFlags = 0xFFFFFFFF;       // Gate 4 will use these for pierce; declare here for ABI stability
uint32 _IgnoreFlags = 0;
```

## Gym spec — manual

`Crowd Separation` gym:

- 4 alcoves around a central open area, tagged `Gym.Crowd.Separation.NorthSpawn`, `SouthSpawn`, `EastSpawn`, `WestSpawn`
- Press **1/2/3/4** → spawn an agent from the corresponding spawn, target = the opposite spawn (forces head-on collisions)
- Press **5** → 5 agents simultaneously target a single point in the center (cluster test)
- Live display: per-pair min-separation history (last 3 seconds)

## Gym spec — AutoStation

`UCk_AutoTest_Crowd_Separation_HeadOnPass`:
- Spawn 2 agents 1500 cm apart, facing each other
- Request move to each other's start position (head-on)
- Sample min-distance every 50ms
- Assert min-distance never drops below `_Radius * 1.5` (= 63 cm at default 42 cm radius)
- Assert both reach goal within 8s

`UCk_AutoTest_Crowd_Separation_Convergence`:
- Spawn 5 agents at known positions
- Request all 5 to move to the same target
- After 6s, assert all 5 are within 200cm of target (cluster formed)
- Assert no two final positions are within `_Radius * 2` (= 84 cm)
- `FinishSuccess()`

## Debugger additions (per [mockup §1](Debugger_Mockup/01_main.html))

| Panel | Gate 3 contribution |
|---|---|
| Agent List | New column: `Nbrs` count. |
| Agent Detail Steering Forces | `Separation` row populated with vector + weight. |
| Agent Detail Neighbors | New section. List entries: nbr-handle (cyan), sep distance in cm, force vector contribution. Sorted by distance. Heading shows total count + radius. |
| Stats | New: `Avg neighbors` field. `Neighbor query` ms breakdown. |
| Event Log | (No new categories in this gate — neighbor traffic is too chatty for the log.) |

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| Probe per agent doubles entity count → CkSpatialQuery perf cliff at 100 agents | Low | 100 probes is well within Jolt's comfort zone per study. Stress-tested in Gate 6. |
| Separation force fights path-follow at the goal — agent oscillates ("zooms past, comes back, zooms past...") | Medium | Force formula falls off quadratically; at the goal there's no path-follow. The combined-vector clamping prevents oscillation as long as separation < max speed. **Mitigation**: when distance to final goal < `_ArrivalRadius * 2`, scale separation force by 0.5 — verify in tuning gate. |
| Two agents in tight aisle perfectly mirror → both get equal force, neither moves (stalemate) | Medium | Add a tiny deterministic tie-breaker: jitter separation force by `±0.05 * sin(handle.id)` per agent. **Mitigation**: piercing in Gate 4 also helps. |
| Probe filter doesn't actually reach Jolt with a `Crowd.Agent` tag → all entities show up as neighbors (props, walls) | Medium | AutoStation asserts `Get_NeighborCache().Num()` for an agent in an empty room is 0, then for an agent next to a static prop is also 0 (props aren't tagged). |

## Done criteria checklist

- [ ] Probe + neighbor cache work end-to-end; AutoTest_HeadOnPass passes.
- [ ] AutoTest_Convergence passes.
- [ ] Debugger Neighbors section matches mockup §1 visual + content.
- [ ] No perf regression at 10 agents (steering + neighbor-sync ms total < 1.0).
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] CkCrowd/Claude.md updated with neighbor-query + separation pattern.
