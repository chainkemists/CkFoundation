# design_ShortcutBudgetUnits — the three prices of one chord, and the slope none of them charge

Read-only design pass, 2026-09-06. Resolves the two confirmed review findings against the route
shortcut shipped in CkFoundation `947bccfa3` (`feat(groundnav): raycast shortcut pass after the corner
offset, priced against the stretch it replaces`) and its pins in CkTests `589212e1`. Shape and
conventions follow `research/design_PathShortcut.md`; rulings it inherits are `[NN-D104]` in
`PROGRESS.md:5532` and the S10-5 GATE 1 reversal at `PROGRESS.md:5587`.

Paths are relative to `Plugins/CkFoundation/Source/` and `Plugins/CkTests/Source/CkTests/Private/`
unless stated. **No wall-clock number appears anywhere; every cost is a count.**

---

## §0 A caveat that must be read first — the tree moved under this pass

`Plugins/CkFoundation` HEAD is `947bccfa3`, but **`CkGroundNav_PathPostProcess.cpp` and
`CkGroundNav_SearchTypes.h` are dirty in the working tree right now**, as are
`Test_GroundNav_PathShortcut.cpp` (932 → **1117** lines) and `Test_GroundNav_QueryFixtures.h`. The test
file gained a sixth pin (`Shortcut_PinnedWaypointSurvivesAClearChord`) and
`Shortcut_RaycastCountIsStableAndRecorded` was renamed `Shortcut_WaypointCountsAreStableAndRecorded`,
between two reads inside this session (file mtime 10:19 today). **A sibling session is editing exactly
these files.**

Every `file:line` below is the **working tree as of this pass**, which is what a reader opens. Where it
differs from `947bccfa3` the drift is small and named:

| File | Committed | Working tree | Delta |
|---|---|---|---|
| `CkGroundNav/Search/CkGroundNav_PathPostProcess.cpp` | 774 lines | 776 | +2 (comment at `:36-39`, `PassIsEnabled` guard `>0` → `>=2` at `:458-460`) |
| `CkGroundNav/Search/CkGroundNav_SearchTypes.h` | 227 | 230 | +3 (comment on `_ShortcutSpanCap`, now `:122`) |
| `UnitTests/CkGroundNav/Test_GroundNav_PathShortcut.cpp` | 932 | 1117 | +185 (new `clearlane` pin; priced-band second assertion re-guarded) |
| `CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.cpp`, `Query/CkGroundNav_QueryTypes.h`, `Search/CkGroundNav_PlatePortalGraph.cpp`, `Search/CkGroundNav_PathPostProcess.h` | — | clean | none |

Nothing below depends on the drift, but **any unit implementing this must re-derive line numbers, and
must not stage those files without the owning session's agreement.**

---

## §1 The three pricing rules, verified

All three price the same object — the chord `Get_Shortcut` is about to accept — and all three answer a
different number. Restated against the code, with the brief's claims corrected where the code
contradicts them.

### 1.1 BUDGET — `Get_ReplacedStretchCostUu`, `CkGroundNav_PathPostProcess.cpp:327-342`

```
CostUu += max(m[i], m[i+1]) * FVector::Dist2D(W[i], W[i+1])          // :337-338
```

- `m[]` comes from `Get_PlateMultipliers` (`:275-297`), one entry per waypoint, read through
  `Get_AreaMultiplier` (`:292-293`) — the field's baked `_CostMultiplier` merged upward with the
  query's table (`CkGroundNav_PlatePortalGraph.cpp:211-238`, baked at `:231`, table max at `:234-235`).
- **XY only** (`FVector::Dist2D`), **no slope term**, **endpoint-sampled** (never per cell).
- Consumed at `:494-496` inside `Get_Shortcut`, once per candidate.

**Brief's claim: correct as stated.**

### 1.2 ADMISSION — `Get_IsChordWalkableWithinBudget`, `CkGroundNav_PathPostProcess.cpp:352-376`

```
BudgetUu = InReplacedCostUu * (1.0 + kShortcutBudgetSlack)    // :361, kShortcutBudgetSlack = 1e-5 at :40
Query._PlateCostMultipliers = InCost._PlateCostMultipliers    // :368
Query._UseBakedPlateCost    = true                            // :372
Query._MaxCost              = float(BudgetUu)                 // :373
return Get_SurfaceRaycast(InField, Query).Get_IsClear();      // :375
```

The ray's own accumulation, `CkGroundNav_Query_SurfaceWalk.cpp`:

- `FTraversal::_Delta` is an **`FVector2D`** (`:194`) and `_TotalLengthUu = _Delta.Size()` (`:229`) — the
  **XY** length of the chord.
- Per cell: `LengthInCellUu = (Get_NextT() - _T) * _TotalLengthUu`, `SegmentCost = _CostMultiplier *
  LengthInCellUu` (`:697-698`); the cap fires at `:700-719` and returns `Blocked` with
  `_StoppedOnCost = true`.
- `_CostMultiplier` is refreshed only where the plate changes (`DoRefresh_CostMultiplier`, `:212-219`;
  called at `:333` on a plate/tile crossing and once at `:507` for the start plate) via
  `Get_PlateCostMultiplier` (`:122-160`): with `_UseBakedPlateCost` the plate's own baked price is the
  floor (`:135-141`) and a named plate takes `FMath::Max(baked, table)` (`:159`) — **the same merge rule
  as `Get_AreaMultiplier`**.
