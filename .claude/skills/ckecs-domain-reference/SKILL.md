---
name: ckecs-domain-reference
description: "Use when reasoning about EnTT and CkEcs internals: registry storage, views, entity versions, processor pumps, destruction, actor bridges, transient roots, or fragment GC."
---

# CkEcs Domain Reference — the ECS mental model

Theory pack for CkFoundation's ECS: what EnTT (the vendored C++ ECS library, **v3.16.0**,
`Source/CkThirdParty/Public/CkThirdParty/entt-3.16.0/`) actually does, how CkEcs wraps it, and how
entity lifetime maps onto UE worlds, actors, and the garbage collector. Every claim below cites the
vendored source or CkEcs source (file:line, verified 2026-07-02). Lingo (Fragment = component,
Processor = system, Handle, Request, Signal): root `CLAUDE.md` — this file assumes those words.

## When NOT to use this skill

| You want | Load instead |
|---|---|
| WHY the architecture is shaped this way; invariants, signal lifecycle, typesafe-handle contract | `ckecs-architecture-contract` |
| Mechanical how-to: add a fragment/processor/request/handle; macro expansions | `ck-macros-and-codegen` |
| Debug a build/UHT/link failure | `ck-debugging-playbook` |
| Incident history (what broke before) | `ck-failure-archaeology` |

## Layer map

```
UWorld
 └─ UCk_EcsWorld_Subsystem_UE              owns TUniquePtr<entt registry> + slot registration
     ├─ ck::registry_table                 (slot, generation) → entt registry*   [global table]
     ├─ ACk_EcsWorld_Actor_UE (per ETickingGroup)  ticks one FProcessorScheduler
     └─ TransientEntity                    per-registry root entity (world fragment lives here)
entt::basic_registry<FCk_Entity::IdType>   one sparse-set storage pool PER FRAGMENT TYPE
FCk_Registry                               trivially-copyable VIEW = FCk_RegistryHandle (slot+gen)
FCk_Handle                                 FCk_Entity (index+version) + FCk_RegistryHandle
FCk_Handle_[Feature]                       same bytes, typed; minted only via ck::StaticCast
```


---

## Reference files — load only what the question needs

Section numbers cited elsewhere (`§1.3`, `§2.4`, `§3.4`, …) point into these files.

| The question is about | Read |
|---|---|
| EnTT itself: entity index+version, sparse-set pools, deletion policies and tombstones, empty types, `storage_type` specialization, views, groups | `references/entt-internals.md` (§1) |
| The CkEcs wrapper: `FCk_Registry` as a generational view, what makes an `FCk_Handle` valid, the `TProcessor` CRTP, `MarkedDirtyBy`, registration and the real tick flow | `references/ckecs-wrapping.md` (§2) |
| Lifetime against UE: world ↔ registry, the `TransientEntity` root, the actor ↔ entity bridge, the deferred leaf-first destroy flow | `references/ue-lifetime.md` (§3) |

The short, high-traffic material — UHT limitations (§4), GC interaction rules (§5), and Common mistakes — stays below.

---

## 4. UHT limitations that shaped the API

UHT (UnrealHeaderTool, the reflection codegen) parses UFUNCTION/USTRUCT declarations with a
restricted grammar. Each restriction below produced a house pattern (style itself: root
`CLAUDE.md`):

| UHT limitation | Ck pattern it produced | Evidence |
|---|---|---|
| No UFUNCTION overloads (one reflected name each) | suffix vocabulary: `_ByName`, `_ByTag`, `_Simple`, `AddOrReplace`, `TryGet_*`; a plain C++ overload MAY shadow a UFUNCTION name | root CLAUDE.md Naming; e.g. `Get_EntityOwningActor` vs `TryGet_EntityOwningActor` (`CkOwningActor_Utils.h:57-71`) |
| No trailing return types on UFUNCTION declarations | split declaration: concrete return type on its own line, `static FCk_Handle\nRequest_CreateEntity(...)`; trailing returns everywhere else | `CkEntityLifetime_Utils.h:67-69`; root CLAUDE.md function shapes |
| Historically no `TOptional` in reflected surfaces | enum-mode + value pair (`ECk_...` + payload). UE 5.5+ UHT now accepts TOptional UPROPERTYs and the 3 newest files use it — open fork, see ADJUDICATIONS **A1** before copying either way | `.claude/reports/ADJUDICATIONS.md` A1 |
| UFUNCTION parameter defaults must be `()`-constructible expressions, not `{}` | `{}` construction everywhere EXCEPT UFUNCTION param defaults, which use `()`; no `= {}` in UFUNCTION signatures | root CLAUDE.md; live: `FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f)` (`CkPmg/CkPmg_Utils_SymbolShapes.h:28`) |
| UHT owns UObject construction (GENERATED_BODY declares the ctor path; CDO factory) | `CK_DEFINE_CONSTRUCTORS` on value structs ONLY, never UObjects; UObjects configure via UPROPERTY defaults | root CLAUDE.md macro table; mechanism: `ck-macros-and-codegen` |

