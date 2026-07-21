# Source/CLAUDE.md — module topology & cross-module patterns

This file owns the **module map** of CkFoundation/Source: which module solves which problem, the
dependency tier table, and the patterns that span modules. It does NOT restate rules owned
elsewhere: **style, naming, macros, signals, requests, error handling, non-negotiables,
collaboration, engine/EnTT versions, skill index** → root [CLAUDE.md](../CLAUDE.md) (doctrine of
record); **AngelScript** (language deltas, `utils_*`, dynamic handles, `asset ... of ...`
definitions) → [Script/CLAUDE.md](../Script/CLAUDE.md); **editor-only modules** →
[EDITOR_MODULES.md](EDITOR_MODULES.md).

Facts below were verified against Build.cs files, headers, and git history on **2026-07-02**
(re-verification commands at the bottom). Claims marked INFERRED are reasoned, not confirmed.

## Documentation navigation order

Before writing any code, navigate the documentation in this order:

1. This file — project-wide topology; find the right module and cross-module pattern.
2. The target module's `Claude.md` — purpose, key API, anti-patterns.
3. [CkCore/Claude.md](CkCore/Claude.md) — use-case lookup table for utilities.
4. Subfolder READMEs in `CkCore/Public/CkCore/<Folder>/README.md` — API details (48 files).

## Finding the right module — "I need to..."