- Single-plate early-out (`Get_PlateEarlyOut`, `:68-113`, taken at `:673-686`) charges
  `StartCostMultiplier * SegmentLengthUu` — one plate's price for the whole chord, which is per-cell
  pricing degenerated correctly.

**Brief's claim: correct as stated.** One addition the brief omits: per plate the two rules *agree*
(both are `max(baked, table)`); the divergence is purely **where they sample** — endpoints versus cells.

### 1.3 REPORTING — `Get_FilledWaypoints`, `CkGroundNav_PathPostProcess.cpp:617-675`

```
AreaMultiplier = max(Get_AreaMultiplier(plate[i-1]), Get_AreaMultiplier(plate[i]))   // :659-661
LegCostUu      = Get_LegCost(Shared, From, To, AreaMultiplier, kNoClearanceFactor)   // :663
_DistanceFromStart += FVector::Dist2D(From, To)                                      // :663-665
_CostFromStart     += LegCostUu                                                      // :668-669
```

and `Get_LegCost` (`CkGroundNav_PlatePortalGraph.cpp:240-274`):

```
BaseUu      = Delta.Size();                                        // :256   THREE-dimensional
SlopeFactor = 1 + _SlopePenaltyK * (|Delta.Z| / Delta.Size2D());    // :258-263
return BaseUu * InAreaMultiplier * SlopeFactor * InClearanceFactor; // :269-273
```

`Shared._SlopePenaltyK` is threaded from the query by `Make_SharedData` (`:211-225`, `:219`).

**Correction to the brief.** The brief describes the reporting rule as
`Get_LegCost(..., max(endpoint multipliers), kNoClearanceFactor)` — true, but it does not say what that
implies: **the fill already carries BOTH the 3D length and the slope penalty.** The fill is therefore
the *only* one of the three sites that speaks the search's own leg model. The budget and the ray are the
two that do not. That reverses the natural reading of Finding B: the defect is not "the fill is wrong
about slope", it is "the budget and the ray are the two sites that were never taught it".

### 1.4 What the SEARCH actually prices (the fourth rule, for reference)

`FCk_GroundNav_PlatePortalGraph::Cost` (`CkGroundNav_PlatePortalGraph.cpp:392-397`) calls the same
`Get_LegCost`, but with `Get_AreaMultiplier(..., DoGet_ArrivalPlate(InFrom))` (`:396`) — **the plate the
leg crosses**, not the max of two endpoint plates — plus a real `InClearanceFactor` and a link traversal
term (`:376-390`). So the fill is already a (pre-existing, out-of-scope) approximation of the search;
"agree with the search's cost model" below means **agree with `Get_LegCost` and with the plate table**,
which is as close as anything reachable inside `Get_PathPlan` can get.

### 1.5 The one-line statement of the defect

> The chord's cost function and the budget's summand cost function are **different functions**, and the
> plan's published cost is computed with a **third**.

Everything in §2 follows from that. In particular: a budget that is a sum of `f(segment)` is only
sub-additive under chord replacement — the property the header's idempotence lemma silently assumes —
when `f` is the same function used to price the chord. `Get_ReplacedStretchCostUu`'s `max`-of-endpoints
rule is sub-additive **only when every multiplier is equal**, in which case it degenerates to arc length
and the triangle inequality does the work. **That is precisely, and only, why the shipped pins are
green**: `Make_Cost` (`UnitTests/CkGroundNav/Test_GroundNav_PathShortcut.cpp:126-135`) sets
`_CornerOffsetK` and `_ShortcutSpanCap` and **nothing else**, so every shortcut pin runs at
`_SlopePenaltyK = 0` (`CkGroundNav_SearchTypes.h:106`) and — except for the priced-band pin, which
prices a plate the route *avoids* — at a uniform multiplier of 1.0.

---

## §2 One worked example per finding

### 2.1 FINDING A — the rise, and why the priced-band scene structurally cannot show it

**First, the negative result, because it is the more useful half.** The priced-band scene
(`Test_GroundNav_PathShortcut.cpp:649-757`) prices plate D — the middle band probed at
`kBandPricedProbe = (550, 600)` (`:681`) — at `kPricedOutMultiplier = 100.0f` (`:687`), and the pin
asserts at `:813-815` that **the search routes AROUND it**. A plate the route avoids carries no
waypoint, so `Get_PlateMultipliers` returns 1.0 for every point of the detour and
`Get_ReplacedStretchCostUu` degenerates to arc length. **The scene can only ever exercise the
*undercharge* direction of the mismatch (a chord cutting *into* dear ground), never the *overcharge*
direction (a chord *ending on* dear ground).** The rise is invisible to it by construction.

