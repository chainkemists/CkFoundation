# CkQueue

`CkQueue` is the server-authoritative spatial queue capability. It owns admission, stable FIFO tickets,
single-line rank assignment, revisioned slot targets, limits, formation retry, teardown reconciliation, and
semantic signals. It does not move entities and does not depend on `CkCrowd`.

## Boundary

- Add Queue to a spatial owner that already has Ck Transform with `utils_queue::Add`.
- `_Category` is an optional semantic `Queue.Category.*` tag. `Add` also stamps a valid category through
  CkEntityTag for presentation and scoped discovery. Category is not an ownership boundary: multi-instance games
  retain or explicitly register the exact Queues belonging to each store, fixture, or service scope.
- The queue owner is the sole source of truth. Consumers retain the `FCk_Handle_Queue` they requested and
  reconcile through `Get_Members`, `TryGet_MemberSnapshot`, `Get_Pressure`, `Get_State`, and `Get_Revision`.
  Do not add a second writable membership fragment to members.
- Consumers that need to display or verify authored geometry read `Get_SlotClaimPolicy`,
  `Get_SlotSpacingUu`, and the slot claim/settle/reacquire radii; they never inspect the Queue params
  fragment directly.
- Runtime membership is reconstructible and is not persisted. Rejoining the same semantic member is
  idempotent and may refresh its mover without minting a new ticket.
- All mutations are deferred requests. Request completions are planner-visible outcomes; signals carry the
  semantic reason and revision needed by GOAP or another AI consumer.

## Admission and ordering

- Tickets form one global, monotonic admission stream and ranks form one contiguous line rooted at the Queue
  owner's world transform. Tickets never change when ranks or targets do.
- Queue-wide hard limits reject before membership or movement is published. Soft limits only
  update pressure. A Crowd adapter rejected by one queue may immediately try another from its completion
  callback.
- `_SlotClaimPolicy` defaults to `ReserveOnFormation`, which preserves eager distinct-slot reservation behavior.
  Opt into `ClaimFirstAvailableOnReach` when every unclaimed member should move toward the next free
  slot. The first current-revision mover to report `Reached` claims that rank; the other contenders are immediately
  retargeted to the following rank with a newer revision. `AtFront`/`AtSlot` remains the authoritative claimed prefix.
- `_SlotClaimRadiusUu` (30 by default) is the semantic arrival boundary. Movement adapters may report the current
  revision reached at that boundary while continuing toward `_SlotSettleRadiusUu` (10 by default). A claimed mover
  displaced past `_SlotReacquireRadiusUu` (20 by default) may reacquire the same slot without another `SlotReached`
  event or assignment revision. All three radii must be finite, and validation requires
  `0 < settle <= reacquire <= claim`.
- `ReserveOnFormation` arrival is level-triggered: Queue reconciles each current mover transform against its unique
  reservation every tick and promotes it at the claim radius. Adapter `Reached` reports remain the responsive path,
  but a stale or missed report cannot leave a physically arrived rank-zero member permanently unserviceable.
- Distance-aware ReserveOnFormation assigns members without a valid reservation by mover distance, then ticket.
  Once a live mover owns a slot, its current lower rank is authoritative during compaction; physical reach controls
  readiness, not ordering, so later members that happen to be close across a folded line cannot jump ahead.
  `_ReserveAssignmentRefreshSeconds` controls rechecks for unreserved candidates (zero means every frame).
  `_ReserveAssignmentRefreshPhaseSpread` defaults to enabled and deterministically spreads independent queues across
  that interval; disable it only when synchronized refresh timing is specifically required.
- `Advance` succeeds only when the Queue's rank-zero member is authoritatively `AtFront`.

## Formation

- `OrthogonalSnake` is the default: exact configured spacing on a non-revisiting lattice with forward/left/right
  search only, so turns are 90 degrees and immediate reversal is impossible. `Linear` is the first alternative.
- The pure builder is atomic and bounded by `_MaxFormationSearchNodes`; no partial placement array is published.
- The runtime validator projects every slot to navigation, rejects excessive projection shift, blocked nav rays,
  Pawn-channel capsule overlap, and any post-projection slot overlap.
- Any materially changed reflow invalidates old assignment revisions before the movement adapter can consume
  them. Stale movement outcomes are successful no-ops.
- Reserve reflow keeps the previous reservation map only as private compaction input while Queue is
  `WaitingForFormation`; adapters and movement outcomes cannot consume it in that state. Formation validates live
  movers and publishes one complete replacement mapping, so no partial incumbent prefix becomes externally runnable.
