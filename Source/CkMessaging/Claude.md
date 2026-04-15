# CkMessaging

**Purpose:** Entity-to-entity message passing. Messages are entities with a payload fragment sent from one entity to another. Processors route and consume messages in the same tick they're sent (or queued for the next).

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Systems that need decoupled cross-entity communication without direct processor dependencies.

---

## Key API

- `UCk_Utils_Messaging_UE` — send message entity, query inbox, drain inbox.
- Messages are typed by payload fragment.

---

## Pattern

Message entity lifecycle: sender creates message entity → processors route it to recipient's inbox → recipient processor drains inbox and processes.

---

## Anti-patterns

1. Don't use messaging for high-frequency per-frame data — signals are faster for that.
2. Don't hold message entity handles beyond the frame they're drained.

---

## See also

- `CkEcs/Claude.md` — signal system for event-driven (vs. message-driven) communication.
