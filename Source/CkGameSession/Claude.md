# CkGameSession

**Purpose:** Game session management — session state machine (lobby, loading, in-game, results), player slot management, and session-level settings. Built on top of UE's `AGameMode`/`AGameState`.

**Depends on:** `CkCore`, `CkEcs`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** `CkUI` (UI visibility driven by session state).

---

## Key API

- No `_Utils.h`. Accessed via the session subsystem and `ACk_GameState` (from `CkCore/Engine`).

---

## Pattern

Session state changes fire ECS signals; UI and gameplay systems listen to those signals rather than polling `AGameMode`.

---

## Implementation notes

`UCk_GameSession_Subsystem_UE` owns a private `ck::FEcsWorld` (`_InternalEcsWorld`) purely to host the
signal entity. This used to be implicit — `FCk_Registry`'s default ctor auto-allocated an entt registry —
but post-migration the world must be allocated explicitly in `Initialize`. `Deinitialize` clears
`_SignalHandle` *before* resetting the world, so outstanding subscribers resolve cleanly via the slot table.

---

## Anti-patterns

Don't poll `AGameMode` state directly in processors — listen to session state change signals.

---

## See also

- `CkCore/Engine/README.md` — `ACk_GameState` base.
- `CkUI/Claude.md` — UI driven by session state.
