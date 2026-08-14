# CkGoap WorldState Parent-Fallback (Import-Aliasing) — Design

**Status:** revised 2026-08-13 per CTO review ([review + response](../reviews/2026-08-13-CkGoap-WsParentFallback-CTO-review.md), verdict CHANGES REQUESTED, commit 56cd98692). All three sign-off conditions folded in: §3.2/§6 rewritten around synchronous pre-registration (the eager-resolution finding), §3.9 dead-parent contract added, §7 gains the adversarial-ordering and parent-teardown tests. No implementation yet.

**Campaign context:** [CONTINUATION_PROMPT_WsKeySubScoping.md](../../Source/CkGoap/CONTINUATION_PROMPT_WsKeySubScoping.md) — BusterBlock keeps the GOAP WS key cap at 64 and moves ~33 of its ~61 `Npc.WS.*` keys off the shared per-NPC world state onto sub-planner-scoped world states. This design is the framework prerequisite: a sub-WS that doesn't own a key defers lookup, writes, and residency classification to its parent WS.

---

## 1. Problem

- Every WS entity has a per-entity `FKeyRegistry` capped at `WorldState_MaxKeys = 64` (`Source/CkGoap/Public/CkGoap/Algorithm/CkGoap_WorldState.h:18`). At capacity, `FindOrRegister` silently returns `InvalidGoapKey` (`:329`), unresolvable preconditions/effects are silently dropped from the cached action def (`CkGoap_Action_Processor.cpp:192,199`), and writes no-op at Verbose (`CkGoap_WorldState_Processor.cpp:79`).
- BusterBlock's NPC catalog is at ~61 keys on one shared WS. Raising the cap was tried (64→128), disproven as the cause of the observed test failures, and reverted by the maintainer — the cap stays.
- Sub-planners already resolve exactly ONE WS source (`FFragment_Goap_Planner_WorldStateSource`, resolved at Add time for top-level, at activation-walk time for promoted — `CkGoap_Planner_Utils.cpp:668-710`). If a sub-planner gets its own WS today, any shared key its actions reference (`IsStuck` on the shopping walk legs) registers in the sub-WS and **desyncs** from the shared value.

## 2. Why import-aliasing (the shape constraint)

A sub-planner's cached `FActionDef`s, the A* `FConstraintSet`, and `FWorldState` are all fixed 64-slot arrays indexed by **one** registry (`CkGoap_WorldState.h:142,305`; O(1) memcmp/CRC identity is the point of the design). A shared key resolved to a *parent* index and a local key to a *sub* index cannot coexist in one search state. Pure "defer the lookup" therefore doesn't compose with the planner. Rejected alternatives:

- **Tag-space search for shared keys** — destroys the fixed-array/A* design. Rejected.
- **(level, index) pair keys in cached defs** — doubles every search-state structure. Rejected.
- **Hoist shared gates to the composite planner** — changes `IsStuck` granularity: today a stuck shopper re-routes WITHIN the trip (`WaitForShelf`/`GiveUp` arbitration); composite-level gating would drop the whole trip. Named design cost in the campaign doc; rejected.
- **Sync-pump mirroring parent keys into sub override layers** — a per-change copy pump with its own ordering bugs. Rejected.

**Chosen rule: a parent-resident key gets a local index in the sub-registry too, flagged as an *import*. The parent owns the truth; the local slot is a per-plan snapshot alias.**

## 3. Mechanics

1. **Parent link.** New fragment on the sub-WS entity:
   `FFragment_Goap_WorldState_ParentLink { FCk_Handle_Goap_WorldState _Parent; TSet<FGameplayTag> _ImportedTags; }`
   Created via an optional parent argument on `UCk_Utils_Goap_WorldState_UE::Create` (the named-child API, `CkGoap_WorldState_Utils.cpp:79`). Chain walks are depth-capped (~8) with a cycle guard; BB uses exactly one level (shared `Npc.AI.WS` ← sub-WS).