**The rise, worked on the four-pillar slab plus one priced plate.** The four-pillar route
(`Test_GroundNav_QueryFixtures.h:700-813`, `kFourPillarAgentRadiusUu = 34.0f` at `:715`,
`kFourPillarWestPost/EastPost` at `:723-724`) is the one shipped fixture with a real false corner: the
pass drops exactly one waypoint, pinned as `funnelled 5 / shortcut 5 / plan off 5 / plan on 4`
(`Test_GroundNav_PathShortcut.cpp:990-993`). Take the two legs it replaces and price the plate the
chord's **far endpoint** stands on at `M = 4` through `_PlateCostMultipliers`
(`CkGroundNav_SearchTypes.h:126`) — the markup shape the campaign's own demo gyms ship.

Legs, in the fixture's magnitudes (2D, uu): `a = 700`, `b = 300`, chord `c = 950`; the chord's last
`p = 60` uu lie inside the priced plate.

| Site | Rule | Arithmetic | Number |
|---|---|---|---|
| Budget `:337-338` | `Σ max(mᵢ,mᵢ₊₁)·Dist2D` | `max(1,1)·700 + max(1,4)·300` | **1900** |
| Cap handed to the ray `:361, :373` | `×(1+1e-5)` | `1900 × 1.00001` | 1900.019 |
| Admission `SurfaceWalk.cpp:697-698` | per-cell `mult × XY len` | `(950−60)·1 + 60·4` | **1130** → `1130 ≤ 1900.019` → **ACCEPTED** |
| Reporting, plan OFF `:659-663` | fill over the two legs | `max(1,1)·700 + max(1,4)·300` | **1900** |
| Reporting, plan ON `:659-663` | fill over the chord | `max(1,4) × 950 × 1.0` | **3800** |

`_CostFromStart` **1900 → 3800 (+100 %)** while `_LengthUu` falls `1000 → 950 (−5 %)`. The pin at
`Test_GroundNav_PathShortcut.cpp:841-842` (`OnCost <= OffCost + kEpsilon`) goes **red**; the second
assertion is skipped because its guard at `:848` sees the length fall — so exactly one assertion fires,
and it fails. The chord was admitted for 1130 and billed at 3800: **the number checked and the number
published are different numbers.**

**The idempotence break, same fixture.** Add a fourth point `D` 500 uu beyond the chord's far end, also
on the priced plate, and suppose the chord `Pᵢ→D` costs 4200 by the ray (mostly dear ground).

- Run 1 from `Pᵢ`: budget over `Pᵢ..D` = `1900 + max(4,4)·500 = 3900`. `4200 > 3900` → refused.
  Falls back to `Pᵢ→Pᵢ₊₂` (1130 ≤ 1900) → keeps `Pᵢ₊₂`.
- Run 2 over the kept list `(Pᵢ, Pᵢ₊₂, D)`: budget = `max(1,4)·950 + max(4,4)·500 = 3800 + 2000 = 5800`.
  Same chord, same 4200 → **4200 ≤ 5800 → accepted.** Run 2 drops a point run 1 kept.

`Shortcut_IsIdempotent` (`:866-…`) would go red. It is green today because its fixture is unpriced. The
header's lemma at `CkGroundNav_PathPostProcess.h:182-188` —

> *"offered at a budget no larger, since the stretch it would now replace is made of chords the first run
> accepted, each priced at no more than the stretch it replaced"*

— is **false as written**: a chord is priced at no more than the stretch it replaced *under the ray's
metric*, and the budget is a sum *under the endpoint-max metric*. The two clauses of the sentence are
about different functions. Confirmed.

### 2.2 FINDING B — the slope, worked on `Bake_RampVsLevelScene`

A sloped fixture **does** exist: `Bake_RampVsLevelScene` (`Test_GroundNav_QueryFixtures.h:590-606`), a
flat floor to `X = 900` and a `kRampAngleDegrees = 20.0` panel east of it (`:72`), built expressly so
"a slope penalty has something to trade against length" (`:498-500`). Its search-side pin
(`UnitTests/CkGroundNav/Test_GroundNav_PathSearch.cpp:1543-1623`) shows the trade working: at
`kSlopePenaltyK = 2.0f` (`:640`) the leg factor is exactly `1 + k·rise/run` (`:1537-1541`), and at
`kSteepSlopePenaltyK = 8.0f` (`:643`) **the search gives the slope up for the level way**
(`:1605-1609`).

`tan 20° = 0.363970`. Take a replaced stretch of two legs on the flat half totalling 1200 uu in XY, and
a chord of 1000 uu in XY whose last 400 uu (in XY) climb the panel along +X:

- endpoint rise `= 400 × 0.363970 = 145.588`; `rise/run = 0.145588`; `3D = √(1000² + 145.588²) = 1010.556`

| Site | Rule | Number |
|---|---|---|
| Budget `:337-338` | `Σ Dist2D` (flat, mult 1) | **1200** |
| Admission `SurfaceWalk.cpp:229, 697-698` | XY accumulation, mult 1 | **1000** → `1000 ≤ 1200.012` → **ACCEPTED** |
| Reporting `:663` at `k = 2.0` | `1010.556 × (1 + 2×0.145588)` | **1304.81** (+8.7 %) |
| Reporting `:663` at `k = 8.0` | `1010.556 × (1 + 8×0.145588)` | **2187.56** (+82.3 %) |

