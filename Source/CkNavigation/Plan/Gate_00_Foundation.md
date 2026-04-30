# Gate 0 — Foundation

> **Status:** ⏳ Pending
> **Day target:** D1 (sequential — blocks every other gate)
> **Parallelizable:** No

## Goal

Stand up the three new module skeletons (`CkNavigation` slim, `CkCrowd`, `CkCrowdDebugger`),
the agent fragment, the debugger window scaffolding, and one trivial gym that proves
agents can be added/removed. **No movement, no pathfinding yet** — those are Gates 1+2.

After this gate: a build compiles, `ck.CrowdDebugger 1` opens an empty window with all
panels visible, and a Crowd Foundation gym lets the user spawn N agents and see them
listed in the debugger's Agent List panel.

## Acceptance criteria

1. ✅ All three modules compile clean. No warnings.
2. ✅ `Plugins/CkFoundation/CkFoundation.uplugin` lists `CkNavigation` and `CkCrowd` as runtime modules. `Plugins/CkGameplayDebugger/CkGameplayDebugger.uplugin` lists `CkCrowdDebugger`.
3. ✅ `ck.CrowdDebugger 1` opens the window. `ck.CrowdDebugger 0` closes it. Layout matches [mockup §2 idle state](Debugger_Mockup/02_idle.html) (panels visible, mostly empty).
4. ✅ Crowd Foundation gym is registered, walkable, and shows the gym station alcove. A keyboard shortcut on the PlayerController spawns one agent at the FootprintCenter anchor.
5. ✅ Agent List panel shows the live agent count and one row per agent (handle id, agent's tag container if any). No status badges yet — those come in Gate 1+.
6. ✅ Removing an agent (kill or detach) removes the row from the list within 1 frame.
7. ✅ AutoStation `UCk_AutoTest_Crowd_Foundation` runs in a headless variant: spawns 5 agents, asserts they all appear in the DataCollector's snapshot, removes them, asserts the list is empty, calls `FinishSuccess()`.

## File inventory

### `Plugins/CkFoundation/Source/CkNavigation/`

```
CkNavigation.Build.cs
CkNavigation_Module.{h,cpp}
CkNavigation_Log.{h,cpp}
Public/CkNavigation/
    Settings/
        CkNav_ProjectSettings.{h,cpp}     # Empty struct + UCk_Plugin_ProjectSettings_UE base; fields land in Gate 1
```

CkNavigation in Gate 0 is *just* the module skeleton — Gate 1 adds the actual API. The reason it exists this early is so `CkCrowd.Build.cs` can declare a public dep on it.

### `Plugins/CkFoundation/Source/CkCrowd/`

```
CkCrowd.Build.cs
CkCrowd_Module.{h,cpp}
CkCrowd_Log.{h,cpp}
Public/CkCrowd/
    Agent/
        CkCrowdAgent_Fragment.{h,cpp}              # FFragment_CrowdAgent_Params alias, FTag_CrowdAgent_NeedsSetup
        CkCrowdAgent_Fragment_Data.{h,cpp}         # FCk_Handle_CrowdAgent typesafe handle, FCk_Fragment_CrowdAgent_ParamsData
        CkCrowdAgent_Processor.{h,cpp}             # FProcessor_CrowdAgent_Setup (stamps NeedsSetup → does nothing in G0)
        CkCrowdAgent_Utils.{h,cpp}                 # UCk_Utils_CrowdAgent_UE — Add / Remove / Has / Get_*
        ProcessorInjector/
            CkCrowdAgent_ProcessorInjector.cpp     # Registers the Setup processor
    Settings/
        CkCrowd_ProjectSettings.{h,cpp}            # Skeleton; tuning lands in Gate 3+
```

`FCk_Fragment_CrowdAgent_ParamsData` for Gate 0 needs only the structural fields:

```cpp
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_ParamsData);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _Radius = 42.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _Height = 192.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _Tags;
public:
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Tags);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_ParamsData, _Radius, _Height);
};
```

Gate 3 adds `_MaxSpeed`, `_MaxAcceleration`, `_SeparationWeight`, `_Flags`, `_IgnoreFlags`. Gate 4 adds `_PiercingAngle`. Gate 5 adds the player-yield tags.

### `Plugins/CkGameplayDebugger/Source/CkCrowdDebugger/`

```
CkCrowdDebugger.Build.cs
CkCrowdDebugger_Module.{h,cpp}                        # FAutoConsoleCommand "ck.CrowdDebugger" + FGlobalTabmanager::RegisterNomadTabSpawner
CkCrowdDebuggerStyle.{h,cpp}                          # Slate style set (subset of CkGoapDebuggerStyle)
Public/CkCrowdDebugger/
    Data/
        CkCrowdDebugger_DataCollector.{h,cpp}         # Per-tick Collect(UWorld*) — populates _Agents (Gate 0: just count + handles)
        CkCrowdDebugger_Types.h                       # FCkCrowdDebugger_AgentSnapshot (Gate 0: handle + tags only)
    ViewModel/
        CkCrowdDebugger_ViewModel.{h,cpp}             # 5 multicast delegates per CkGoapDebugger pattern
    Window/
        SCkCrowdDebuggerWindow.{h,cpp}                # SSplitter top-level matching mockup
        SCkCrowdDebugger_NavmeshStatusPanel.{h,cpp}   # G0: empty placeholder ("populated in Gate 1")
        SCkCrowdDebugger_AgentListPanel.{h,cpp}       # G0: live list of handles with tag column
        SCkCrowdDebugger_AgentDetailPanel.{h,cpp}     # G0: shows "Identity" section only
        SCkCrowdDebugger_StatsPanel.{h,cpp}           # G0: total-agent count only
        SCkCrowdDebugger_EventLogPanel.{h,cpp}        # G0: empty log
```

DataCollector + ViewModel pattern mirrors `CkGoapDebugger` exactly:

- `FCkCrowdDebugger_DataCollector` is plain C++ (not UObject). Owned by the ViewModel.
- `FCkCrowdDebugger_ViewModel` is plain C++ (not UObject). Owns DataCollector, exposes 5 `TMulticastDelegate<>`s: `OnAgentListChanged`, `OnAgentDataRefreshed`, `OnSelectedAgentChanged`, `OnPausedChanged`, `OnViewModeChanged`. Tick once per frame from window.
- Window owns ViewModel as `TSharedPtr<>`. Each panel takes the same shared pointer in its `SLATE_ARGS`.

### `Plugins/CkTests/Script/CkCrowd/` (new folder)

```
CkCrowdGym_Foundation_GameMode.as
CkCrowdGym_Foundation_PlayerController.as
```

Plus registry update in `Plugins/CkTests/Script/Common/CkTests_GymRegistry.as`:

```as
CkGym_Cycler::RegisterProjectGym("Crowd Foundation", ACkCrowdGym_Foundation_GameMode);
```

The gym has one alcove tagged `Gym.Crowd.Foundation.AgentSpawn`. Press **Space** → spawn a new agent at FootprintCenter. Press **Backspace** → remove the most recent agent.

## Gym spec — manual

Layout:

- One station alcove at world origin, tagged `Gym.Crowd.Foundation.AgentSpawn`.
- Title: `"FOUNDATION"`. Description: `"Press Space to spawn agents. Backspace removes the most recent. Open the debugger with ck.CrowdDebugger 1 to see them."`
- A small text actor floating above the alcove showing live agent count.

Behavior:

- Agents are pure entities (no Actor). Their positions don't move (no steering yet).
- Spawning: `UCk_Utils_CrowdAgent_UE::Add(OwnerEntity, Params)` creates the agent fragment + the typesafe handle. Tags include `Crowd.Agent` (always) and `Crowd.Foundation` (gym-specific).
- The debugger's Agent List should fill up live as agents spawn.
- In the debugger, click any agent to populate the Agent Detail panel (Identity section only — handle, owner entity, tags).

## Gym spec — AutoStation

`UCk_AutoTest_Crowd_Foundation` (in `CkCrowdGym_Foundation_PlayerController.as` or its own file):

```as
class UCk_AutoTest_Crowd_Foundation : UCk_AutoTest_Base
{
    private TArray<FCk_Handle_CrowdAgent> _Agents;
    private FCk_Handle_Timer _StepTimer;

    UFUNCTION(BlueprintOverride)
    void DoConstruct() override
    {
        Set_Status(ECk_AutoTest_Status::Running);
        // Step 1: spawn 5 agents
        for (int32 i = 0; i < 5; ++i)
            _Agents.Add(SpawnAgent(FVector(i * 100, 0, 50)));
        Assert_Equals_Int32(_Agents.Num(), 5, f"5 agents spawned");

        // Step 2 fires after 0.5s — gives DataCollector time to snapshot.
        _StepTimer = CreateOneShot(this, n"Step_VerifySnapshot", FCk_Time(0.5f));
    }

    UFUNCTION()
    void Step_VerifySnapshot(FCk_Handle_Timer InTimer, FCk_Chrono InChrono, FCk_Time InDeltaT)
    {
        const auto Snapshot = UCk_Utils_CrowdDebugger_UE::Get_AgentCount();
        Assert_Equals_Int32(Snapshot, 5, f"5 agents in debugger snapshot");
        for (auto& Agent : _Agents)
            UCk_Utils_CrowdAgent_UE::Remove(Agent);
        _StepTimer = CreateOneShot(this, n"Step_VerifyEmpty", FCk_Time(0.2f));
    }

    UFUNCTION()
    void Step_VerifyEmpty(FCk_Handle_Timer InTimer, FCk_Chrono InChrono, FCk_Time InDeltaT)
    {
        Assert_Equals_Int32(UCk_Utils_CrowdDebugger_UE::Get_AgentCount(), 0, f"empty after remove");
        FinishSuccess();
    }
}
```

`UCk_Utils_CrowdDebugger_UE::Get_AgentCount()` is a small BP-callable wrapper on the DataCollector exposed for assertion use; it returns `_DataCollector->_Agents.Num()`.

## Debugger additions

| Panel | Gate 0 contribution |
|---|---|
| Toolbar | Pause button, refresh button, console-hint label, selection-id readout (empty in G0). Health Check button is greyed-out — wired in Gate 1. |
| Navmesh Status | Static placeholder `"Populated in Gate 1"`. Panel is visible. |
| Agent List | Live list: handle id, tag container (`Crowd.Agent`, etc.). Click to select. Status column header reads `Status` but values are empty `—`. |
| Agent Detail | Identity section only: handle, owner-entity, tags, authority. Other sections show placeholder text `"populated in Gate <N>"`. |
| Stats | `Total agents` field only. |
| Event Log | Empty (`"events appear as gates land"`). |

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| Module circular dep accidentally created (CkCrowd → CkCrowdDebugger) | Low | Debugger has read-only public access to `CkCrowd` types; CkCrowd does not link the debugger. Verified by Build.cs review. |
| `FAutoConsoleCommand` registers too late (CDO timing) | Medium | Use the static-file-scope pattern from CkNavDebugger_UserSettings (per the user's note: "must be registered before CDO construction"). Module StartupModule is too late for some CVar setups, but `FAutoConsoleCommand` itself is fine because it doesn't bind to a UClass property — the existing CkGoapDebugger pattern works. |
| Tab spawning fails when invoked at runtime (not just editor) | Medium | `FGlobalTabmanager` is available at runtime in PIE. Verify with the same pattern CkGoapDebugger uses; if it fails, add a fallback that hosts the window in a transient SWindow instead of a tab. |

## Done criteria checklist (mark before opening Gate 1)

- [ ] All 3 modules in their respective .uplugin manifests.
- [ ] Compiles green; no warnings.
- [ ] `ck.CrowdDebugger 1` opens an empty window matching mockup §2 layout.
- [ ] Crowd Foundation gym walkable, manual spawn/remove works.
- [ ] AutoTest passes headlessly.
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] CkCrowd/Claude.md created with the agent fragment / utils API documented at the level of "what's there now."
