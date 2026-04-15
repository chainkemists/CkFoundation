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

## Anti-patterns

Don't poll for nearby interactables every frame in a Processor — use the spatial query (`CkSpatialQuery`) results as the input to the interaction system.

---

## See also

- `CkSpatialQuery/Claude.md` — range detection.
- `CkAttribute/Claude.md` — interaction cost/cooldown via attributes.