At `k = 8` the pass hands back **the very leg the search refused** (`Test_GroundNav_PathSearch.cpp:1605-1609`)
and reports it at 1.82× what the detour cost. Neither the budget nor the ray sees a single unit of that.

**What no existing fixture has.** `RampVsLevel` has slope but **no false corner near the ramp**: with the
penalty on, the route takes the level west gap and never comes near the panel, so no chord over the
slope is ever offered. The four-pillar slab has false corners but is **flat**. So B is real but currently
unreachable by any shipped pin.

**Minimal new fixture.** `Make_RampGapPillarScene()` = `Make_RampVsLevelScene()`
(`Test_GroundNav_QueryFixtures.h:565-588`) **plus one 150×150 box, 150 uu tall, standing in the east gap**
(`X ∈ [1050, 1300]`, entirely on the panel) — e.g. centred at `(1175, 750)`, baked through the same
`Bake_RampVsLevelScene` panel call (`:598-603`). Constraints checked: the panel's rise over its own run
is `600 × tan20° = 218.4 uu`, so a 150-tall pillar tops out at `368 uu < kMaxZ = 400` (`:37`) and needs no
params change; at 150 uu it exceeds the profile's 140 uu standing height
(`kFourPillarAgentHalfHeightUu`-style capsule, `Make_Params:85-88`) so it is a real obstacle. The pillar
forces the east-gap crossing to weave, putting a false corner **on the slope**, which is the one thing
`RampVsLevel` lacks. The exact centre needs one measured run to confirm a corner actually survives the
offset — see §5.

---

## §3 Options

