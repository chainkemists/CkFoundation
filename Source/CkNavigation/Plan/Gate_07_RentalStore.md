# Gate 7 — Rental Store Scenario

> **Status:** ⏳ Pending
> **Day target:** D8 (buffer)
> **Parallelizable:** No
> **Depends on:** Gates 0–6 (all complete)

## Goal

Final integration gym shaped like the actual game: a small video-game rental store
interior with shelves, a counter, customers wandering, employees moving stock, the player
walking around. Everything ties together and "feels right" — or we identify the last
remaining tuning passes and apply them.

This is also the buffer day. If Gates 0–6 ran clean, this is a polish + showcase day. If
anything slipped, this is where it gets folded in.

## Acceptance criteria

1. ✅ A rental-store-shape gym (`Crowd Rental Store`) is walkable. Layout: 1 entrance, 4 browse aisles with shelf-front anchors, 1 counter with a queue lane, 1 employee zone behind counter.
2. ✅ 8 customer NPCs in a wandering loop (browse aisle 1 → idle 2s → browse aisle 2 → ...). They yield to the player. They queue at the counter. They despawn on counter completion (acted as if a transaction completed).
3. ✅ 2 employee NPCs moving stock around behind counter (predefined waypoint loop).
4. ✅ Player walks through, NPCs flow around. Player reaches counter, the queueing customer ahead steps forward, transaction triggers, customer despawns, next customer in queue advances.
5. ✅ At any time, a Tab key opens the menu HUD; ck.CrowdDebugger 1 opens the debugger; both work alongside the gym.
6. ✅ All Gate 0–6 AutoStations still pass (regression-clean).
7. ✅ Profiling: full scene with all NPCs + player active runs at target fps with crowd-related ms < 4.0.
8. ✅ Final pass on each module's `Claude.md`: every gate's contribution is reflected; the docs are usable as an onboarding reference for someone joining the project.

## Sub-tasks

### Sub-task 7A — Rental store gym layout

**Files:** `Plugins/CkTests/Script/CkCrowd/CkCrowdGym_Rental_*.as`

Layout (top-down, dimensions in cm):

```
┌─ 5000 ─┐ (entrance bottom-center)
│   |    │
│ Aisle1 │  ← Aisle1 = west wall, 4 shelves: Browse_W1..W4
│   |    │
│ Aisle2 │  ← Aisle2 = east wall, 4 shelves: Browse_E1..E4
│   |    │
│        │
│ Counter │  ← Counter in middle
│   ┃    │
│ Employee│
└────────┘
```

Tags:
- `Gym.Crowd.Rental.Entrance`
- `Gym.Crowd.Rental.Browse.W1`..`W4`, `E1`..`E4` (shelf positions)
- `Gym.Crowd.Rental.Counter` (anchor for queue lane head)
- `Gym.Crowd.Rental.QueueSlot.1`..`Gym.Crowd.Rental.QueueSlot.4` (queueing positions)
- `Gym.Crowd.Rental.EmployeeZone.A`, `Gym.Crowd.Rental.EmployeeZone.B`

### Sub-task 7B — Customer behavior loop

Each customer NPC has a small state machine (could be hand-written ECS state machine,
or use existing CkStateMachine module if light enough):

```
Browsing(aisle) → Idle(2s) → Browsing(other aisle) → Idle(2s) → … (4–6 cycles)
                                                                ↓
                                                           Queue → Counter → Despawn
```

Cycle count is randomized per customer (4–6) so the scenario doesn't tick in lockstep.

Customers spawn at the entrance every `_CustomerSpawnIntervalSeconds` (default 3.0s). Cap
total live customers at 8 — over the cap, defer spawn until a customer despawns.

### Sub-task 7C — Employee behavior loop

