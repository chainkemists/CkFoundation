# Idioms — verified against shipping gameplay code

Reference for `ck-game-angelscript-gameplay`: the gameplay-AS idiom catalogue — utils, mixins, deferred requests, signal binding.

## 2. Idioms — each verified against shipping gameplay code

House style throughout: Allman braces, 4-space indent, `auto`, `== false`, no `b` bool prefix.
`<Prefix>` is your game's class prefix (BusterBlock uses `Bb_`, Venus uses `Vns_`).

### 2.1 Signal binding: `BindTo_On*` vs `Promise_On*`

Two distinct shapes; picking the wrong one is a common review catch.

- **`BindTo_On<Event>`** — recurring subscription to a feature's typed signal. Delegate FIRST;
  binding policy and post-fire behavior are optional trailing params (`Script/CLAUDE.md` §11–12;
  policy semantics: `ckecs-architecture-contract` §5).
- **`Promise_On<Event>`** — one-shot continuation on a pending operation (entity-script spawn,
  replication readiness). Fires once, then the pending handle is done.

Corpus example (BusterBlock) — recurring attribute subscription, and reading the new value from the
**payload** (`Script/ECS/DayCycle/BB_DayCycle_Processor_Setup.as:22-23,96`):

```angelscript
MinutesAttr.BindTo_OnValueChanged(ECk_MinMaxCurrent::Current,
    FCk_Delegate_IntegerAttribute_OnValueChanged(this, n"OnAnyTimeAttrChanged"));

UFUNCTION()
private void OnAnyTimeAttrChanged(FCk_Handle InAttrOwnerEntity, FCk_Payload_IntegerAttribute_OnValueChanged InPayload)
{
    // InPayload carries the post-mutation value — read it here, NOT via a fresh Get_ this frame (§2.2)
}
```

Corpus example (BusterBlock) — one-shot spawn continuation
(`Script/ECS/CandyDealer/BB_CandyDealer_EntityScript.as:380`; pattern spec:
`Script/CLAUDE.md` §10):

```angelscript
auto Pending = utils_entity_script::Request_SpawnEntity(LifetimeOwner, UMyFeature_EntityScript, Params);
utils_pending_entity_script::Promise_OnConstructed(
    Pending, FCk_Delegate_EntityScript_Constructed(this, n"OnFeatureConstructed"));
```

Replication caveat (one line, owned by `ck-game-replication-patterns`): on clients,
`OnConstructed` means *composed*, not *values applied* — read replicated values only after
`Promise_OnReplicationComplete` (`ckecs-architecture-contract` §7).

### 2.2 Request-then-read: mutations are deferred