Shared notation: `n` = waypoints handed to `Get_Shortcut`; `S` = pinned spans. The candidate loop
(`CkGroundNav_PathPostProcess.cpp:494-511`) runs from `FarthestIndex` (`:487-488`) down to `Index + 2`,
so a kept point costs at most `min(spanEnd − i, cap) − 1` casts and the whole pass costs at most
**`(n−1)(n−2)/2`** casts (the design's §2 quoted the looser `n(n−1)/2`). At the pinned `n = 5`
(`Test_GroundNav_PathShortcut.cpp:990`) that is **≤ 6**. Each cast is **O(1)** — one cell read — when
both ends share one plate that admits the body (`Get_PlateEarlyOut`, `SurfaceWalk.cpp:68-113`, taken at
`:673-686`); otherwise a DDA of ≈ `L/25` steps (`kCellSize = 25.0f`, `Test_GroundNav_QueryFixtures.h:31`).

---

### O1 — segment-level slope/3D factor on **both** the budget and the chord's cap

**Change.**

1. `CkGroundNav_PathPostProcess.cpp:327-342` — replace the body of `Get_ReplacedStretchCostUu` with the
   fill's own function:
   `CostUu += Get_LegCost(Shared, W[i], W[i+1], max(m[i], m[i+1]), kNoClearanceFactor)`, taking
   `Shared` from `Make_SharedData` (`:211-225`) — one extra parameter on the helper. This buys `Delta.Size()`
   (3D) and `1 + k·rise/run` for free, from the one function the plan already reports through.
2. `CkGroundNav_PathPostProcess.cpp:352-376` — the ray still accumulates XY, so divide rather than
   multiply: compute `ChordFactor = (FVector::Dist(From,To) / FVector::Dist2D(From,To)) × (1 + k·|ΔZ|/Dist2D)`
   from the chord's own endpoints and set `Query._MaxCost = float(BudgetUu / ChordFactor)` at `:373`.
   Guard `Dist2D > 0`. **No SurfaceWalk change.**
3. `Get_Shortcut` (`:447-519`) passes `InCost` through to the helper; no structural change.

**Raycast bound per pass:** unchanged, `(n−1)(n−2)/2` — pure arithmetic.

**Idempotence at the unbounded cap:** **still broken.** O1 changes the *unit* both sides speak but not
the *sampling*: the budget stays endpoint-max, the chord stays per-cell. The §2.1 counterexample survives
verbatim (it is flat, so O1's factors are all 1.0). O1 fixes B and leaves A untouched.

**Fill's reporting rule:** untouched. Budget and reporting now use the *same function* on the same
points — which is a real gain: `Get_ReplacedStretchCostUu` becomes, by construction, "what the plan paid
for this stretch".

**Pins that move:** none. Every shipped shortcut pin runs at `_SlopePenaltyK = 0` and on flat ground
(`Make_Cost:126-135`), so `Get_LegCost` degenerates to `mult × Dist2D` and `ChordFactor = 1`. Expect
**delta-zero** on all six.

**The one new pin:** `Shortcut_RespectsTheSlopePenalty` — the `RampGapPillarScene` of §2.2, planned twice
with `_SlopePenaltyK = kSteepSlopePenaltyK` (8.0), pass off and on; assert
`On._Waypoints.Last()._CostFromStart <= Off._Waypoints.Last()._CostFromStart + kEpsilon`, and assert the
same plan at `_SlopePenaltyK = 0` *does* drop the waypoint — so the pin proves the refusal came from the
penalty and not from the geometry. Red before O1, green after.

---

### O2 — price the replaced stretch with the **same ray**, cached per segment index

**Change.**

1. `CkGroundNav_PathPostProcess.cpp` — new private
   `Get_SegmentRayCostUu(InField, InCost, InAgent, InVerticalToleranceUu, From, To) -> TOptional<double>`:
   a `FCk_GroundNav_RaycastQuery` identical to `:363-373` but with `_MaxCost = 0.0f` (uncapped), returning
   `Result._AccumulatedCost` when `Get_IsClear()` (`QueryTypes.h:318`) and `unset` otherwise.
2. `Get_Shortcut` (`:470`) allocates `TArray<TOptional<double>> SegmentCosts` of size `n−1` beside
   `Multipliers`; each entry filled lazily on first use. **Each original segment is cast at most once per
   pass.**
3. `Get_ReplacedStretchCostUu` (`:327-342`) sums `SegmentCosts[i]` where set, and falls back to
   `max(m[i],m[i+1]) × Dist2D` where the segment's own ray is not clear — which happens: the funnel's
   apexes hug their walls at exactly one radius, the very effect that forced the `[NN-D104]` F-6 reversal
   (`PROGRESS.md:5587`).
4. Fold in O1's segment factor on top (the brief's own "plus the segment-level slope factor"): multiply
   each segment's ray cost by its own `(3D/2D)×(1+k·rise/run)`, and divide the chord's cap by the chord's.

**Raycast bound per pass:** `(n−1)(n−2)/2 + (n−1)`. At `n = 5`: **≤ 6 + 4 = 10**. The extra casts are the
cheapest ones in the pass — a funnel segment usually lies inside one plate and takes the O(1) early-out.

**Idempotence at the unbounded cap: HOLDS**, and for a reason that generalises. Acceptance is
`rayCost(chord) ≤ Σ budget(segment)`, and `budget` is additive over sub-stretches. Run 2's spans are
delimited by the same pinned points, and every run-2 segment is a chord run 1 accepted, so
`budget(run-2 segment) = rayCost(that chord) ≤ budget(the sub-stretch it replaced)`. Summing,
`run-2 budget(p..q) ≤ run-1 budget(p..q)`. The chord `p→q` has the same `rayCost` in both runs. Therefore
run 1's refusal (`rayCost > run-1 budget`) implies run 2's. Note this survives the fallback in (3),
because the argument only needs the *budget* to be additive and the *accepted* chords to have been priced
by the same function as the chord under test — both hold.

**Fill's reporting rule:** untouched — and that is O2's limit. Admission and budget now agree, but the
plan still publishes `Get_LegCost` with endpoint-max, so the §2.1 rise **survives**: the chord admitted
for 1130 against a per-cell budget of ≈ 1130-scale is still *billed* at 3800.
`Shortcut_NeverLowersThePlateCostPaid` can still go red on a fixture with a dear endpoint.

**Pins that move:** none expected. On the four-pillar slab every plate is 1.0 and `segRay = Dist2D`, so
the budget is unchanged. On the priced band the detour's segments all lie on 1.0 plates, so the budget is
unchanged and the chord is refused for the same reason. `Shortcut_WaypointCountsAreStableAndRecorded`'s
four counts (`:990-993`) hold. **Delta-zero expected on all six.**

**The one new pin:** `Shortcut_BudgetSeesGroundNoWaypointStandsOn` — a markup fixture where a **dear band
lies inside a replaced segment but under neither of its endpoints** (the mirror of the priced band).
Today the budget prices that segment at 1.0 and is too tight, so a legitimate chord is refused; under O2
the segment's own ray charges the band and the chord is admitted. Assert: with the pass on, the plan's
waypoint count is strictly lower than with it off, and `OnCost <= OffCost + kEpsilon`. Red before O2
(count equal), green after.

---

### O3 — per-cell slope weighting **inside** the surface raycast

**Change (touches Query/).**

1. `CkGroundNav_QueryTypes.h:268-292` — add `float _SlopePenaltyK = 0.0f;` to `FCk_GroundNav_RaycastQuery`,
   default zero so an unset query is byte-identical.
2. `CkGroundNav_Query_SurfaceWalk.cpp` — `FTraversal` already tracks `_SurfaceZUu` (`:189`) and refreshes
   it on every admitted step (`:317`), and `Get_StepAcross` already outputs the next surface's Z
   (`:294-298`). So the per-step rise is available at **zero extra cell reads**. Carry
   `_PreviousSurfaceZUu`, and at `:697-698` weight the cell segment by
   `(1 + k·|ΔZ|/LengthInCellUu) × √(1 + (ΔZ/LengthInCellUu)²)`, guarded for `LengthInCellUu → 0`. Set the
   field in `Get_SurfaceRaycast` at `:688-691` beside `_UseBakedPlateCost`.
3. Budget side: still needs O1's fix, or the two sides speak different units again.

**Raycast bound per pass:** unchanged, `(n−1)(n−2)/2`. One extra multiply and one Z compare per cell step;
**no extra cell reads**.

**Idempotence at the unbounded cap:** unchanged by O3 alone — still broken, for A's reason. O3 is
orthogonal to sampling; it only makes the ray's unit right.

**Blast radius (asked for explicitly).** Small, and enumerable. `FCk_GroundNav_RaycastQuery` has four
call sites outside the type: `CkGroundNav_PathPostProcess.cpp:363` (the shortcut — the only production
caller with a non-zero cap), `Facade/CkGroundNav_NavSurfaceAdapter.cpp:188-193` (no cap),
`Debug/CkGroundNav_DebugDraw.cpp:3167-3175` (`_MaxCost = 0.0f`, `:3173`), and the tests
(`Test_GroundNav_Query_Raycast.cpp:72-80`, `Test_GroundNav_Query_ThreadStress.cpp:145`,
`Test_GroundNav_Facade_Equivalence.cpp:234`, `Test_GroundNav_PathShortcut.cpp:782`). The cost-cap pin at
`Test_GroundNav_Query_Raycast.cpp:533-544` runs on flat ground, so `ΔZ = 0` and it is unmoved. **The real
cost is conceptual, not mechanical:** `_AccumulatedCost` stops being comparable to a distance for any
caller that reads it, and `Get_MoveAlongSurface` shares `FTraversal` (`:514-624`) without pricing — a
reviewer must confirm the new field never leaks into the walk.

**Fill's reporting rule:** untouched. Per-cell slope makes the ray *stricter* than the fill (which uses
one endpoint-to-endpoint slope), so O3 alone can make admission and reporting disagree in the *opposite*
direction from A — a chord refused for slope the fill would have averaged away. Not a correctness bug;
it is a second, quieter mismatch, and it is why O3 should not ship without O1's budget change.