Two employees behind the counter:
- Employee A: moves between EmployeeZone.A and Counter on a 4s cycle, does a "stocking" idle.
- Employee B: similar but offset by 2s (so they don't move in lockstep).

The employees demonstrate "NPCs moving in tight space behind counter without pile-ups" — the
lower-density mirror of the customer loop.

### Sub-task 7D — Queue logic (gameplay layer, not crowd layer)

Queueing is a gameplay primitive, **not** a steering primitive. The crowd steering layer
just moves agents to a target. The queue logic:

1. Customer arrives at counter zone, requests assignment from a `QueueManager` entity.
2. QueueManager assigns a slot (`QueueSlot.N`); customer's MoveTo target = that slot.
3. When slot 1 is reached, customer is "served" after `_CounterServiceSeconds` (1.5s default).
4. On serve completion: customer despawns, all customers in slots 2..N shift forward by one slot.

The QueueManager is implemented as a simple ECS fragment + processor in the gym .as file.
Not part of CkCrowd — but it's the kind of feature that depends on having a CrowdAgent that
just-works for moving entities to slot positions.

### Sub-task 7E — Player walking-through showcase

The user (game lead) plays the gym for 5 minutes. Notes any moments that feel off:
- Customer never yields to me at aisle approach
- Customer pile-up at queue
- Employee gets stuck in counter geometry
- NPC arrival oscillates / overshoots
- etc.

Each note → either a tuning pass (adjust `_SeparationWeight`, etc.) or a bug fix in the
appropriate module. **No new architecture work in this gate** — only fixes to existing.

### Sub-task 7F — Documentation pass

For each of the three module `Claude.md` files:
- Verify every public API mentioned matches what shipped
- Verify the patterns described are what the code actually does
- Add a "Limitations / known issues" section listing what's *not* in this version (no ORCA, no flow-field, no off-mesh, no agent-following, no ScriptMixin from AS, etc.)
- Add a "Future work" section listing the planned post-ship improvements

The Claude.md files become the reference any future agent / developer uses to understand
the modules. They're as important as the code itself.

## Gym spec — manual

The Rental Store gym IS the manual gym. It's the showcase. No alternate manual variant.

## Gym spec — AutoStation

`UCk_AutoTest_Crowd_Rental_Smoke`:
- Run the rental store scenario for 30 seconds
- Customers spawn, browse, queue, get served, despawn — the full loop
- Assert: no NPC fails (zero `Failed` transitions)
- Assert: every customer who spawned has either despawned or is in the queue
- Assert: no NPC stalls > 1.5s
- `FinishSuccess()`

This is a smoke test, not an exhaustive correctness check. The fine-grained assertions are
already in Gates 0–6 AutoStations.

## Debugger additions

None planned. Gate 7 should not add debugger features — those are Gate 6's contribution.
If Gate 6 was rushed, this is where leftover debugger polish lands (sparkline visual fix,
panel resizing, etc.).

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| Queue logic is more complex than 1 day allows | Medium | The queue logic IS the most realistic deferable thing. If we're tight, ship a "first-come-first-served immediate-counter" version (no queue line), and add real queue logic post-ship. |
| Performance acceptable at Gate 6's stress test (150 agents) but feels different at Gate 7 (8 customers + 2 employees + spatial complexity) | Low | Lower agent count; if anything, this is easier than Gate 6. |
| Player walking through reveals a behavioral issue not caught in earlier gates (e.g., customer sometimes gets stuck behind a shelf) | Medium-High | This IS the value of Gate 7. Fixes go into existing modules; if a fix changes a defaulted tunable, document the change in CkCrowd/Claude.md and the corresponding gate file. |
| Gates 0–6 slipped enough that there's no buffer for Gate 7 | Medium | If Gate 7 has < 1 day, drop the queue logic + employee loop. Ship just the customer browse loop + player walking. |

## Done criteria checklist

- [ ] Rental store gym walkable, looks like a video game rental store interior.
- [ ] 8 customers + 2 employees + player + queue + counter all working.
- [ ] AutoTest_Rental_Smoke passes.
- [ ] All Gate 0–6 AutoStations still pass.
- [ ] User played gym 5 minutes; identified issues fixed.
- [ ] Final pass on all 3 module `Claude.md` files.
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] PLAN.md scope-decisions section reviewed: any decision changed during the build is updated here.
- [ ] (Optional, post-ship): consider deleting `Plan/` folder + this PLAN.md per "Post-ship cleanup" in the index.