- A navigation-generation revalidation preserves an arrived reservation whose rank and projected target
  are unchanged, including its `AtFront`/`AtSlot` state and assignment revision. En-route reservations still receive
  a fresh revision so their Crowd adapter replans a possibly invalid corridor, but the replacement `MoveTo` retains
  momentum. Only materially changed assignments publish member reflow events; nav rebuilds must not create an
  arrival/reflow feedback loop.
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

## QueueCoordinator

`CkQueue/Coordinator` is the generic controller for several independent Queues that provide one scoped service.
It does not create, own, destroy, advance, or move those Queues.

- A gameplay driver composes one QueueCoordinator and explicitly registers the Queue handles in that driver's
  scope. The coordinator never performs a registry-global category scan.
- An optional required `Queue.Category.*` tag rejects a mismatched registration, but the explicit roster remains
  the ownership contract.
- `LeastMembersThenDistance` is the default selection policy. `NearestThenLeastMembers` is the alternative.
  Both break final ties by stable registration ordinal.
- Select requests drain serially before Queue requests. Choices earlier in the same drain contribute projected
  admissions, so a same-frame burst does not route every member from one stale pressure snapshot.
- The result carries the selected Queue plus the remaining eligible Queues in deterministic fallback order.
  The caller submits the normal Queue/Crowd join and may resubmit with rejected Queues excluded.
- Existing membership is sticky and returned idempotently. Score changes never move an admitted member between
  Queues; leaving and selecting again is an explicit gameplay decision.
- Reconciliation prunes invalid registered handles. Coordinator destruction cancels its own pending requests and
  never mutates the registered Queues.

## CkCrowd adapter

`CkCrowd/Public/CkCrowd/Queue` is an optional adapter owned by `CkCrowd`; dependency direction is
`CkCrowd -> CkQueue` only.

- `Request_JoinQueue` uses the CrowdAgent as both Queue member and mover.
- Dispatch issues one nonzero-correlated Crowd `MoveTo` for the current Queue assignment revision.
- Crowd reports the Queue claim at the configured claim radius but keeps its owned movement episode until the tighter
  settle radius. Once claimed, the adapter retains station keeping: displacement past the reacquire radius starts a
  fresh correlated move to the same target, without re-reporting the already-claimed assignment revision.
- The adapter verifies assignment revision, correlation, active goal, and movement state. If another Crowd
  consumer replaces or stops the episode, the adapter reacquires its still-current assignment.
- Before reporting an outcome, the adapter asks Queue whether the handle is still authoritative and request-accepting.
  A queue that is invalidating or has lost authority terminates only the adapter-owned movement episode and clears the
  stale routing state; it never calls `Request_ReportMovementOutcome` on a queue that must reject requests.
- A retained Reserve snapshot is non-driveable while Queue is not `Ready`. Dispatch, outcome observation, and facing
  all gate on Ready; an unresolved formation stops only the adapter-owned episode and clears its issued/report markers
  so a later successful formation can dispatch and report the reservation again.
- Suppression, leave, invalidation, mover mismatch, and teardown stop only an episode whose correlation belongs
  to the adapter. Unrelated movement is never blanket-cancelled.
- Outcome polling reports reached/failed only when both Queue revision and Crowd correlation still match.
- Once Crowd reaches the slot and becomes idle, the adapter maintains the assignment rotation after Crowd's normal
  facing pass. Non-front slots face the Queue owner; the coincident front slot uses the owner rotation.

## Diagnostics

- `Get_DebugSnapshots` returns detached, value-only queue/category/reservation data for one ECS world. It
  never returns handles, registry references, fragment references, or UObjects.
- `ck.Queue.DebugDraw 1` enables the PIE world overlay: Queue-owner arrows, reservation points and facing, formation
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
  Crowd movement, owner-transform reflow, coordinated independent Queues, layout switching, impossible-nav recovery, limits, destruction,
  and the four GOAP-facing signal streams. `Ck_GymQueue_AddAgents <count>` extends the live queue up to its normal
  hard limit of 30; `Ck_GymQueue_Overfill` remains the explicit rejection test.

## Non-goals

Purchases, service policy, GOAP fact names, speech, inventory, and persistence identity remain consumer-owned.
Never write Queue or member transforms directly, add game collision channels, or introduce an arbitrary per-frame
Blueprint/AngelScript layout callback.