`Request_*` calls do not mutate immediately — they queue, and a processor consumes them later in
the frame (or on the authority). The WHY — determinism, replication routing, teardown safety — is
`ckecs-architecture-contract` §3 (root doctrine non-negotiable #5). Consumer consequences:

1. **Never `Request_X` then immediately `Get_X` and expect the new value.** Read the result from
   the change-signal **payload** (§2.1), or settle a frame/tick before reading.
2. Read-modify-write goes through the request too. Corpus example (BusterBlock),
   `Script/ECS/WorkoutStation/BB_WorkoutStation_Hfsm.as:115`:

```angelscript
utils_float_attribute::Request_Override_Current(Attr, Attr.Get_FinalValue() + InAmount);
```

3. Attribute values are `float32`, not AS's 64-bit `float` — cast explicitly. Corpus example
   (BusterBlock), `Script/Objectives/BB_Objective_StoreProgress_EntityScript.as:190`:

```angelscript
utils_float_attribute::Request_Override(Attr, float32(Sum), ECk_MinMaxCurrent::Current);
```

4. A request queued on a dying entity is **never consumed** — don't destroy mid-operation; use the
   feature's Cancel verb (`ck-lifecycle-teardown-campaign`, Mechanics primer).

Your own game features follow the same contract: expose `Request_*` functions that enqueue onto a
Requests fragment, drained by your processor (structure: `ck-game-feature-recipe`).

### 2.3 Mixin call forms — and the two traps

Feature accessors are authored as `mixin` functions on the typesafe handle, so consumers call them
member-style. Corpus example (BusterBlock), `Script/ECS/Entryway/BB_Entryway_Utils.as:113-117`:

```angelscript
mixin const FBb_Fragment_Entryway_Params& Get_Params(const FCk_Handle_Entryway& Self)
{
    auto _CkPerfScope = ck::ScopedStat();
    return Self.Get_Fragment(FBb_Fragment_Entryway_Params);
}
// consumer:  auto Params = MyEntryway.Get_Params();
```

**Trap 1 — the namespace-qualified form doesn't exist for mixins.**
`utils_store_driver::Get_EmployeeManager(InDriver)` → "No matching signatures"; the member form
`InDriver.Get_EmployeeManager()` works. Verified fix commit (BusterBlock `ad077a510`). Same rule
for C++ ScriptMixin-bound statics (`ck-angelscript-interop` §1.2, catalog item 6).

**Trap 2 — a non-const `Self&` mixin won't bind on a by-value or const source.** AS reports
"expected T&, got T" (or no matching signature). Copy to a mutable local first. Verified fix
commit (BusterBlock `5c4fd4572`, `Script/Debugger/Pages/Bb_DebugPage_NamedNpc.as:834`):

```angelscript
auto MutableNamed  = Named;   // mixin Self& won't bind a by-value param — copy to a local
const bool Became  = MutableNamed.Add_Loyalty(constants_named_npc::k_LoyaltyPerItem);
```

The same applies to a handle fresh out of `As_<X>()` in an expression — bind it to a local before
calling mutating mixins (BusterBlock fix `48678651a`).

### 2.4 Handles in f-strings need `.ToString()`

A raw handle in an f-string compiles clean and **throws at runtime** ("Invalid type to append to
string") — invisible to every compile gate including `-skipcompile` headless boots
(`Script/CLAUDE.md` §22.1; `ck-angelscript-interop` catalog item 2). Corpus example (BusterBlock),
`Script/ECS/BalloonDarts/BB_BalloonDarts_EntityScript.as:50`:

```angelscript
ck::EnsureIfNot(ck::IsValid(Actor), f"BalloonDarts entity script [{InHandle.ToString()}] has no owner");
```

### 2.5 Fluent `Set_` chains on Params/Request structs

Framework `_Spec` (né `_ParamsData`) and `Request_` structs expose generated `Set_X` setters that return the
struct for chaining — construct with the required args, then `Set_` the optional ones. Never hunt
for a mega-constructor. Corpus example (BusterBlock),
`Script/ECS/DayCycle/BB_DayCycle_Processor_Setup.as:44-46`:

```angelscript
auto TickParams = FCk_Timer_Spec(FCk_Time(1.0f));
TickParams.Set_StartingState(ECk_Timer_State::Running)
          .Set_Behavior(ECk_Timer_Behavior::ResetOnDone);
auto TickTimer = utils_timer::Add(InHandle, TickParams);
```

(Also `Script/ECS/CandyDealer/BB_CandyDealer_EntityScript.as:107-109` for SM params.)

### 2.6 Your own signals: delegate + `_MC` event pair in a Signals fragment

The house shape for game-feature signals — one single-cast `delegate` (the bind parameter type) and
one multicast `event` suffixed `_MC` (the storage), gathered in a `_Signals` fragment, with lazy
`BindTo_`/`UnbindFrom_` mixins. Corpus example (BusterBlock),
`Script/ECS/Entryway/BB_Entryway_Feature.as:106-117` + `BB_Entryway_Utils.as:141-156`:

```angelscript
delegate void F<Prefix>_Delegate_MyFeature_OnThing(FCk_Handle_MyFeature InFeature, FCk_Handle InEntity);
event    void F<Prefix>_Delegate_MyFeature_OnThing_MC(FCk_Handle_MyFeature InFeature, FCk_Handle InEntity);

USTRUCT()
struct F<Prefix>_Fragment_MyFeature_Signals
{
    F<Prefix>_Delegate_MyFeature_OnThing_MC OnThing;
}

mixin void BindTo_OnThing(FCk_Handle_MyFeature& Self, F<Prefix>_Delegate_MyFeature_OnThing InDelegate)
{
    auto& Fragment = Self.AddOrGet_Fragment(F<Prefix>_Fragment_MyFeature_Signals);   // lazy — added on first bind
    Fragment.OnThing.AddUFunction(InDelegate.GetUObject(), InDelegate.GetFunctionName());
}
```

`UnbindFrom_*` checks `Has_Fragment` first and returns quietly if absent (same file, :148-156).
Venus uses the identical shape — this is generic doctrine.

### 2.7 Tags-as-state: empty USTRUCTs gate processor passes

State that is *presence*, not *data*, is an empty struct added/removed as a fragment; processors
declare it as their dirty trigger. Corpus example (BusterBlock),
`Script/ECS/Entryway/BB_Entryway_Feature.as:119` and
`Script/ECS/DayCycle/BB_DayCycle_Processor_Setup.as:1-12,71`:

```angelscript
struct FBb_Tag_DayCycle_NeedsSetup {}                       // in the Feature file

class UBb_Processor_DayCycle_Setup : UCk_Processor_Script_Base_UE
{
    default _Group = n"FGroup_Gameplay_Script";
    default _MarkedDirtyBy = FBb_Tag_DayCycle_NeedsSetup;   // pass runs only while the tag exists

    UFUNCTION(BlueprintOverride)
    void Tick(FCk_Time DeltaSeconds)
    {
        _Handle.ForEach_EntityWithTwoFragments(
            FBb_Tag_DayCycle_NeedsSetup, FBb_Feature_DayCycle,
            FCk_DynamicFragment_ForEachEntity(this, n"ForEach_Setup"));
    }
    // ForEach_Setup ends with: InHandle.Request_TryRemove(FBb_Tag_DayCycle_NeedsSetup);
}
```

Discovery uses the sibling mechanism `utils_entity_tag::Add(Handle, n"TAG_<Prefix>MyFeature")` so
owners/drivers can find instances later (`ck-game-driver-architecture` owns discovery + delta-gating;
warning: an `EntityTagQuery` continuous-update fires **every frame** — delta-gate on
`_Added`/`_Removed` or you rebuild the population per frame).

### 2.8 `assets::` accessors for Blueprint/content references

Never hardcode content paths in gameplay AS. An editor-side registry config
(`UCkAssetRegistryConfig`, `Script/CLAUDE.md` §13) generates typed accessors into
`Script/Generated/<Game>Assets.as`; gameplay code calls them (3,114 call sites in BusterBlock).
Corpus example (BusterBlock), `Script/UI/InfoWidget/BB_InfoWidgetContainer_Widget.as:63`:

```angelscript
if (ContentClass == nullptr)
{ ContentClass = assets::load::InfoWidgetContent_BB_WBP_Class(); }
```

Blueprints stay data/visual shells (widget trees, meshes, spawn-params); logic lives in AS. Only 3
`BlueprintImplementableEvent`s exist in all of BusterBlock's C++ — BP is not a logic tier here.

### 2.9 Per-function perf scope

Every utils/processor function body in the corpus opens with:

```angelscript
auto _CkPerfScope = ck::ScopedStat();
```

This feeds `stat CkScript` for free (`ck-performance-and-analysis` §1.4). Adopt `ck::ScopedStat()`
project-wide or not at all; the corpus does — it is what makes the drop-to-C++ decision measurable
later (§5). `[UNDER ADJUDICATION — see CkFoundation .claude/reports/ADJUDICATIONS.md A6]`