| I need to... | Module / entry point |
|---|---|
| validate a handle or custom type | `CkCore/Validation` + `ck::IsValid` |
| assert a precondition with diagnostics | `CkCore/Ensure` + `CK_ENSURE_IF_NOT` |
| format a string | `CkCore/Format` + `ck::Format_UE` |
| get world time | `CkCore/Time` + `Get_WorldTime` (pattern below) |
| countdown/accumulate ticks (no entity) | `CkCore/Chrono` + `FCk_Chrono` |
| define a getter/setter macro | `CkCore/Macros` + `CK_PROPERTY` (semantics: root CLAUDE.md) |
| check/intersect gameplay tags | `CkCore/GameplayTag` |
| walk FProperty / reflection | `CkCore/Reflection` |
| bound range [min,max] + normalize | `CkCore/Math/ValueRange` |
| create/destroy entities | `CkEcs` + `UCk_Utils_EntityLifetime_UE` |
| pool/recycle a UObject (or subsystem-own its lifetime) | `CkCore/ObjectPooling` — `UCk_Utils_Object_UE::Request_CreateNewObject` with `FCk_ObjectPooling_PoolParams` (Recycle) or DestroyOnRelease to just pin; release via `TryReleaseToPool`. Poolable EntityScripts: the `InstancedPerEntity_Poolable` policy |
| write a processor | `CkEcs/Processor` (`TProcessor`, self-registered via `CK_REGISTER_PROCESSOR`) |
| bind/fire signals | `CkEcs/Signal` + `CK_SIGNAL_BIND` / `CK_SIGNAL_UNBIND` |
| actor ↔ entity bridge | `CkEcs/OwningActor` (`UCk_Utils_OwningActor_UE`) + `CkActor`; EntityHolder in `CkEcsExt` |
| data-driven entity logic (spawnable unit) | `CkEcs/EntityScript` (`UCk_EntityScript_UE`) |
| store entity-role identity (1 tag) | `CkLabel` (`UCk_Utils_GameplayLabel_UE`) |
| store entity's child entities | `CkRecord` |
| add configurable typed data via data assets | `CkProvider` |
| replicated named variables on an entity | `CkVariables` |
| store multiple behavior tags (replicated) | `CkTagSet` |
| FName-flavored multi-tags + tag queries | `CkEntityTag` |
| higher-level ECS (SceneNode, Meta, Transform) | `CkEcsExt` |
| attach/manage UActorComponents on entities | `CkUnrealComponent` (no doc yet) |
| place/spawn EntityScripts in a level | `CkEntitySpawner` (`AInfo`-derived spawner actor; no doc yet) |
| pool/recycle EntityScript-spawned entities, UObjects, or actors | `CkPool` (`UCk_Utils_EntityPool_UE` promise-based / `UCk_Utils_ObjectPool_UE` synchronous; budgeted prewarm, per-class project settings) |
| entity presets / archetypes | EntityScript spawn params (`FInstancedStruct`, `CkEntityScript.h:65`) + `CkProvider`. CkTemplate/CkEcsTemplate were REMOVED (`ad045415b`); these are the successors (INFERRED) |
| ECS timers with signals/delegates | `CkTimer` |
| ECS interpolation / follow a spline | `CkTween` (+ `CkSpline` for path data) |
| ECS typed attributes (health/mana) | `CkAttribute` |
| ECS audio tracks | `CkAudio` |
| ECS Niagara VFX | `CkVfx` |
| one-shot gameplay cues (base framework) | `CkCue` |
| ECS camera shake | `CkCamera` |
| ECS animation assets | `CkAnimation` |
| ECS state machine (data-asset conditions) | `CkStateMachine` |
| ECS inventory + 2D grid | `CkInventory` + `CkGrid` |
| ECS physics acceleration/forces | `CkPhysics` |
| ECS projectiles / ballistics | `CkProjectile` |
| ECS interaction channels | `CkInteraction` |
| ECS spatial overlap/collision | `CkOverlapBody` + `CkShapes` |
| ECS spatial volume query / probes (Jolt) | `CkSpatialQuery` |
| own/step/query the Jolt physics world | `CkJolt` |
| native entity queries (rings/cones over entities) | `CkEqs` (no doc yet; NOT UE's EQS) |
| UE EQS wrappers | `CkAi` |
| AI perception → ECS | `CkPerception` |
| navmesh integration (paths, projection) | `CkNavigation` |
| crowd steering / avoidance | `CkCrowd` |
| sidewalk/path preference + authoring (ZoneGraph-lite) | `CkPathNetwork` (+ `CkPathNetworkEditor` tooling) |
| GOAP planner (A* over Action entities) | `CkGoap` |
| grid-based pathfinding | `CkAStar` + `CkGrid` |
| ECS raycast sensing | `CkRaySense` |
| replicate render-target pixels / draw calls | `CkRenderTarget` (no doc yet) |
| runtime shader Looks / outline rendering | `CkUsf` |
| ISM / skeletal-instance rendering | `CkIsmRenderer` / `CkIskmRenderer` |
| vertex-animation-texture playback (bake skeletal anims to textures, tick-less ISM instances) | `CkVat` (+ `CkVatEditor` baker) |
| ECS targeting / scoring | `CkTargeting` |
| ECS aggro / threat table | `CkAggro` |
| entity relationships (ally/enemy, teams) | `CkRelationship` |
| multi-source damage/buff resolution | `CkResolver` |
| entity-to-entity messages | `CkMessaging` |
| runtime-composable behaviors / dynamic handles | `CkDynamic` |
| tracked entity sets with change detection | `CkEntityCollection` |
| post-construction opt-in fragments | `CkEntityExtension` |
| quest-like objectives | `CkObjective` |
| save/restore world state (snapshots) | `CkSnapshot` (v3 rebuild+hydrate — see `CkSnapshot/Claude.md`) — features persist via a Produce/HydrationApply handler on `FCk_PersistenceHandlerRegistry` (`CkEcs/Persistence/`, save subset `Get_SaveHandlerTypes`), NOT a per-fragment macro (`CK_REGISTER_SNAPSHOTABLE` removed 2026-07-13) |
| session state machine | `CkGameSession` |
| CommonUI-based UI layer | `CkUI` |
| dependency-gated loading screen | `CkLoadingScreen` (subsystem + `ICk_LoadingProcess` holders) |
| Enhanced Input IMC lifecycle | `CkInput` |
| async asset loading → fragments | `CkResourceLoader` |
| relay entity events to an actor (channels) | `CkActorRelay` |
| debug shapes / procedural mesh text | `CkPmg` |
| physics-substep ticking | `CkSubstep` |
| project settings exposed to editor | `CkSettings` |
| console variables / runtime tuning | `CkCVar` |
| style tokens for editor-tool Slate UI (colors, fonts, tones) | `CkEditorTools` + `CkStyle::` |
| log a message | `CkLog` (per-module `ck::<feature>` functions — root CLAUDE.md) |
| profile a processor | `CkProfile` + `SCOPE_CYCLE_COUNTER` |
| generate AngelScript accessors | `CkAngelscriptGenerator` (Editor module) |

## Module tier table

All **75 non-editor modules** (CkVat added 2026-07-09; CkPool added 2026-07-07), regenerated from every `Source/<Module>/<Module>.Build.cs` on
2026-07-02. **Deps column = Ck-only** (Public + Private combined, `Ck` prefix stripped); engine
modules are not listed. Tiers are semantic bands; a module may sit higher than its minimal depth,
but **deps must never point to a higher band**. Editor/UncookedOnly modules are excluded (see T5).

### T0 — roots (no Ck deps)

| Module | Ck deps | Notes |
|---|---|---|
| CkBuildConfig | — | Hosts the shared `CkModuleRules` base; not listed in the uplugin |
| CkSettings | — | DeveloperSettings base; not listed in the uplugin |
| CkThirdParty | — | Vendored: EnTT, JoltPhysics, fmt, cleantype, ctti, delegate, bitwise-enum |
| CkIskmRendererVF | — | Engine-only vertex-factory shim for CkIskmRenderer (RenderCore/RHI/Renderer); `PostConfigInit` so the VF registers before the engine seals its factory list; plain ModuleRules |

### T1 — foundation

| Module | Ck deps |
|---|---|
| CkCVar | Core,Log |
| CkCore | BuildConfig,Log,Settings,ThirdParty |
| CkEditorTools | Settings (added 2026-07-09; Runtime on purpose — consumed by CkGameplayDebugger's Runtime modules; hosts the shared `CkStyle::` tokens + `UCk_Style_UserSettings_UE`) |
| CkLog | Settings,ThirdParty |
| CkMemory | Core,Log |
| CkPerception | Core,Log,ThirdParty |
| CkProfile | Core,Log |

### T2 — ECS core + direct-attach primitives

| Module | Ck deps |
|---|---|
| CkAi | Core,Ecs,Log |
| CkEcs | Core,Log,Memory,Profile,Settings,ThirdParty |
| CkInput | Core,Ecs,Log,Settings |
| CkLabel | Core,Ecs,Log |
| CkLoadingScreen | Core,Log,Settings |
| CkPool | Core,Ecs,Label,Log,Record,Settings |
| CkProvider | Core,Ecs,Log |
| CkRecord | Core,Ecs,Label,Log |
| CkResourceLoader | Core,Ecs,Log,Settings |
| CkTagSet | Core,Ecs,Log |
| CkVariables | Core,Ecs,Log |

### T3 — actor bridge

| Module | Ck deps |
|---|---|
| CkActor | Core,Ecs,Log,Variables |
| CkEcsExt | Actor,Core,Ecs,Label,Log,Record,Settings |

### T4 — feature modules

| Module | Ck deps |
|---|---|
| CkAStar | Core,Ecs,EcsExt,Log |
| CkActorRelay | Core,Ecs,EcsExt,Label,Log,Settings |
| CkAggro | Attribute,Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkAnimation | Core,Ecs,EcsExt,Label,Log,Provider,Record |
| CkAttribute | Core,Ecs,EcsExt,Label,Log,Provider,Record |
| CkAudio | ActorRelay,Core,Cue,Ecs,EcsExt,Label,Log,Provider,Record,Settings,Timer |
| CkCamera | Attribute,Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkChaos | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings,Targeting |
| CkCompass | Camera,Core,Ecs,EcsExt,Log,Poi,UI |
| CkCompositeAlgos | Core,Ecs,EcsExt |
| CkConsoleCommands | Core,Ecs,Label,Log,Record,Settings |
| CkCrowd | Core,Ecs,EcsExt,Label,Log,Navigation,Physics,Pmg,Projectile,Record,Settings,Shapes,SpatialQuery |
| CkCue | ActorRelay,Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings,Timer |
| CkDynamic | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkEntityCollection | Core,Ecs,EcsExt,Label,Log,Record,Settings |
| CkEntityExtension | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkEntitySpawner | ActorRelay,Core,Ecs,EcsExt,Log,Settings |
| CkEntityTag | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkEqs | Core,Ecs,EcsExt,EntityTag,Label,Log,Record,Settings,Shapes,SpatialQuery,ThirdParty |
| CkFx | Core,Ecs,EcsExt,Label,Log,Record,Settings |
| CkGameSession | Core,Ecs,Label,Log,Record,Settings |
| CkGoap | AStar,Core,Ecs,EcsExt,Label,Log,Record |
| CkGraphics | Core,Ecs,Log,Variables |
| CkGrid | Core,Ecs,EcsExt,Label,Log,Record,Settings (+EntitySpawner, editor-only) |
| CkInteraction | Attribute,Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkInventory | Attribute,Core,Ecs,EcsExt,Grid,Label,Log,Record,Settings,TagSet |
| CkJolt | Core,Ecs,EcsExt,Log,Settings,ThirdParty (owns the Jolt world; extracted from CkSpatialQuery 2026-07-16; +EcsExt Phase 3, also engine PhysicsCore/Landscape) |
| CkIsmRenderer | Core,Ecs,EcsExt,Graphics,Label,Log,Provider,Record,Settings |
| CkIskmRenderer | Animation,Core,Ecs,EcsExt,Graphics,IskmRendererVF,Label,Log,Physics,Provider,Record,Settings |
| CkMessaging | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkMinimap | Camera,Core,Ecs,EcsExt,Label,Log,Poi,Record,UI |
| CkNavigation | Core,Ecs,EcsExt,Label,Log,Record,Settings |
| CkPathNetwork | AStar,Core,Ecs,EcsExt,Label,Log,Navigation,Record,Settings |
| CkObjective | ActorRelay,Attribute,Core,Cue,Ecs,EcsExt,EntityCollection,Label,Log,Provider,Record,Settings |
| CkOverlapBody | Actor,Core,Ecs,EcsExt,Graphics,Label,Log,Physics,Record,Settings |
| CkPhysics | Actor,Chaos,Core,Ecs,EcsExt,Label,Log,Record |
| CkPmg | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkPoi | Core,Ecs,EcsExt,Log,Settings |
| CkProjectile | Core,Ecs,EcsExt,Log,Physics,Record,Variables |
| CkRaySense | Core,Ecs,EcsExt,IsmRenderer,Label,Log,Provider,Record,Settings,Shapes |
| CkRelationship | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkRenderTarget | ActorRelay,Core,Ecs,EcsExt,Label,Log,Profile,Record,Settings,Timer |
| CkResolver | Attribute,Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings,Targeting |
| CkShapes | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkSnapshot | Core,Ecs,EcsExt,Log,ThirdParty |
| CkSpatialQuery | Core,Ecs,EcsExt,Jolt,Label,Log,Physics,Provider,Record,Settings,Shapes,ThirdParty |
| CkSpline | Core,Ecs,EcsExt,Log |
| CkStateMachine | ActorRelay,Core,Dynamic,Ecs,Label,Log,Provider,Record,Settings,Timer |
| CkSubstep | Core,Ecs,EcsExt,Label,Log,Record,Settings |
| CkTargeting | Actor,Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings |
| CkTimer | Core,Ecs,EcsExt,Label,Log,Profile,Record |
| CkTween | Core,Ecs,EcsExt,Label,Log,Provider,Record,Settings,Spline,Timer |
| CkUI | Core,Ecs,EcsExt,GameSession,Graphics,Log,Settings,ThirdParty |
| CkUnrealComponent | Core,Ecs,EcsExt,Label,Log,Record,Settings |
| CkUsf | Core,Ecs,Graphics,Log |
| CkVat | Core,Ecs,EcsExt,Graphics,IsmRenderer,Log,Usf |
| CkVfx | ActorRelay,Core,Cue,Ecs,EcsExt,Label,Log,Provider,Record,Settings,Timer |
| CkWatermark | Core,Ecs,Jolt,Log,Memory,Settings,UI |

### T5 — editor modules (25 UncookedOnly + 3 Editor; runtime code must NEVER depend on these)

Full reference: [EDITOR_MODULES.md](EDITOR_MODULES.md). 18 are `Ck<Feature>Editor` twins. The
standalone ones: `CkEditorGraph` (graph/schema base), `CkEditorStyle` (shared style, PreDefault),
`CkEditorToolbar`, `CkK2Nodes` (BP node customizations), `CkDataViewer` (entity state overlay),
`CkInsightsAnalyzer` (Insights trace analysis), `CkAssetExporter` (asset data → JSON). Editor-type:
`CkAngelscriptGenerator` (PreDefault; emits `Script/Generated/`), `CkPieLayoutEditor`, `CkUsfEditor`.

### Table notes

- **Per-module docs** live at `Source/<Module>/Claude.md` (CkGoap's is `CLAUDE.md`). **No doc yet:**
  CkEqs, CkRenderTarget, CkSpline, CkEntitySpawner, CkUnrealComponent.
  CkIskmRendererVF is covered by CkIskmRenderer's doc. CkCrowd's and CkNavigation's docs were
  flagged stale on 2026-07-02 ("not yet created" / "skeleton only" — both modules are fully built);
  trust code over doc and note the drift.
- **`CkScripts/` is NOT a module** (no Build.cs) — a support dir holding maintenance scripts
  (`CkLfsLocks`, `CkEcsTemplateReplacer.ps1`). The latter still references the deleted CkEcsTemplate
  scaffold and is stale.
- CkTemplate and CkEcsTemplate were removed in commit `ad045415b` (2026-06-09). Do not re-add rows
  for them.

## Module-authoring rules

- **Scaffold by mimicry, not from the stale replacer script:** copy the smallest complete feature
  quartet (`CkTimer` — the root doctrine's canonical exemplar) and rename.
- Build.cs inherits `CkModuleRules` (`Source/CkBuildConfig/CkBuildConfig.Build.cs`): C++20, unity
  build, per-config define matrix, auto-detected `WITH_ANGELSCRIPT_CK`. Only CkThirdParty and
  CkIskmRendererVF use plain `ModuleRules` — don't add a third without cause.
- Add the module to `CkFoundation.uplugin` with the standard Win64/Mac/Linux allowlist. LoadingPhase
  is `Default` unless you can justify otherwise (only 3 modules deviate today).
- Dependency discipline: depend only on same-or-lower tiers; runtime never on T5. Editor-only deps
  go inside `if (Target.bBuildEditor)` (see CkGrid → CkEntitySpawner).
- Ship a `Claude.md` with the module (purpose, key API, anti-patterns) and add its row here.

## Key cross-module patterns

### "Add a feature to an entity" (the composition ritual)

Canonical implementation: `UCk_Utils_Timer_UE::Add` (`CkTimer/Public/CkTimer/CkTimer_Utils.cpp:40-81`):

1. `UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, ...)` — create the feature's child
   entity under the owner.
2. `UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_TimerName())` — label it.
3. Add the Params/Current fragments and lifecycle tags (`FTag_Timer_NeedsSetup`, ...) on the child.
4. Connect it to the owner's Record: `RecordOfTimers_Utils::AddIfMissing(InHandle, ...)` +
   `Request_Connect(InHandle, NewTimerEntity, ...)`.
5. The feature's processors drive the child entity from there.

Simpler features add fragments directly on the target entity instead of creating a child — read the
target module's `Add` before assuming which shape it uses.

### Entity game logic — EntityScript

`UCk_EntityScript_UE` (C++ / Blueprint / AS) lifecycle: `Construct → BeginPlay → EndPlay`.

- `Construct(FCk_Handle&, const FInstancedStruct& InSpawnParams)` returns
  `ECk_EntityScript_ConstructionFlow::Finished` (BeginPlay fires) or `::Continue` (you must call
  `DoFinishConstruction()` when ready). Spawn params are the data-preset mechanism.
- Construct: compose features, spawn child entities. BeginPlay: bind signals, start timers.
  EndPlay: unbind signals (automatic for binds made with `PostFireBehavior::Unbind`).
- Self-access: `Get_AssociatedEntity()` (C++) / `DoGet_ScriptEntity()` (BP/AS).
- Full lifecycle, instancing policy, and replication contract: [CkEcs/Claude.md](CkEcs/Claude.md).

### Signal (event) flow

Processor detects condition → `UUtils_Signal_OnX::Broadcast(Handle, Payload)` → delegate bound in
EntityScript BeginPlay fires → EntityScript acts (further `Request_*` calls, or
`UCk_Utils_EntityLifetime_UE::Request_DestroyEntity`). Macros, binding policies, and lifecycle:
root CLAUDE.md + `ckecs-architecture-contract` skill.

### Reading values from providers in a processor

```cpp
const auto Val = ck::IsValid(InParams.Get_MyProvider())
    ? InParams.Get_MyProvider()->Get_Value(InHandle)
    : DefaultValue;
```

### Component lifetime (Niagara, Audio, any UObject the entity owns)

Setup processor creates → monitor processor observes and fires signals, never destroys → EndPlay
processor destroys during entity cleanup:

```cpp
struct FFragment_VfxCue_Current
{
    friend class FProcessor_VfxCue_Setup;
    friend class FProcessor_VfxCue_EndPlay;

private:
    TStrongObjectPtr<UNiagaraComponent> _NiagaraComponent;

public:
    CK_PROPERTY_GET(_NiagaraComponent);
};

// Setup — create + store (factory function, see below):
InCurrent._NiagaraComponent = TStrongObjectPtr{Component};

// LifetimeMonitor — observe, fire signals, DO NOT destroy:
auto Component = InCurrent._NiagaraComponent.Get();
if (ck::IsValid(Component) && NOT Component->IsActive())
{ UUtils_Signal_OnFinished::Broadcast(InHandle, ...); }

// EndPlay — destroy during entity cleanup:
auto Component = InCurrent._NiagaraComponent.Get();
if (ck::IsValid(Component))
{ Component->DestroyComponent(); }
InCurrent._NiagaraComponent.Reset();
```

Cleanup order this guarantees: signal fires → EntityScript reacts (destroys entity if that's the
behavior) → EndPlay processor destroys the component. Component lifetime is tied to entity
lifetime. Never call `DestroyComponent()` from a monitor/update processor.

### Standalone components (no actor owner)

`NewObject<UNiagaraComponent>(World)` + manual `RegisterComponent()` fails or fires ensures. Use
the factory functions that register with the world, with every bool argument named:

```cpp
constexpr auto AutoDestroy = false;
constexpr auto AutoActivate = false;
constexpr auto PreCullCheck = true;
auto Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    World, Effect, Location, Rotation, Scale,
    AutoDestroy, AutoActivate, ENCPoolMethod::None, PreCullCheck);
```

Same idea for audio (`UGameplayStatics::SpawnSoundAtLocation`) and other component types.

### Replicated + persisted fragments — the persistence-handler contract

The SAME registered projection drives the net wire AND the save/load path. Registry:
`FCk_PersistenceHandlerRegistry` (`CkEcs/Persistence/CkPersistenceHandlerRegistry.h`) — split out of Net/ so the
save path (CkSnapshot) reuses it without a Net dependency. Client-side application of replicated container data is
deferred: net receive only marks entries pending; `FProcessor_ReplicatedFragments_Dispatch` drains them each tick;
the load path uses the twin `FProcessor_Hydration_Dispatch`. Register in the feature's `_Fragment.cpp` — **prefer a
named participation shape** (`Register_NetOnly` / `Register_SaveOnly` / `Register_NetAndSave_SharedApply` /
`Register_NetAndSave_SplitApply`) over hand-building an `FHandler`. Each takes a **designated-init args struct** so
every lambda is labeled at the call site, and required slots are compile-enforced (an omitted `.Produce`/
`.HydrationApply`/`.NetApply` does not compile — the `Produce`-without-`HydrationApply` misconfig is *uncompilable*):

```cpp
// Team — both transports, one authority-safe applier (net receive + load hydration share the body):
FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_Team>({
    .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>   // save capture; READ-ONLY
    {
        if (NOT UCk_Utils_Team_UE::Has(Entity)) { return {}; }
        return FInstancedStruct::Make(FCk_RepData_Team{Entity.Get<ck::FFragment_TeamInfo>().Get_TeamID()});
    },
    .SharedApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
    {
        if (NOT UCk_Utils_Team_UE::Has(Entity))                        // NotReady BEFORE any mutation
        { return ECk_Persistence_ApplyResult::NotReady; }
        UCk_Utils_Team_UE::Assign(UCk_Utils_Team_UE::Cast(Entity), New.Get<FCk_RepData_Team>().Value);
        return ECk_Persistence_ApplyResult::Applied;
    },
});
```

- `NetApply` runs AFTER OnConstructed-driven composition — never inline during net receive — and must
  **never compose the feature itself**; composition belongs to construction. `HydrationApply` (the load-path twin)
  is authority-side and follows the same NotReady-before-mutation rule.
- Return `NotReady` while the target feature is not composed yet: the dispatcher retries each tick
  and past a timeout drops the entry LOUDLY (ensure naming the type and entity). A perpetual
  timeout means the feature is never composed on the client.
- `Old` is unset on the first application; otherwise it holds the last APPLIED data (net path only —
  the load path never coalesces, so `HydrationApply`'s `Old` is always unset).
- Consumer consequence: **`OnConstructed` means composed, not values-applied** — read replicated
  values only from `Promise_OnReplicationComplete`. Full contract: [CkEcs/Claude.md](CkEcs/Claude.md); the
  save/load authoring recipe: [CkSnapshot/Claude.md](CkSnapshot/Claude.md).

### Variant dispatch — `ck::Visitor` takes ONE generic lambda

Defined in `CkCore/TypeTraits/CkTypeTraits.h`; canonical use `CkTimer_Processor.cpp:51`:

```cpp
// CORRECT — single generic lambda; overload the handler instead:
ck::algo::ForEachRequest(RequestsCopy, ck::Visitor(
    [&](const auto& InRequest) -> void
    {
        DoHandleRequest(InHandle, InCurrent, InRequest);
    }), ck::policy::DontResetContainer{});

// WRONG — ck::Visitor is not a std::visit overload-set:
ck::Visitor([&](const AddRequest&) { ... }, [&](const RemoveRequest&) { ... })
```

Disambiguate per-type via `DoHandleRequest` overloads on the handler side.

### Algorithm library (`ck::algo`)

`CkCore/Public/CkCore/Algorithms/CkAlgorithms.h`:

| Function | Variants | Notes |
|---|---|---|
| `Filter` / `FilterInPlace` | copy / mutating | distinct names because return-value assignment differs |
| `Sort` | in-place (`&`), copy (`const&`) | |
| `Except(A, B)` / `Intersect(A, B)` | basic + projection overloads | set difference / intersection |
| `Transform` | to new container, into existing | map |
| `ForEach` | container, iterator, `IsValid` variants | |
| `ForEachRequest` | `TArray`, `TOptional`, `DontResetContainer` policy | request draining |
| `AllOf` / `AnyOf` / `NoneOf` | container, iterator | |
| `FindIf` | iterator, `TOptional` return | |
| `CountIf` / `FindIndex` | container | |

Projection overloads take a member-function pointer as the projected key, e.g.
`ck::algo::Except(Current, Previous, &FCk_InventoryItem_ReplicatedEntry::Get_ItemHandle)`.

### Technique pipeline (`ck::Technique`)

`CkCore/Public/CkCore/Technique/CkTechnique.h` — CRTP step pipeline for processor logic with 3+
distinct phases; step names replace `// ---- Phase N ----` comments.

```cpp
struct FTechnique_MyOperation
    : ck::Technique<FTechnique_MyOperation, FContext_MyOperation&>
{
    FTechnique_MyOperation()
    {
        AddStep(&FTechnique_MyOperation::Validate);
        AddStep(&FTechnique_MyOperation::ProcessItems);
    }

    auto ShouldAbort() const -> bool { return _Abort; }   // optional, SFINAE-detected

    static auto Validate(FTechnique_MyOperation& InSelf, FContext_MyOperation& InCtx) -> void;
    static auto ProcessItems(FTechnique_MyOperation& InSelf, FContext_MyOperation& InCtx) -> void;

    bool _Abort = false;
};

// In the processor — instances hold only the step list, so static is fine:
static auto Technique = FTechnique_MyOperation{};
Technique._Abort = false;
Technique.ProcessAllSteps(Context);
```

Steps run in registration order; `ShouldAbort()` (if defined) is checked between steps. State lives
in the plain-aggregate Context, not the Technique.

## UE specifics — verified helpers

**Actor ↔ entity (C++).** There is no `ck::SelfEntity(this)` / `ck::GetOwnerEntity()` — older docs
taught these but they exist nowhere in Source. The real API is `UCk_Utils_OwningActor_UE`
(`CkEcs/Public/CkEcs/OwningActor/CkOwningActor_Utils.h`):

- Actor → entity: `Get_ActorEntityHandle(InActor)` (:101) / `TryGet_ActorEntityHandle(InActor)` (:110)
- Entity → actor: `Get_EntityOwningActor(InHandle)` (:61) / `TryGet_EntityOwningActor(InHandle)` (:69)
  / `TryGet_EntityOwningActor_Recursive(InHandle)` (:77)
- Readiness: `Get_IsActorEcsReady(InActor)` (:117), `Promise_OnActorEcsReady(InActor, InDelegate)` (:125)

Inside an EntityScript use `Get_AssociatedEntity()`; context owner via
`UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle)` (`CkEcs/ContextOwner/CkContextOwner_Utils.h:30`).
The AngelScript equivalent is `ck::ToEntity(Actor)` / `ck::ToEntity(EntityScript)`
(`Script/CkUtils_Common.as:5,10`) — see [Script/CLAUDE.md](../Script/CLAUDE.md).

**World from an entity:** `UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle)`
(`CkEcs/EntityLifetime/CkEntityLifetime_Utils.h:128`).

**World time** (`CkCore/Time/CkTime_Utils.h` — params struct constructs from `UWorld*` or `UObject*`):

```cpp
const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
const auto CurrentTime = TimeResult.Get_WorldTime().Get_Time();
```

**Common includes:** `CkCore/Chrono/CkChrono.h` (`FCk_Chrono`), `CkCore/Time/CkTime_Utils.h`.

## Owned elsewhere — do not look for it here

- Function formatting, code style, `CK_PROPERTY` encapsulation, interface design, request structs,
  enum+value optionality, signal macros/binding policies, error handling & logging → root
  [CLAUDE.md](../CLAUDE.md). "No fallbacks that hide problems" is root non-negotiable #3.
- Collaboration workflow (Research → Plan → Implement, stuck protocol) → root "Collaboration protocol".
- AngelScript compatibility + `asset ... of ...` asset definitions → [Script/CLAUDE.md](../Script/CLAUDE.md).
- Testing layers (AutoTest / Gauntlet / gym) → `ck-tests-authoring-and-running` skill (CkTests).

## Provenance and maintenance

Verified 2026-07-02 against the working tree (submodule HEAD `7330c1bab`). Re-derivation:

- **Tier table:** per module, collect `"Ck*"` literals from both dep lists:
  `rg --no-ignore -A30 'DependencyModuleNames' Source/<M>/<M>.Build.cs` — watch for
  editor-conditional blocks (`if (Target.bBuildEditor)`).
- **Module types/phases:** `"Type"` / `"LoadingPhase"` entries in `CkFoundation.uplugin`.
- **Doc coverage:** `Get-ChildItem Source -Recurse -Depth 2 -Filter Claude.md` (the Grep/Glob tools
  can false-empty here — root CLAUDE.md provenance notes).
- **Cited symbols:** all grepped/read 2026-07-02; if one goes missing, re-verify with
  `rg -n '<symbol>' Source/<Module>` before editing this file.