---

## 5. GC interaction rules

**The one-sentence law: UE's garbage collector traces UPROPERTY graphs only — EnTT fragment
members are INVISIBLE to it.** A fragment is a plain C++ struct living in an EnTT storage page;
no reflection, no `AddReferencedObjects`. A `UPROPERTY()` inside a USTRUCT-shaped fragment does
NOT help — nothing walks it once the struct lives in a pool. A UObject whose only reference is a
fragment member **will be collected** mid-play. This is not theoretical: the replication-driver
UObject was collected ~1.2s after possession whenever no netdriver happened to also reference it,
cascading into PlayerController destruction (fix `56b344310`; a follow-up audit swept 82 fragment
UObject fields). Full incident: `ck-failure-archaeology`.

**Ownership split (the rule that follows):**

- `TStrongObjectPtr<T>` — the entity OWNS the object (spawned component, render target, driver):
  roots it against GC; the feature's EndPlay/Destructor processor must Reset/destroy it.
- `TWeakObjectPtr<T>` — observation only: auto-nulls when the owner-of-record collects it; always
  re-validate with `ck::IsValid` before use.
- `TObjectPtr<T>` — only in real reflected contexts (UPROPERTY on a UClass/USTRUCT that UE
  actually traverses); inside fragments it is GC-invisible decoration.

**The audit question for ANY `UObject*`-family member in a fragment: "who roots this?"** Acceptable
answers: this fragment via TStrongObjectPtr; a UPROPERTY on a live outer (subsystem, actor,
component); the engine (asset in memory, rooted CDO). "The net connection replicator chain
happened to reference it" is how the incident above shipped — a masking reference, not ownership.