2. **Residency classification** — in `FProcessor_Goap_Action_Setup`'s key-registration loop (`CkGoap_Action_Processor.cpp:160-161`), per key:
   - local `Find` hit → done (already local or already imported);
   - else parent-chain `Find` hit → register locally **and** add to `_ImportedTags`;
   - else `FindOrRegister` locally (today's behavior — a genuinely new local key).

   **Setup ordering gives no guarantee here.** WS-source resolution is *eager by design* — `CkGoap_Planner_Internal.h:19-20`: "Eager, so the child's Setup can run before any parent plan is requested"; `DoResolveChildWorldStateFromParent` runs at AddAction time for both promoted hosts (`CkGoap_Planner_Utils.cpp:748`) and top-level actions (`:756`), and a sub-planner carrying `_WorldStateSource_Override` (BB's intended wiring) resolves immediately. Every action in the tree therefore clears the setup gate (`CkGoap_Action_Processor.cpp:139`) in the first setup pass after construction, with arbitrary iteration order between top-level and sub-action setups. The activation-time resolver (`CkGoap_Planner_Processor.cpp:395-427`) is a second-chance fallback, not the primary path. Classification correctness is guaranteed *by construction* instead: shared keys are **synchronously pre-registered at WS composition** (§6), so they are resident in the parent registry before any setup pass can run.

3. **Reads** (`Get_Value`, `CkGoap_WorldState_Utils.cpp:124-154`): own override stack → own registry; a non-imported hit reads own `Values`; imported or miss → parent's *effective* value (parent overrides included, reusing `DoGetEffectiveValue`). Miss at chain root → `false` (unchanged contract).

4. **Writes** (`SetValue` drain, `CkGoap_WorldState_Processor.cpp:64-109`): local non-imported → local (unchanged). Imported, or miss-locally-but-found-in-ancestor → **apply directly to the owning ancestor**: its `Values`, its `ChangeLog`, `OnValueChanged` broadcast on *its* handle, *its* subscribers dirtied. Same drain pass, so latency is identical to writing the parent handle directly — no extra frame. Miss everywhere → `FindOrRegister` locally (unchanged). Cross-entity mutation inside the drain is safe: the view iterates `Requests` fragments; the forwarded write touches only the ancestor's `Values`/`ChangeLog`/`Subscribers`, which the utils already mutate cross-entity today (`DoTagSubscribersDirty`).

5. **Planner plan snapshot** — in `FProcessor_Goap_Planner_HandleRequests`'s Plan branch (`CkGoap_Action_Processor.cpp:427-447`): copy own `Values` → overwrite each imported tag's local slot with the parent's effective value → flatten own override stack (existing code unchanged; imported tags resolve via local `Find`). The A* inner loop, `FGoapGraph`, and heuristics are untouched.

6. **Dirty propagation** — a planner subscribes to **every WS in its resolved chain**, at both existing subscribe sites: top-level `Add` (`CkGoap_Planner_Utils.cpp:139-144`) and the activation-time subscribe (`CkGoap_Planner_Processor.cpp:436-441`). A shared-gate flip (`IsStuck`) then dirties the sub-planner exactly as today — preserving within-trip re-routing. **Unsubscribe walks the same chain**: `DoUnsubscribeActionFromWorldState` (`CkGoap_Planner_Processor.cpp:447-455`) becomes chain-aware in the same pass, else a deactivated sub-planner stays subscribed to the parent and every shared-gate flip generates spurious replan pressure.

7. **Goal resolution** — unchanged (`SetGoal` uses local read-only `Find`, `CkGoap_Action_Processor.cpp:540-555`). A parent-resident key that no local action affects stays an `_InvalidGoal` diagnostic — consistent with the existing "goal keys must be affectable by the catalog" contract.

8. **Capacity** — the per-registry at-capacity Warning (`CkGoap_Action_Processor.cpp:169-174`) is unchanged and applies per sub-registry. Imports consume sub-registry slots: worst BB case ShoppingTrip ≈ 16 local + ~5 imports ≈ 21/64. The shared WS drops from ~61 to ~28.

9. **Dead-parent contract.** When `_Parent` no longer resolves (parent WS entity destroyed before the sub-WS), imported keys degrade to **miss semantics**: `Get_Value` returns `false` (the miss-at-chain-root contract), the plan-snapshot merge writes `false` into imported slots (reads and A* must agree), and forwarded writes are **dropped** with the same Verbose diagnostic class as the registry-full drop (`CkGoap_WorldState_Processor.cpp:77-79`). The stale alias slot is never served as truth. *Deviation from the review's parenthetical recommendation ("writes fall through to local"), for coherence: a write that lands locally while reads return `false` is a write that can never be read back — dropping it with a diagnostic is the honest failure. The hard requirements (defined contract, no stale-slot reads, teardown AutoTest) are met either way.*

## 4. Decisions taken (vetoable at review)

1. **Direct-apply write forwarding** over re-enqueueing the request on the parent WS: zero added latency, no request ping-pong, single dirty/broadcast site (the owner).
2. **Sub-WS overrides may shadow imported keys** (override stack is tag-keyed and checked first): a sub-planner gets hypothetical-local override of a shared gate for free; consistent with existing override semantics.
3. **Unresolved-anywhere writes still register locally.** Keeps today's write contract; the residual hazard is closed by synchronous pre-registration at composition (§6).
4. **Goal `Find` stays local-only** (no chain fallback): a goal on a parent key the local catalog can't affect is unreachable anyway and belongs in `_InvalidGoal`.

## 5. Named gaps (accepted for this pass)

- **`ReplanCause` evidence**: a sub-planner's replan-cause record reads only its own WS ChangeLog (`CkGoap_Action_Processor.cpp:384-395`); parent-key changes won't appear in its coalescing evidence. Diagnostic-only; folds into the per-sub-WS GOAP debugger view already flagged as follow-up in the campaign doc.
- **Signals**: `OnValueChanged` for a shared key fires on the owning (parent) handle only. Game-side binds must target the owning WS.
- **Save/load**: whether CkGoap WS values persist via a snapshot handler is unverified; to check before implementing. Registries and `_ImportedTags` rebuild at action setup, truth lives in the parent, and `ParentLink` is composition (re-established by `DoConstruct` replay), so import-aliasing survives a rebuild+hydrate load; `_Parent` gets the standard restored-handle tolerance treatment.
- **Cross-handle same-frame write ordering**: today all writes to one WS land in one `Requests` fragment and drain FIFO. With forwarding, a same-frame write to the same shared key via the child handle and via the parent handle resolves in entity-iteration order, not enqueue order — last-wins is nondeterministic *across handles* (converges cross-frame). Convention: **one writer handle per key per frame.** BB's classification honors this — sensors write shared keys via the shared handle only.

## 6. Residency guarantee: synchronous pre-registration at WS composition

Because setup ordering guarantees nothing (§3.2), residency classification needs an anchor that exists **before any setup pass can run**. The framework cannot know a game's shared-key set — game-side declaration is the right division of labor — but the declaration mechanism must be ordering-proof, and a deferred `Request_RegisterKey` is not (whether its drain precedes the first `FProcessor_Goap_Action_Setup` pass for a same-frame-constructed NPC is an unpinned scheduler property).

**Mechanism:** `FCk_Fragment_Goap_WorldState_ParamsData` (empty today; `DoStampWorldStateFragments` at `CkGoap_WorldState_Utils.cpp:48-49` awaits its first knob) gains a `_PreRegisteredKeys` tag array, registered **synchronously inside `Add`/`Create`** at composition time. This does not violate the locked "`Set_Value` stays deferred" decision: key registration is monotonic and value-free, and action setup already registers synchronously via `Get_MutableRegistry().FindOrRegister` (`CkGoap_Action_Processor.cpp:146-147,160-161`). With it, classification is airtight by construction: the shared gates are resident in the parent registry from the instant the parent WS exists, which is necessarily before any sub-WS (created later, parented to it) or any action setup.

BusterBlock's ~28 shared-gate keys move into the shared WS's creation params — no construction-time request loop.

The framework additionally logs (Verbose, not Warning — this is the *normal* path for all ~33 sub-local keys, and the AutoTest harness escalates Warnings) when a sub-WS with a parent registers a brand-new local key, so a misclassification is findable. A cold-path desync audit (console command, folded into the deferred per-sub-WS debugger view) walks parented WS entities and flags any tag local-non-imported in a child AND resident in an ancestor — the true detector, at zero hot-path cost; `Create`'s existing `FRecordOfGoapWorldStates` child record (`CkGoap_WorldState_Utils.cpp:103-105`) provides the reverse walk.

## 7. Framework tests (AS AutoTests, `Plugins/CkTests/Script/CkGoap/`, patterned on `CkAutoTest_Goap_Planner_WSInheritance` / `_WSOverride`)

1. **Classification**: action referencing one parent-resident + one new key → import vs local residency observable per-handle.
2. **Read-through**: parent value visible via sub handle.
3. **Write-through**: `Set_Value` via sub handle lands in parent; no local shadow (parent read AND sub read agree after settle) — and the alias slot never leaks into `Get_Value`: after a plan-snapshot merge, a subsequent parent-side change is still what the sub handle reads.
4. **Plan flip**: sub-planner precondition on a parent key changes plan outcome when the parent value flips.
5. **Dirty**: parent key change triggers sub-planner replan (AutoReplan fires).
6. **Sub-override shadows** an imported key in the plan snapshot.
7. **Unresolved-anywhere write** still registers locally.
8. **Two sub-planners** under one parent see one truth (no divergence).
9. **Adversarial ordering**: sub-action references a shared-gate tag with pre-registration deliberately omitted → local (mis)classification is observable and the Verbose diagnostic fires; repeat with `_PreRegisteredKeys` populated → import. Pins the §6 hazard as a regression test.
10. **Parent teardown**: destroy the parent WS before the sub-WS → imported reads return `false`, forwarded writes drop with the Verbose diagnostic, plan snapshot agrees with reads (§3.9 contract).

All WS asserts sit behind a settle (`Set_Value` is a deferred request — the exact trap that misdiagnosed this campaign's first hypothesis).

## 8. Implementation order

1. **Unify the twin WS-source resolvers first**: `DoResolveChildWorldStateFromParent` (`CkGoap_Planner_Utils.cpp:668-710`) and `DoResolveAndAssignWorldStateSource` (`CkGoap_Planner_Processor.cpp:395-427`) are near-duplicates; any chain-aware change must land in both or they drift — fold into one shared internal before touching either.
2. `_PreRegisteredKeys` on `FCk_Fragment_Goap_WorldState_ParamsData`, registered synchronously in `Add`/`Create`.
3. `ParentLink` fragment (friend spelling: global-scope-qualified `friend class ::UCk_Utils_Goap_WorldState_UE;` per the header's own comment) + chain-aware utils (`Get_Value`, `Has_Key_InChain`, internal owning-WS resolver, `Create` parent arg). Chain walker depth-capped with a **`CK_ENSURE` on cycle** (a ParentLink cycle is authoring error, not a boundary condition — walker idiom: `Goap_Planner_GetParentPlanner`, `CkGoap_Action_Processor.cpp:109-118`).
4. `SetValue` drain routing (write forwarding + §3.9 dead-parent drop). Invariant: **a forwarded write never touches an ancestor's `Requests` fragment** — direct-apply only.
5. Action-setup residency classification (import-aliasing).
6. Planner snapshot merge of imported values (incl. §3.9 dead-parent `false`).
7. Chain subscribe AND unsubscribe at all sites (§3.6).
8. The 10 framework tests; gate `--test` on the Goap pattern.
9. **BB migration, pilot first**: Brainwash (2 keys, one writer file, existing autotest) → then ShoppingTrip (16 keys, many writers) → remaining sub-planners per the classification table in the campaign doc §3. Shared-gate `_PreRegisteredKeys` list lands with the pilot.

### Files touched (framework pass)

- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h` — `_PreRegisteredKeys` on `FCk_Fragment_Goap_WorldState_ParamsData`
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Fragment.h` — `FFragment_Goap_WorldState_ParentLink`
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Utils.{h,cpp}` — Create parent arg, synchronous pre-registration, chain-aware Get/Has, owning-WS resolver
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Processor.cpp` — drain routing
- `Source/CkGoap/Public/CkGoap/Action/CkGoap_Action_Processor.cpp` — setup classification + snapshot merge
- `Source/CkGoap/Public/CkGoap/Planner/CkGoap_Planner_Utils.cpp`, `CkGoap_Planner_Processor.cpp`, `CkGoap_Planner_Internal.h` — resolver unification, chain subscribe/unsubscribe
- `Plugins/CkTests/Script/CkGoap/` — 10 new AutoTests
