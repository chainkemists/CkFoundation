# CkInteraction

**Purpose:** Interaction system — entities that can be interacted with (interactables) and entities that can initiate interaction (interactors). Handles detection, request, begin/end lifecycle, and signals.

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Pickups, doors, NPCs, any gameplay element with a player-facing interaction.

---

## Key API

- `UCk_Utils_Interaction_UE` — add interactable/interactor feature, begin/end interaction, query state.
- Signals: `OnInteractionStarted`, `OnInteractionEnded`.

---

## Pattern

Interactable entities have an Interaction fragment; interactor entities hold a reference to the active interactable. Processors manage distance/range checks and fire signals on state transitions.

---

## Implementation notes

- **InteractTarget owns interaction-entity lifetime.** It creates the Interaction entity in
  `FCk_Try_InteractTarget_StartInteraction`, so it is also the one that destroys it in
  `OnInteractionFinished`.
- **InteractTarget calls into InteractSource** (`Request_StartInteraction` on the resolved source
  handle) after creating the interaction. That dependency direction is not ideal but is accepted,
  and it matches the pattern already used by the Resolver.

- **The resolver's best-target cache is invalidated by BOTH of its inputs.**
  `FProcessor_InteractionResolver_Persistent` runs only when `FTag_InteractionResolver_ResolveDirty`
  is present, and that tag is stamped by the intent handlers (`StartIntent`/`StopIntent`) AND by any
  handler that really mutates `_AvailableTargets` (add / remove / remove-all-by-channel; never on
  their no-op early-returns). Target churn therefore re-resolves *while a button is held*, which is
  what lets a consumer retarget mid-hold instead of latching whatever was picked at press time.
  Before this, `Request_AddInteractTarget` had no observable effect until some unrelated intent edge
  happened along.
- **A live interaction outranks proximity.** `DoResolveTargets_Internal` pins targets this resolver
  is already interacting with ahead of the distance sort and the `MaxConcurrentInteractions`
  truncation. Without it, a nearer same-channel target appearing mid-hold evicts the live one, the
  consumer cancels what it no longer sees, and a cancelled `Timed` interaction is destroyed along
  with its progress — a hold silently resetting to zero. The pin asks the interaction record
  directly rather than reading `Get_CanInteractWith`'s `AlreadyExists`, because that result is
  decided against the INTERACT-SOURCE CAST of the source and is unreachable for any consumer that
  composes no `InteractSource`.
  The pin's contract is therefore: **the interaction's stored source must equal the resolver's own
  entity**. A consumer that starts interactions with some other handle as the source gets no pin.
  Both in-tree consumers satisfy it (the resolver is composed on the same entity the bridge passes
  as the source).

## Anti-patterns

Don't poll for nearby interactables every frame in a Processor — use the spatial query (`CkSpatialQuery`) results as the input to the interaction system.

---

## See also

- `CkSpatialQuery/Claude.md` — range detection.
- `CkAttribute/Claude.md` — interaction cost/cooldown via attributes.
