# CkQueue

`CkQueue` is the server-authoritative spatial queue capability. It owns admission, stable FIFO tickets,
multi-origin assignment, revisioned slot targets, limits, formation retry, teardown reconciliation, and
semantic signals. It does not move entities and does not depend on `CkCrowd`.

## Boundary

- Add Queue to a spatial owner that already has Ck Transform with `utils_queue::Add`.
- `_Category` is an optional semantic `Queue.Category.*` tag. `Add` also stamps a valid category through
  CkEntityTag, so gameplay and debuggers can group physical queues by service without a game-specific census.
- The queue owner is the sole source of truth. Consumers retain the `FCk_Handle_Queue` they requested and
  reconcile through `Get_Members`, `TryGet_MemberSnapshot`, `Get_Pressure`, `Get_State`, and `Get_Revision`.
  Do not add a second writable membership fragment to members.
- Runtime membership is reconstructible and is not persisted. Rejoining the same semantic member is
  idempotent and may refresh its mover without minting a new ticket.
- All mutations are deferred requests. Request completions are planner-visible outcomes; signals carry the
  semantic reason and revision needed by GOAP or another AI consumer.

## Admission and ordering

- Tickets form one global, monotonic admission stream. Multiple origins each expose an independent rank-zero
  front; service at one origin does not serialize the others.
- Origin assignment minimizes `load / weight`, breaks ties by origin index, and is recomputed deterministically
  after topology or membership changes. Tickets never change when origins/ranks do.
- Queue-wide and per-origin hard limits reject before membership or movement is published. Soft limits only
  update pressure. A Crowd adapter rejected by one queue may immediately try another from its completion
  callback.
- `_SlotClaimPolicy` defaults to `ReserveOnFormation`, which preserves eager reservation behavior. Opt into
  `ClaimFirstAvailableOnReach` when one mover per origin should be offered the next unclaimed rank while later
  members remain pending; `AtFront`/`AtSlot` is then the authoritative claimed prefix.
- `AdvanceOrigin` succeeds only when that origin's rank-zero member has reported `AtFront`.

## Formation

- `OrthogonalSnake` is the default: exact configured spacing on a non-revisiting lattice with forward/left/right
  search only, so turns are 90 degrees and immediate reversal is impossible. `Linear` is the first alternative.
- The pure builder is atomic and bounded by `_MaxFormationSearchNodes`; no partial placement array is published.
- The runtime validator projects every slot to navigation, rejects excessive projection shift, blocked nav rays,
  Pawn-channel capsule overlap, and any post-projection same- or cross-origin overlap.
- Any reflow invalidates old assignment revisions before the movement adapter can consume them. Stale movement
  outcomes are successful no-ops.
- A `MovementFailed` outcome relinquishes its target/rank/revision, keeps its ticket, moves the member behind
  viable members, and waits for a navigation generation change. It still counts toward pressure/limits but has
  no slot; viable survivors reflow without a synchronous runtime navigation query.
- A semantic member whose previously valid optional mover is destroyed enters `WaitingForMover`: its mover and
  assignment are cleared and navigation changes do not rearm it. Rejoin that same member with a valid new mover
  to return it to pending formation. Initial no-mover joins remain supported for synthetic/external reporters.

## Navigation recovery

- A failed formation performs the configured bounded retry episodes, then enters
  `WaitingForNavigationChange` and emits `NavigationRetryExhausted`.
- `UCk_Queue_NavigationRevisionSubsystem_UE` lazily binds to the NavSystem. First successful binding advances its
  revision, so a NavSystem that appears after world-subsystem initialization cannot strand a waiting queue.
- A navigation-generation change emits `NavigationChanged`, resets the retry episode, and retries the complete
  formation. Search-budget exhaustion also waits for an explicit topology/configuration change instead of
  spinning.

## CkCrowd adapter

`CkCrowd/Public/CkCrowd/Queue` is an optional adapter owned by `CkCrowd`; dependency direction is
`CkCrowd -> CkQueue` only.

- `Request_JoinQueue` uses the CrowdAgent as both Queue member and mover.
- Dispatch issues one nonzero-correlated Crowd `MoveTo` for the current Queue assignment revision.
- The adapter verifies assignment revision, correlation, active goal, and movement state. If another Crowd
  consumer replaces or stops the episode, the adapter reacquires its still-current assignment.
- Suppression, leave, invalidation, mover mismatch, and teardown stop only an episode whose correlation belongs
  to the adapter. Unrelated movement is never blanket-cancelled.
- Outcome polling reports reached/failed only when both Queue revision and Crowd correlation still match.
- Once Crowd reaches the slot and becomes idle, the adapter maintains the assignment rotation after Crowd's normal
  facing pass. Non-front slots face their assigned origin; the coincident front slot uses the authored origin rotation.

## Diagnostics

- `Get_DebugSnapshots` returns detached, value-only queue/category/origin/reservation data for one ECS world. It
  never returns handles, registry references, fragment references, or UObjects.
- `ck.Queue.DebugDraw 1` enables the PIE world overlay: origin arrows, reservation points and facing, formation
  links, member-to-slot links, rank/ticket/state labels, and formation/retry state. It is off by default.
- `Get_IsDebugDrawEnabled` / `Set_DebugDrawEnabled` let a scoped development tool preserve and restore the prior
  overlay state instead of blindly owning the global CVar.
- CkGameplayDebugger consumes the same snapshot in the Crowd debugger. CkQueue never depends on debugger UI.

## Events and teardown

The public signals are member-state changed, pressure changed, formation-state changed, and invalidated. Owner
destruction publishes member invalidation and Queue invalidation before pending request completions are cancelled,
so callbacks cannot re-enter a live partial queue. Destroyed members are removed and survivors reflow; a destroyed
optional mover does not destroy the semantic member.

## Verification homes

- Pure layout tests: `CkTests/Private/UnitTests/CkQueue/Test_Queue_Layout.cpp` (`Ck.Queue.Layout`).
- Runtime AS tests: `CkTests/Script/CkQueue/CkAutoTest_Queue_*.as`.
- Manual visual gym: `CkTests/Script/CkQueue/CkQueueGym_*` and the `Queue` registry entry. The gym exercises live
  Crowd movement, origin reflow, weighted origins, layout switching, impossible-nav recovery, limits, destruction,
  and the four GOAP-facing signal streams. `Ck_GymQueue_AddAgents <count>` extends the live queue up to its normal
  hard limit of 30; `Ck_GymQueue_Overfill` remains the explicit rejection test.

## Non-goals

Purchases, service policy, GOAP fact names, speech, inventory, and persistence identity remain consumer-owned.
Never write Queue or member transforms directly, add game collision channels, or introduce an arbitrary per-frame
Blueprint/AngelScript layout callback.
