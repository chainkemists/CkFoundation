# Gym Authoring Cheatsheet — AngelScript Gotchas

Quick reference when writing CkCrowd test gyms in `Plugins/CkTests/Script/CkCrowd/`.

## File set per gym

A typical gym is 2–4 .as files. Match the [CkSceneNode reference](../../../../CkTests/Script/CkSceneNode/):

```
CkCrowdGym_<Variant>_GameMode.as          # Trivial — overrides PC/HUD class
CkCrowdGym_<Variant>_PlayerController.as  # Get_RequiredStations() + Request_StartGym()
CkCrowdGym_<Variant>_Agent.as             # Per-agent EntityScript (optional)
CkCrowdGym_<Variant>_Display.as           # Per-station live display EntityScript (optional)
```

Register the gym in [`Script/Common/CkTests_GymRegistry.as`](../../../../CkTests/Script/Common/CkTests_GymRegistry.as):

```as
CkGym_Cycler::RegisterProjectGym("Crowd <Variant>", ACkCrowdGym_<Variant>_GameMode);
```

Keep the registry alphabetical within the existing convention. Multiple Crowd gyms group as `Crowd Foundation`, `Crowd Locomotion`, `Crowd Separation`, etc.

## Class inheritance (must match)

```as
class ACkCrowdGym_<Variant>_GameMode : ACkTests_Gym_Base_GameMode
{
    default PlayerControllerClass = ACkCrowdGym_<Variant>_PlayerController;
}

class ACkCrowdGym_<Variant>_PlayerController : ACk_Gym_Base_PlayerController
{
    UFUNCTION(BlueprintOverride)
    TArray<FCkGym_Station_SpawnParams_Payload> Get_RequiredStations() override { ... }

    UFUNCTION(BlueprintOverride)
    void Request_StartGym() override { ... }
}
```

`ACkTests_Gym_Base_GameMode` calls `CkTests_Gyms::RegisterAll()` in BeginPlay automatically.

## Station spawn payload (typical pattern)

```as
auto Station = FCkGym_Station_SpawnParams_Payload();
Station.Tags.Add(n"Gym.Crowd.AgentSpawn");
Station.Title = FText::FromString("AGENT SPAWN");
Station.Description.Add(FText::FromString("Drop new agents at the marker."));
Station.Description.Add(FText::FromString("They path to the corridor exit."));
Stations.Add(Station);
```

In `Request_StartGym()`, look up anchor transforms with the same tag:

```as
const FTransform AnchorXform =
    CkGym_Common::Get_StationAnchorTransform(n"Gym.Crowd.AgentSpawn", ECk_GymStation_Anchor::FootprintCenter);
```

## AS gotchas (the ones that bite)

| Gotcha | Wrong | Right |
|---|---|---|
| **No lambdas.** | `BindToOnTick([&]() { ... })` | `BindToOnTick(this, n"OnTickHandler")` — handler is a `UFUNCTION()` method on the class |
| **No `static_cast`.** | `static_cast<ACustomerNPC>(Pawn)` | `Cast<ACustomerNPC>(Pawn)` — and check `ck::IsValid(Result)` after |
| **`f"{...}"` interpolation, not `$`/`%s`.** | `FString::Printf(TEXT("%s"), Name)` | `FString Msg = f"agent {AgentId} reached goal";` |
| **`float` is 64-bit in AS.** | passing `float` to a C++ UFUNCTION declared as `double` | both sides line up; just be aware truncation can hide bugs |
| **RPCs are reliable-by-default in AS.** | adding `unreliable` keyword | omit; reliability is implicit |
| **Component attach at class scope.** | manual SetupAttachment in BeginPlay | `UPROPERTY(DefaultComponent, Attach = Parent) UStaticMeshComponent Mesh;` |
| **No memory management.** | `delete obj` | entities clean themselves up; ComponentHandles auto-release on entity death |
| **By-value struct param gotcha.** | mutating a struct passed by value and expecting changes to flow back | pass `UPARAM(ref)` to mutate, otherwise you're working on a copy |
| **Pure virtual override.** | `void Foo() override` | `UFUNCTION(BlueprintOverride) void Foo() override` — the macro is required |

## CkGym_Common helpers (the ones you'll actually use)

| Function | Purpose |
|---|---|
| `Request_SpawnNewStation(Payload)` | Returns a pending handle; await `Promise_OnConstructed()` before placing children. |
| `Get_StationAnchorTransform(tag, anchor)` | Anchor in world space. **Use `FootprintCenter` for nav surfaces, `PanelCenter` for shelf-height pickups.** |
| `Update_StationDisplay(entity, title, desc, instructions)` | Pushes new display text via fragment; cheap. |
| `Draw_DebugSphere(entity, offset, color, radius)` | One-shot PMG sphere. For continuous overlays use `UCk_Utils_Pmg_DebugShape_UE` directly. |

## AutoStation pattern (for assertions)

```as
class UCk_AutoTest_Crowd_<Name> : UCk_AutoTest_Base
{
    UFUNCTION(BlueprintOverride)
    void DoConstruct() override
    {
        Set_Status(ECk_AutoTest_Status::Running);
        // …spawn agents, kick off scenario…
    }

    UFUNCTION()
    void OnAgentReachedGoal(FCk_Handle_CrowdAgent InAgent)
    {
        Assert_True(ck::IsValid(InAgent), f"agent valid");
        // …
        if (AllAgentsArrived) FinishSuccess();
    }

    UFUNCTION()
    void OnTimeout()
    {
        FinishFailure(f"timeout — {ArrivedCount}/{ExpectedCount} agents arrived");
    }
}
```

`ACk_AutoTestRunner` polls the result fragment each tick. Don't try to bind delegates in lambdas; use `n"FunctionName"` and a `UFUNCTION()` handler.

## Anchor cheat-sheet

For nav-surface tests use **`FootprintCenter`**. For shelf-mounted props use **`PanelCenter`**. Spawn cones around an alcove use **`AgentSpawn{Front,Back,Left,Right}`**. Banner / overhead labels use **`PanelTopFront`**.

## Tag taxonomy for Crowd gyms

```
Gym.Crowd.<Variant>.<Slot>

Gym.Crowd.Foundation.AgentSpawn
Gym.Crowd.Foundation.Counter
Gym.Crowd.Locomotion.Source
Gym.Crowd.Locomotion.Target
Gym.Crowd.Separation.Funnel
Gym.Crowd.Doorway.Entrance
Gym.Crowd.Doorway.Exit
Gym.Crowd.PlayerProxy.Center
Gym.Crowd.Stress.Spawn.Cluster_<n>
Gym.Crowd.Stress.Counter
Gym.Crowd.Rental.Browse.<aisle>
Gym.Crowd.Rental.Counter
```

Keep tag names stable across gates so the debugger filter works consistently.
