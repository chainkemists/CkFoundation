# Reaching a driver, and discovery mechanics

Reference for `ck-game-driver-architecture`: the sanctioned no-globals forms for reaching a driver (§3) and the mechanics of doing tag discovery right (§4).

## 3. Reaching a driver — the sanctioned forms (no globals)

Verified BusterBlock reality: drivers are reached by **tag scan scoped through the
lifetime-owner chain**, or by an **Acquire ticket + promise**, or by **injection** (§2). Note:
`ck::Ctx` is how SM/task micro-entities resolve their *own subject entity* — it is **not** the
driver-lookup mechanism (verified by grep: all `ck::Ctx` call sites are SM tasks/conditions
resolving their host).

Ranked, strongest first:

1. **Injection** — the dependency arrives in `Params(...)` at spawn. Zero discovery, zero races.
   Use whenever the spawner already holds the handle.
2. **Acquire ticket + promise** — for consumers that may run before the driver exists. The
   feature's Utils exposes `Acquire<Driver>(WorldContext)` returning a pending handle whose
   `Promise_OnReady` fires *now* if the driver is already ready, else is late-resolved by a flush
   processor. This is the guaranteed-delivery form; teach it to every consumer of your driver.
3. **Scoped tag scan** — `TryFind_<Driver>(Context)` for opportunistic, may-return-invalid reads.

The Acquire-ticket recipe (three pieces, all in the driver's feature dir — verified against
`Script/ECS/DayCycle/BB_DayCycle_Utils.as:56-93` and
`Script/StoreDriver/BB_StoreDriver_Utils.as:112-150` + `BB_StoreDriver_Processor.as:6-36`):

```angelscript
namespace utils_my_driver
{
    // (a) Scoped scan. Descendant-preferring: first lifetime-descendant of InContext
    //     wins (disambiguates multi-instance maps and isolates concurrent tests);
    //     global fallback covers consumers outside the lifetime chain (HUD widgets).
    FCk_Handle_MyDriver TryFind_Driver(const FCk_Handle& InContext)
    {
        auto FirstAny = FCk_Handle_MyDriver();
        for (auto RO : utils_entity_tag::ForEach_Entity(InContext, n"TAG_<Prefix>MyDriver"))
        {
            auto Driver = RO.As_MyDriver(ECk_SanityCheck::UnChecked);
            if (ck::Is_NOT_Valid(Driver))
            { continue; }
            if (Is_DescendantOf(RO, InContext))
            { return Driver; }
            if (ck::Is_NOT_Valid(FirstAny))
            { FirstAny = Driver; }
        }
        return FirstAny;
    }

    // (b) Ticket factory. The driver need not exist yet.
    FCk_Handle_PendingMyDriver AcquireMyDriver(const FCk_Handle& InWorldContext)
    {
        auto PendingEntity = utils_entity_lifetime::Request_CreateEntity(InWorldContext);
        PendingEntity.Add_Fragment(F<Prefix>_Feature_PendingMyDriver());
        auto& Frag = PendingEntity.AddOrGet_Fragment(F<Prefix>_Fragment_PendingMyDriver);
        Frag.Driver = TryFind_Driver(InWorldContext);
        return PendingEntity.As_PendingMyDriver();
    }
}

// (c) Promise mixin: fire inline if already ready, else stamp Unresolved so the
//     flush processor (dirty-gated on that tag) retries the find and fires later.
mixin void Promise_OnReady(FCk_Handle_PendingMyDriver& Self, F<Prefix>_Delegate_MyDriver_OnReady InDelegate)
{
    auto& Frag = Self.AddOrGet_Fragment(F<Prefix>_Fragment_PendingMyDriver);
    Frag.Promise = InDelegate;
    if (ck::IsValid(Frag.Driver) && Frag.Driver.Get_IsReady())
    { InDelegate.ExecuteIfBound(Frag.Driver); return; }
    Self.AddOrGet_Fragment(F<Prefix>_Tag_PendingMyDriver_Unresolved);
}
```

Scoping details that earn their keep (all verified in `BB_StoreDriver_Utils.as:50-107`):

- `Is_DescendantOf` walks `Get_LifetimeOwner` upward, depth-capped, stopping at the transient
  entity (which has no owner — walking past it ensures).
- Provide a `_Strict` variant that rejects non-descendants; tests pass strict so a stale
  still-Ready sibling-test driver can't resolve their promise with the wrong handle.
- The flush processor resolves through the **ticket's lifetime owner**, not the ticket entity,
  so the retry path scopes identically to the original call.
- Singleton tags are **opt-in per instance**, not baked into `Add`: BB's DayCycle stamps
  `TAG_BbDayCycle_Global` only via an explicit `Mark_AsGlobal` "so gym/test entities don't crowd
  out each other under one shared tag" (`BB_DayCycle_Utils.as:24-25`, verified). Give your
  singleton drivers the same split: a feature tag stamped by `Add`, a global/instance tag stamped
  deliberately by the production spawner.

## 4. Discovery done right — the mechanics

Discovery/composition timing is the #1 failure mode the maintainer flags (ruling 4, 2026-07-03).
**ck-game-feature-recipe owns the canonical warning and the producer-side rule (stamp the
discovery tag LAST, after the feature finishes composing).** This section owns the consumer
(driver) side. The StoreDriver is the reference implementation
(`BB_StoreDriver_EntityScript.as:1-21` header, whole file verified 2026-07-03).

### 4.1 Never construct-time-scan and stop

A one-shot scan in `DoConstruct` misses everything that composes later: async
EntitySpawnParams placers, runtime-placed fixtures, entities whose tag lands a frame after
yours. Corpus fix commits for exactly this: `66ac804db` (defer-rescan so late-composing entities
still bind), `1ca589b0d` (async-composed gondolas). The verified two-axis model:

- **Axis 1 — always-on wall-clock window** (`DiscoveryTimeoutSeconds`, production default 5.0s;
  tests pass ~0.5s; `<= 0` collapses to immediate-ready for degenerate scenes). Run the sync
  bind sweep at `DoConstruct` **and again at window close** to catch entities tagged during the
  window.
- **Axis 2 — optional minimums contract** via `CkEntityTagQuery` (`MinExpected<Category>` /
  `Require<Thing>`): the query fast-paths Ready the moment requirements are met (cancelling the
  timer); if the window closes unmet, emit per-category warnings, stamp a diagnostic tag, and
  **still mark Ready so consumers don't hang** (`BB_StoreDriver_EntityScript.as:3-11, 719-745`).

Categories split into **readiness-gating essentials** (things the scope cannot function without)
vs **fixtures** discovered opportunistically forever and never gating Ready (StoreDriver:
managers/signs gate; gondolas/entryways/counters are fixtures with a persistent runtime watcher —
`:462-475`). Decide the split up front; changing it later strands dead `MinExpected*` fields kept
only for `::Params()` positional stability (`:146-181` — a real scar, see
ck-game-entity-composition-patterns on ExposeOnSpawn append-only discipline).

### 4.2 Per-category binders: dedupe, validate, no signal binding

Every discovery path (sync scan, query fire, runtime watcher, explicit refs) funnels into one
idempotent `BindOne_<Category>` that dedupes against a Dependencies fragment
(`:862-873`):

```angelscript
private void BindOne_Widget(FCk_Handle& InContext, FCk_Handle_Widget& InWidget)
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }
    auto& Deps = InContext.AddOrGet_Fragment(F<Prefix>_Fragment_MyDriver_Dependencies);
    if (Deps.Widgets.Contains(InWidget))
    { return; }
    Deps.Widgets.Add(InWidget);
}
```

- **Validate the tag→feature contract on every scan hit** and name the producer-side failure in
  the message: `ck::EnsureIfNot(ck::IsValid(Widget), "...tagged but not a valid Widget — tag
  stamped before the feature finished composing?")` (`:856-858` and six siblings, verified).
  This ensure is your early-warning system for producer features violating the tag-last rule.
- **Explicit ExposeOnSpawn refs and AutoDiscover are mutually exclusive per category** — refs
  win and skip the scan (`:841-860`). Cardinality-1 categories are first-wins with a warning on
  extras telling the designer to pass an explicit ref (`:1046-1072`).
- **COLLECT and BIND are separate phases**: discovery only fills the Dependencies fragment;
  signal binding happens once, in the Ready-state tasks, after the set is final (`:837-840`).

### 4.3 Delta-gate every persistent query handler

`EntityTagQuery` **OnContinuousUpdate fires every Evaluate pass — every frame — by design** (the
query processor is not dirty-gated; tested framework contract, see ckecs-architecture-contract).
And **All-mode OnSatisfied re-delivers the full match set** on each fire. Two consequences,
both with corpus incidents:

1. A handler that rebuilds/diffs its tracked set unconditionally costs real frame time —
   BusterBlock's three driver discovery handlers combined for **~250ms/frame** before commit
   `531b0c956` (verified diff). The fix pattern:

```angelscript
private bool _HasReconciled = false;

UFUNCTION()
private void OnPopulationChanged(FCk_Handle_EntityTagQuery InQuery, bool InIsSatisfied,
                                 const TArray<FCk_EntityTagQuery_Result>&in InResults)
{
    if (InResults.Num() == 0)
    { return; }
    // Skip no-delta frames AFTER a guaranteed first reconcile — the bind-time
    // FireIfPayloadInFlight pass carries the full population with EMPTY deltas,
    // so gating on deltas alone would drop the initial seed.
    if (_HasReconciled
        && InResults[0]._Added.Num() == 0 && InResults[0]._Removed.Num() == 0)
    { return; }
    _HasReconciled = true;
    // ... reconcile from InResults[0]._Handles (full set), dedupe via your Bound set ...
}
```

2. Even with the gate, keep the reconcile itself idempotent (the `BindOne_` dedupe) — re-delivery
   of the full set must be harmless (`:697-715`).

### 4.4 SCOPE the discovery — tag scans are registry-wide

`utils_entity_tag::ForEach_Entity` sees **every** tagged entity in the world. On any map with
more than one instance of your scope, an unscoped scan claims foreign entities. Corpus incident
(commit `287ee6601` + `Prune_ForeignEntryways`, `BB_StoreDriver_EntityScript.as:621-695`,
verified): the StoreDriver's registry-wide entryway scan bound *every* building's doors — 94% of
customer "entered" occupancy adds came from foreign doors, and NPCs were lured to the wrong
building. Scoping mechanisms, in preference order:

1. **Explicit refs** (ExposeOnSpawn) — the designer names exactly which instances belong.
2. **Lifetime-descendant filtering** — if the driver spawns/owns the entities, scope by
   `Is_DescendantOf` (this is what makes multi-store and concurrent tests work, §3).
3. **Spatial pruning from an INTERIOR anchor** — for level-placed entities with no ownership
   link, prune anything beyond a radius measured from an anchor *deep inside* the scope
   (StoreDriver: centroid of its store managers; anchoring on the front sign forced the radius
   up into foreign-door range — `:626-635`). Fail **open** (keep all) when no anchor resolves.
4. **Per-scope tags** — a distinct tag per instance; heavier authoring cost, use when 1–3 don't fit.

Run the prune once, after discovery closes and **before** Ready is stamped.

