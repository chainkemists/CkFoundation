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

## Anti-patterns

Don't poll for nearby interactables every frame in a Processor — use the spatial query (`CkSpatialQuery`) results as the input to the interaction system.

---

## See also

- `CkSpatialQuery/Claude.md` — range detection.
- `CkAttribute/Claude.md` — interaction cost/cooldown via attributes.
