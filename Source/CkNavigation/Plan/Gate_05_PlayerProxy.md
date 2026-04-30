# Gate 5 — Player Proxy + Soft-Push

> **Status:** ⏳ Pending
> **Day target:** D5 (afternoon)
> **Parallelizable:** No (single sub-task)
> **Depends on:** Gates 0–4 (especially Gate 3 separation)

## Goal

The Player remains an `ACharacter` (per the locked scope decision); NPCs are pure entities.
The bridge is a **Player Proxy Entity** — a single entity per local player that mirrors the
player's transform + velocity into the steering layer each frame. NPCs see the proxy as
just another neighbor and avoid it; the proxy never moves itself.

After this gate: when the player walks through a crowd, NPCs separate around them
naturally. When the player heads toward an NPC at a shelf, the NPC steps aside (soft-push).
When two NPCs and the player converge on the same doorway, the player wins (yield rule).

## Acceptance criteria

1. ✅ A `FCk_Handle_PlayerProxy` typesafe handle exists. `UCk_Utils_PlayerProxy_UE::Add(LocalPlayer)` creates the proxy entity attached to the player's `APlayerController` (one per local player; client-side mirror, server-side mirror separately).
2. ✅ Proxy entity has a CrowdAgent fragment with `_Flags = PLAYER_PROXY`, marked `FTag_CrowdAgent_IsObstacleOnly` (steering processor never updates desired velocity for these).
3. ✅ A per-frame `FProcessor_PlayerProxy_Mirror` reads the player's `APlayerController` pawn transform + velocity and writes them to the proxy entity's SceneNode + `FFragment_Velocity_Current`.
4. ✅ Server-side proxy is authoritative for steering: NPCs (server-only) see the server-side proxy as a neighbor.
5. ✅ Default agent params include `_IgnoreFlags` value that does **not** include `PLAYER_PROXY` — i.e., agents do consider the proxy a neighbor.
6. ✅ When player approaches an idle NPC at a shelf within `_PlayerProxySoftPushRadius` (default 120cm), the NPC's separation gets a 2× boost on the player-proxy contribution. NPC visibly steps aside.
7. ✅ When player + 2 NPCs converge on a doorway, the player passes; NPCs yield. (Verified by relative-arrival-order in AutoStation.)
8. ✅ Debugger Agent List shows the proxy as `#PROXY Player.Proxy` with a distinct `Live` info-blue badge — matches [mockup §1 last row](Debugger_Mockup/01_main.html).
9. ✅ Debugger Event Log shows `PROXY` category entries when soft-push activates — matches [mockup §1 row at 8.0s](Debugger_Mockup/01_main.html).
10. ✅ AutoStation `UCk_AutoTest_Crowd_PlayerProxy` runs: simulate player movement (no physical APlayerController; AutoStation creates a fake-input proxy), assert NPCs separate from it.

## File inventory

```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/PlayerProxy/
    CkPlayerProxy_Fragment.{h,cpp}
        FCk_Handle_PlayerProxy            # typesafe handle
        FFragment_PlayerProxy_Source      # Holds TWeakObjectPtr<APlayerController> source ref
        FTag_PlayerProxy                  # Marker on the proxy entity
        FTag_CrowdAgent_IsObstacleOnly    # Modifier: steering skips desired-velocity for this agent
    CkPlayerProxy_Fragment_Data.{h,cpp}
    CkPlayerProxy_Processor.{h,cpp}
        FProcessor_PlayerProxy_Mirror     # Per-frame: copy AController->GetPawn()->transform/velocity → SceneNode + FFragment_Velocity_Current
    CkPlayerProxy_Utils.{h,cpp}
        UCk_Utils_PlayerProxy_UE::Add(APlayerController*)
        UCk_Utils_PlayerProxy_UE::Remove(InProxy)
        UCk_Utils_PlayerProxy_UE::Get_Proxy_FromController(APlayerController*) → FCk_Handle_PlayerProxy
```

The proxy is a CrowdAgent that *does not have steering applied to it*. Its SceneNode position is driven by the mirror processor reading the actor pawn. NPCs include it in their neighbor list (filter tag `Crowd.Agent` matches both real agents and the proxy, since the proxy is also tagged `Crowd.Agent`).

### Soft-push amplification

Inside `FProcessor_CrowdAgent_Separation`, when a neighbor's flags include `PLAYER_PROXY` AND `_PlayerYieldEnabled` is set on this agent, multiply the per-neighbor force by `_PlayerYieldMultiplier` (default 2.0). Other agents' neighbor traffic is unchanged.

This is the *only* gameplay-mode bias in steering. Don't add more on top — additional yield rules (employee yields to player, customer yields to employee, etc.) are out of scope.

### Player Proxy spawn lifecycle