**Pins that move:** none on flat fixtures — all six. On any future sloped fixture, `Get_SurfaceRaycast`'s
`_AccumulatedCost` for a capped query changes meaning; nothing pins it today.

**The one new pin:** `Raycast_CostChargesTheClimbPerCell` — in `Test_GroundNav_Query_Raycast.cpp`'s style,
on `Bake_RampVsLevelScene`, one uncapped ray straight up the panel and one across it at the same XY
length; assert the up-ray's `_AccumulatedCost` equals the across-ray's times
`√(1+tan²20°)·(1 + k·tan20°) = 1.06424 × 1.72794 = 1.83892` at `k = 2`, to within a cell of quantisation.
Falsifies the per-cell arithmetic directly, with no path pass in the way.

---

### O4 — price the chord under the fill's endpoint-max rule only (the previously rejected shape)

**Change.** `Get_IsChordWalkableWithinBudget` (`:352-376`) drops `_MaxCost`, `_PlateCostMultipliers` and
`_UseBakedPlateCost` (`:368-373`) — the ray becomes a pure walkability test — and the accept rule becomes
arithmetic: `Get_LegCost(Shared, W[i], W[j], max(m[i],m[j]), 1.0) ≤ Get_ReplacedStretchCostUu(i,j) × (1+slack)`.

**Raycast bound per pass:** `(n−1)(n−2)/2` — the ray is still needed for geometry, just uncapped.

**Idempotence at the unbounded cap: HOLDS**, trivially and for the same reason as O2: one function on
both sides, additive budget, run-2 budget ≤ run-1 budget.

**Why `[NN-D104]` rejected it, and whether that still holds.** The recorded reason
(`PROGRESS.md:5570-5573`) is that a waypoint-only patch "leaves crossed-but-unnamed plates at 1.0", so a
chord may cut through dear ground it is never charged for — which is exactly what the plate table exists
to prevent. **That reason still holds, and it is stronger than recorded, because the pin that names the
property would go GREEN while the property became false.** Run O4 against the shipped priced-band
fixture:

- `Test_GroundNav_PathShortcut.cpp:789-791` already proves `kBandStart (200,600) → kBandGoal (900,600)`
  is **walkable throughout** — length 700 uu, straight through plate D.
- With the unbounded cap and no pinned interior point, the **first candidate the loop tries** (`:494`,
  descending from `FarthestIndex`) is exactly waypoint 0 → waypoint last, i.e. that chord.
- O4's budget = the detour's own fill cost. Every detour waypoint stands on a 1.0 plate (the pin asserts
  the corridor avoids D at `:813-815`), so the budget is the detour's 2D arc length — around
  `223.6 + 500 + 223.6 = 947.2` uu for the taut way round the south divider.
- O4's chord cost = `max(1,1) × 700 × 1.0 = 700`. `700 ≤ 947.2` → **ACCEPTED**.
- Plan collapses to two waypoints straight across ground priced at 100×; `OnCost = 700`, `OffCost ≈ 947`;
  `:841-842` passes, and `:848` sees the length fall so the second assertion is skipped. **Pin green.**

O4 therefore converts `Shortcut_NeverLowersThePlateCostPaid` into a tautology while the body walks 700 uu
of the exact ground the search refused. **Rejected, and the fixture above is the proof to record.**

**Fill's reporting rule:** untouched, and by construction identical to the accept rule — which is O4's
only genuine merit: the number checked *is* the number published.

**Pins that move:** `Shortcut_NeverLowersThePlateCostPaid` stops meaning anything (stays green);
`Shortcut_WaypointCountsAreStableAndRecorded` (`:990-993`) is at risk — the four-pillar bare-pass count of
5 is a *geometric* refusal (a chord passing pillar 2 at ≈ 32 uu against a 34 uu radius,
`PROGRESS.md:5587`), so it should hold, but `kPlanWaypointsWithThePassOn = 4` must be re-measured.