**"Disregard for GC" pool (one paragraph):** UE stamps every UObject created before engine init
completes into a permanent pool the GC never traverses (`gUObjectArray`'s disregard-for-GC set) —
they are immortal, AND their outbound references are never traced. In this codebase, AngelScript
`asset ... of ...` owners and AS CDOs are created during AS InitialCompile, *before* that set
closes. When post-boundary code attaches normal-pool objects under such an owner, the first real
GC collects the children out from under the untraversed parent → dangling pointer → packaged-only
0xC0000005 (PIE never exercises this timing; cooked Development client does). Root fix: a pre-GC
delegate feeds AS disregard objects through `CollectReferences` and `AddToRoot`s unrooted targets
(`feb08ee94`, diagnostics `Ck.Diag.VerifyGCAssumptions` / `Ck.Diag.DumpAngelscriptAssets`). The
boundary lesson: an object created during AS InitialCompile has DIFFERENT GC semantics than the
identical object created one frame later. Incident detail: `ck-failure-archaeology`.

---

## Common mistakes

1. **Holding `FCk_Entity` instead of `FCk_Handle`.** A raw entity has no registry and no staleness
   protection beyond its embedded version; store handles (typed where possible), re-check
   `ck::IsValid` at use time.
2. **Caching a fragment reference across mutations.** In this codebase addresses are stable
   (paged storage §1.2 + in_place pools §1.3), so the ref only dies when THAT entity's fragment is
   removed — but it dies silently: the object is destroyed in place and the slot may later be
   refilled for an unrelated entity. Re-`Get` per tick; never cache across frames.
3. **Structural mutation of OTHER entities inside `ForEachEntity`.** Violates the view
   invalidation contract (§1.6). Queue a request fragment, use `FDeferredCommandBuffer`, or tag
   and let another processor act.
4. **`MarkedDirtyBy` without consuming the marker.** The scheduler re-pumps you to the 30-pass cap
   and logs your processor as still-dirty every frame (§2.4). Symmetrically: a time-integrating
   processor without `PumpPolicy = SkipPump` re-applies work at DeltaT=0.
5. **Treating `Get_RegistryView()` / `operator*` results as long-lived references.** Returned by
   value; binding to a reference is a dangling temporary (`CkHandle.h:251-262`).
6. **Assuming default-invalid == destroyed.** During the destroy pipeline the entity still exists
   and EndPlay/Teardown processors still run; use `IncludePendingKill` /
   `CK_IF_HANDLE_IS_PENDING_KILL` when cleanup code must address it (§2.2, §3.4).
7. **Expecting the first View type to drive iteration.** The smallest pool leads (§1.6); don't
   "optimize" by reordering template arguments.
8. **A UObject reference parked in a fragment with no root.** It will vanish at an arbitrary GC;
   ask "who roots this?" (§5).
9. **Sorting a pool with pending tombstones.** `FCk_Registry::Sort` asserts "Sorting with
   tombstones not allowed" if the pool has un-reused in-place holes (§1.3); non-movable fragments
   additionally trip the pinned-type assert in swap paths (`storage.hpp:318-321`).

---

## Provenance and maintenance

Campaign date **2026-07-02**; verified against submodule HEAD `7330c1bab`, EnTT vendored 3.16.0.
Re-verification (PowerShell/Git Bash, cwd `d:\Repos\BusterBlock\Plugins\CkFoundation`):

- EnTT version: `ls Source/CkThirdParty/Public/CkThirdParty/ | grep entt` (expect `entt-3.16.0`);
  masks in `.../src/entt/entity/entity.hpp:31-40`.
- Cited EnTT internals move between minor versions — on an EnTT bump re-check:
  `deletion_policy` (`entity/fwd.hpp:17-26`), `in_place_delete` trait (`entity/component.hpp:14-22`),
  view invalidation doc (`entity/view.hpp:203-219`), smallest-pool pick (`view.hpp:244-254`),
  group conflict assert (`entity/registry.hpp:1119`).
- Groups/sigh still unused: `rg -n "\.group<|on_construct\(\)" Source -g '!*ThirdParty*'` (expect 0).
- Global in_place specialization still present: `rg -n "struct entt::component_traits" Source/CkEcs`
  (expect `CkHandle.h:72`); per-type members: `rg -n "in_place_delete" Source -g '!*ThirdParty*'`
  (expect CkSignal_Fragment.h ×2, CkHandle_Debugging.h, CkHandle.h).
- Slot table shape: `rg -n "kRegistryTable_MaxSlots|EnttRegistryType" Source/CkEcs/Public/CkEcs/Registry/CkRegistry_SlotTable.h`.
- Handle validity ladder: `rg -n "IsValid_Policy_IncludePendingKill\) const" Source/CkEcs/Public/CkEcs/Handle/CkHandle.cpp`.
- Processor CRTP: `rg -n "namespace ck_exp" Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h`
  (expect :237, :342); marker consumption `rg -n "Remove<MarkedDirtyBy>" Source/CkTimer`.
- Tick flow: `rg -n "_Scheduler->Tick|DoBuildGraphAndSpawnActors" Source/CkEcs/Public/CkEcs/Subsystem/CkEcsWorld_Subsystem.cpp`;
  pump cap `rg -n "_MaxPumpIterations" Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorScheduler.h`.
- Destroy pipeline tags: `rg -n "CK_IGNORE_PENDING_KILL" Source/CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h`.
- ProcessorScript subsystem still dormant:
  `rg -ln "Request_CreateNewProcessorScript" Source -g '!**/CkProcessorScript_Subsystem.*'` (expect 0
  — a single `*` glob does not cross `/`, so the old `!*CkEcs/...` form excluded nothing).
- A1 status (TOptional): `.claude/reports/ADJUDICATIONS.md` — re-read before teaching optionality.
- GC incident SHAs: `git log --oneline --no-walk feb08ee94 56b344310` (both exist in this repo's history).
- Tooling caveat: Grep/Glob tools are blind under `Script/`, `docs/`, `Content/` here — use
  `rg --no-ignore` (root CLAUDE.md provenance notes).