```cpp
// In ACk_GameMode or PlayerController BeginPlay (project-side, called once per local player):
UCk_Utils_PlayerProxy_UE::Add(this); // 'this' is the PlayerController

// On PlayerController EndPlay:
UCk_Utils_PlayerProxy_UE::Remove(_Proxy);
```

The proxy entity should be owned by the PlayerController's entity-bridge (existing CkActor pattern). When the PC dies / is destroyed, the proxy goes too.

## Tunables

```cpp
// Crowd agent params additions:
float _PlayerProxySoftPushRadius = 120.0f;   // cm — distance below which player-yield amp kicks in
float _PlayerYieldMultiplier = 2.0f;
bool _PlayerYieldEnabled = true;
```

Default tags / flags:

```cpp
// In FCk_Fragment_CrowdAgent_ParamsData defaults:
const uint32 _Flags = AGENT;            // bit 0
const uint32 _IgnoreFlags = 0;          // by default, no agents are ignored

// Proxy params:
const uint32 _Flags = AGENT | PLAYER_PROXY;  // bits 0 + 2
```

`AGENT = 1 << 0`, `EMPLOYEE = 1 << 1`, `PLAYER_PROXY = 1 << 2`, plus reserves for future. Bit assignments documented in CkCrowd/Claude.md.

## Gym spec — manual

`Crowd Player Proxy` gym (the rental-store-shape preview):

- A small "store interior": entrance, two browse aisles (each with shelf-front anchors), one counter
- 4 stationary NPCs at shelf-front positions (browse loop — they path between adjacent shelf positions slowly)
- 1 stationary NPC behind counter
- Player spawns at entrance
- Player walks through; NPCs visibly step aside and resettle
- Press **A** → toggle NPCs aware of player vs not (sanity baseline)
- Live display: count of NPCs currently soft-pushing this frame

## Gym spec — AutoStation

`UCk_AutoTest_Crowd_PlayerProxy_SoftPush`:
- Spawn 1 NPC at known position
- Spawn a "fake player" (just a SceneNode + actor with the proxy attached) at far position
- Animate fake player toward the NPC over 2 seconds
- Sample NPC position every 100ms
- Assert NPC's lateral displacement (perpendicular to player's approach axis) increases by at least 30cm before player reaches NPC's start position
- Assert min-distance between NPC and player never drops below `_Radius * 1.5`

`UCk_AutoTest_Crowd_PlayerProxy_DoorwayYield`:
- Setup: 100cm doorway. Fake player + 2 NPCs simultaneously approaching from the same side, target = beyond doorway.
- Assert player reaches the goal first (relative arrival order).
- Assert all 3 reach the goal eventually.

## Debugger additions (per [mockup §1](Debugger_Mockup/01_main.html))

| Panel | Gate 5 contribution |
|---|---|
| Agent List | New row for `#PROXY Player.Proxy` with `Live` info-blue badge. Always last in list (sort key: proxies after real agents). Click to select shows proxy detail (Identity, Transform — the rest is `n/a`). |
| Agent Detail Steering Forces | `Player yield` row populated with `active`/`inactive` + last activation time. |
| Event Log | New `PROXY` category entries (orange). One row per soft-push activation: `Player yielded by #00XX at 87 cm`. |

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| Proxy mirror lags player by one frame → NPCs avoid where player was, not where they are | Low | Acceptable. 1-frame lag at 60fps = ~16ms = ~4cm at running speed. Within `_Radius * 1.5` margin. |
| Multiple local players (split-screen) — what's the proxy hierarchy? | Low | Multi-local-player not a current rental store concern. Single-proxy-per-PlayerController works. Document edge case for future. |
| Server doesn't have a player-controlled pawn (dedicated server) | Low | The proxy on a dedicated server reads the *server-side* APlayerController's pawn (the server holds the authoritative pawn). Same code path. |
| Soft-push multiplier breaks neighbor force balance — agents oscillate when player walks past | Medium | Tested in `SoftPush` AutoStation: lateral-displacement assertion. If it oscillates, lower `_PlayerYieldMultiplier` from 2.0 → 1.5 in tuning gate. |
| The proxy spawned client-side conflicts with server-side authoritative behavior | Medium | The proxy is **server-only** by default for crowd steering decisions. Clients have a local proxy too, but only for visual debug overlays — not consulted by client-side steering (which mostly just smooths replicated transforms). Document this. |

## Done criteria checklist

- [ ] Proxy spawns automatically per local player; despawns with PC.
- [ ] Mirror processor keeps proxy SceneNode + velocity in sync each frame.
- [ ] Soft-push activates within `_PlayerProxySoftPushRadius`.
- [ ] DoorwayYield AutoStation: player wins.
- [ ] SoftPush AutoStation: NPC laterally displaces by ≥ 30cm.
- [ ] Debugger shows proxy in agent list with distinct badge.
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] CkCrowd/Claude.md updated with PlayerProxy section + flags table.
