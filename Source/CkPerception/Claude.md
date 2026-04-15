# CkPerception

**Purpose:** AI perception — hearing/sight queries using UE's AI Perception system, wrapped so results feed into ECS. Processors consume perception data as fragment state rather than actor callbacks.

**Depends on:** `CkCore`, `CkLog`, `CkThirdParty`.
**Used by:** AI agents, stealth systems.

---

## Key API

- `UCk_Utils_HearingPerception_UE` — hearing range queries, perceived sound source location.
- Additional sight/team utilities (see folder for full list).

---

## Pattern

Perception data arrives from UE's perception system and is written into ECS fragments each tick by a bridge processor. Downstream processors read those fragments for decision-making.

---

## Anti-patterns

Don't subscribe to AI perception delegates inside ECS processors — use the bridge processor pattern.

---

## See also

- `CkAi/Claude.md` — EQS integration.
- `CkTargeting/Claude.md` — target selection after perception.