---

### O5 — **the recommendation**: O2's ray budget **and** a fill-metric conjunct

Not in the brief's list; it is the smallest combination that closes both findings, and it is O2 plus four
lines.

**Change.**

1. Everything in O2 (per-segment ray budget, cached, with the fallback).
2. `Get_ReplacedStretchCostUu` (`:327-342`) additionally returns a **second** number: the fill's own price
   of the stretch, `Σ Get_LegCost(Shared, W[i], W[i+1], max(m[i],m[i+1]), kNoClearanceFactor)` — i.e. O1's
   change, kept as a separate value rather than replacing the first.
3. `Get_IsChordWalkableWithinBudget` (`:352-376`) takes both and accepts only when **both** hold:
   - `(i)` the ray's per-cell cost ≤ the ray budget — *never cross ground dearer than the corridor crossed*;
   - `(ii)` `Get_LegCost(Shared, From, To, max(m[i],m[j]), kNoClearanceFactor) ≤ fill budget` — *never
     publish a cost higher than the plan already carried*. Pure arithmetic; no second ray.

**Raycast bound per pass:** `(n−1)(n−2)/2 + (n−1)`; at `n = 5`, **≤ 10**. Identical to O2 — conjunct (ii)
costs no casts.

**Idempotence at the unbounded cap: HOLDS.** Each conjunct is individually "one function both sides, over
an additive budget", so each satisfies `run-2 budget ≤ run-1 budget` by the O2 argument, and each chord's
cost is run-independent. A run-1 refusal is a refusal under (i) or (ii); whichever it was, run 2 refuses
under the same conjunct. Conjunction of two idempotent accept rules is idempotent. The header lemma at
`CkGroundNav_PathPostProcess.h:182-188` becomes true, and should be rewritten to say *which* function it
is quantifying over — the current wording is the bug.

**Fill's reporting rule:** untouched, and now **enforced**: conjunct (ii) is literally "the fill's number
for the chord ≤ the fill's number for what it replaces", so `_CostFromStart` is monotone by construction
rather than by hope. That is the property `Shortcut_NeverLowersThePlateCostPaid` names and has never
actually tested.

**Pins that move: none.** On the flat, unpriced four-pillar scene both conjuncts reduce to the triangle
inequality (`chord 2D ≤ Σ segment 2D`) and acceptance is decided by geometry alone — exactly as today, so
`funnelled 5 / shortcut 5 / plan off 5 / plan on 4` (`:990-993`) holds. On the priced band, the detour's
segments are all 1.0 plates so both budgets equal today's, and conjunct (i) refuses the chord for today's
reason. `FourPillar_RouteHasNoFalseCorners` (`:279-…`), `Shortcut_KeepsEveryLinkEndpoint` (`:469-…`),
`Shortcut_PinnedWaypointSurvivesAClearChord` (`:571-…`) and `Shortcut_IsIdempotent` (`:866-…`) are all
untouched. **Delta-zero expected on all six** — which is the right shape for a contract fix.

**The one new pin:** `Shortcut_CostNeverRisesWhenAnEndpointStandsOnDearGround` — the §2.1 fixture: the
shared four-pillar slab, a `_PlateCostMultipliers` entry for the flat plate under the **east post**
(`kFourPillarEastPost`, `Test_GroundNav_QueryFixtures.h:724`), read off exactly as
`Get_PricedPlate` reads its own (`Test_GroundNav_PathShortcut.cpp:729-744`). Two assertions, both red
today and green under O5:
1. `On._Waypoints.Last()._CostFromStart <= Off._Waypoints.Last()._CostFromStart + kEpsilon`;
2. `Get_Shortcut(Get_Shortcut(x)) == Get_Shortcut(x)` **under that same priced table** — the idempotence
   pin's assertions (`:866-…`) re-run with a non-uniform multiplier, which is the case the shipped one
   never reaches.

One fixture falsifies both symptoms of Finding A. Pair it with O1's `Shortcut_RespectsTheSlopePenalty`
for B.

---

## §4 Recommendation

**Ship O5 — O2's per-segment ray budget plus the fill-metric conjunct — as one unit, with O1 folded into
the fill conjunct (it comes free, since `Get_LegCost` already carries the 3D length and the slope term).
Park O3.**

**Why.** The root cause is a single sentence (§1.5): three functions where there should be one per side of
one comparison. O5 is the only option that repairs all three relationships:

| | budget ↔ admission | admission ↔ reporting | speaks slope + 3D |
|---|---|---|---|
| today | ✗ | ✗ | reporting only |
| O1 | ✗ | ✗ | all three |
| O2 | ✓ | ✗ | budget + reporting |
| O3 | ✗ | ✗ | admission + reporting |
| O4 | ✓ | ✓ | budget + reporting | *(and admits dear ground unpriced — rejected)* |
| **O5** | **✓** | **✓** | **budget + reporting; admission stays honest in XY** |

**Smallest change that makes all three sites agree with the search's own cost model: O5.** Strictly, the
three sites end up agreeing with `Get_LegCost` + the plate table, which is the closest reachable target —
the fill's `max`-of-two-endpoints already differs from the search's arrival-plate rule
(`CkGroundNav_PlatePortalGraph.cpp:396`), and closing *that* would mean changing `Get_FilledWaypoints` and
moving `_CostFromStart` for every consumer (`Debug/CkGroundNav_DebugDraw.cpp:3799` and any crowd reader).
Out of scope, and no pin holds an absolute cost value today — `Test_GroundNav_PathPostProcess.cpp:563-574`
pins only monotonicity — so it stays available as a later phase.

**Strongest counterargument.** *O5 is unfalsifiable on every fixture that ships today.* Both conjuncts
reduce to the triangle inequality on flat, uniformly-priced ground, so the change is delta-zero on all six
pins by design — it costs `n−1` extra raycasts per completed query on the exact per-frame budget the B4
metric exists to protect, in exchange for a defect nothing in the suite can currently see. A reviewer is
entitled to say: leave it, and write the pins instead.

**The rebuttal, and why I still recommend shipping.** The defect is in the **contract**, not only in the
behaviour: `CkGroundNav_PathPostProcess.h:182-188` states an idempotence lemma that is false, and
`Shortcut_NeverLowersThePlateCostPaid` names a property the pass does not have. Both are things a future
consumer will build on. The campaign's own demo gyms ship markup and the search ships `_SlopePenaltyK`, so
the unreachable case is one gym scene away from being the common case. And the price is bounded and small:
`n−1 ≤ 11` extra casts, most of them the single-plate O(1) early-out, once per completed search in
`DoPublish_Success` — not per tick. **If the counterargument wins, the minimum acceptable outcome is the
two new pins landed red-then-skipped with the lemma at `:182-188` rewritten to state its own precondition
("holds where every replaced waypoint's plate multiplier is equal"), so nobody builds on a false one.**

---

## §5 What I could not verify

- **No measurement of anything.** No build, no test, no editor, no toolbox — read-only by instruction.
  Every number in §2 is arithmetic over fixture constants, not a reading off a run.
- **The four-pillar waypoint coordinates.** §2.1's `a = 700`, `b = 300`, `c = 950`, `p = 60` are plausible
  magnitudes for that slab, **not measured**. The pinned counts (`5 / 5 / 5 / 4`,
  `Test_GroundNav_PathShortcut.cpp:990-993`) and the one recorded coordinate `(-401.6, -243)`
  (`PROGRESS.md:5587`) are the only hard numbers I have about that route. The *mechanism* does not depend
  on the magnitudes — the condition is `max(mᵢ,mⱼ)·c > Σ max(mₖ,mₖ₊₁)·dₖ`, which is satisfiable for any
  bend once one endpoint multiplier exceeds the rest — but a unit must measure the real legs before
  pinning a number.
- **The priced-band plate decomposition.** I inferred five rectangles (west strip / east strip /
  south-middle / middle band / north-middle) from the box list at `:689-713` and from
  `Get_PricedPlate`'s probe. Only the middle band is *proved* to be its own plate, by the pin at
  `:774-777`. §2.1's negative result ("the scene structurally cannot show the rise") rests on the pin's own
  assertion at `:813-815` that the corridor avoids the priced plate, which is solid; the detour's
  arc-length figure of 947.2 uu in §3/O4 is derived from the divider corners and is **not measured**.
- **The `RampGapPillarScene` pillar placement.** That a 150-tall box at `(1175, 750)` produces a false
  corner *on the panel* that survives the corner offset is an inference from the four-pillar scene's
  behaviour, not an observation. One run with `Get_Shortcut` off tells you whether the funnel emits an
  interior waypoint there at all; if it does not, move the pillar or add a second.
- **Whether the ray's admission and the search's admission agree.** `design_PathShortcut.md:258-260`
  flagged this and did not prove it; I did not either. If `Get_IsAdmitted` and `Get_StepAcross` have
  drifted, conjunct (i) is checking a different notion of walkable from the one the corridor was built on,
  and no budget arithmetic repairs that.
- **The segment-ray fallback rate.** O2/O5 fall back to the endpoint-max price for any replaced segment
  whose own ray is not clear. That case is *known* to exist (the F-6 reversal at `PROGRESS.md:5587` is
  exactly a chord refused at 32 uu against a 34 uu radius), but I have no idea how often it fires on a
  real route. If it fires on most segments the ray budget degenerates to today's and O5 buys only its
  fill conjunct. **Instrument it before pinning** — a count of fallbacks per pass belongs in the
  `[SHORTCUT-BUDGET]` report line at `Test_GroundNav_PathShortcut.cpp:990-…`.
- **The live tree (§0).** A sibling session is editing `CkGroundNav_PathPostProcess.cpp`,
  `CkGroundNav_SearchTypes.h`, `Test_GroundNav_PathShortcut.cpp` and `Test_GroundNav_QueryFixtures.h`
  right now. Line numbers here are from a read at this pass's time and **will drift**; the sibling's
  in-flight `_ShortcutSpanCap >= 2` guard and new `clearlane` pin are already in them and are not in
  `947bccfa3`. Nothing here conflicts with those edits, but the unit that implements this must rebase on
  whatever that session commits before touching either file.
