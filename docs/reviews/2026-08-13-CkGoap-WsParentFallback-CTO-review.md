# CkGoap WS Parent-Fallback (Import-Aliasing) — CTO Design Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the bottom of this file. Commit your changes — the design author's assistant will pick up your notes from there.

> **Pre-implementation review.** No code has been written. The artifact under review is the **design spec** for a CkGoap framework change — parent-WS fallback lookup — that is the prerequisite for BusterBlock's WS key sub-scoping migration. Green-light means implementation starts (framework first, then the BB pilot); a blocker means the spec gets revised first.

---

## Reviewer brief

### Your role

Senior reviewer / architect. You are reviewing a proposed CkGoap extension: **world-state entities gain an optional parent, and keys a sub-WS doesn't own resolve, read, and write through to the parent — via local "import aliases" so each planner keeps a single 64-slot index space.** Specifically:

1. Catch architectural issues that would be expensive to discover mid-migration — this touches the A* snapshot path, the request drain, and action-setup key registration, and ~33 BusterBlock keys will migrate on top of it.
2. Judge whether import-aliasing is the right shape versus the rejected alternatives (spec §2), given the fixed-array A* design it must preserve.
3. Scrutinize the determinism argument for residency classification (spec §3.2 + §6) — the one place a silent desync could re-enter.
4. Either green-light for implementation, or list specific blocking concerns.

You are expected to **read code in the repo**, not just review the spec in isolation. The spec cites file:line for every load-bearing claim; spot-check the ones in section C below.

### What's being built

BusterBlock's per-NPC GOAP world state is at ~61 of 64 registry keys. The cap stays at 64 (raising it to 128 was tried, disproven as the cause of the observed failures, and reverted by the maintainer). Instead, ~33 sub-planner-local keys (AtShelf, AtKiosk, …) move onto per-sub-planner world states, while ~28 shared gates (`IsStuck`, `IsThreatened`, the `WantsTo*` bridges, …) stay on the shared WS. The framework prerequisite: without parent-fallback, a sub-planner action referencing `IsStuck` would register/read it in the sub-WS and silently desync from the shared value. Named design cost of the alternative (hoisting shared gates to the composite planner): a stuck shopper would abandon the whole shopping trip instead of re-routing within it.

### Design spec location

[2026-08-13-CkGoap-WsParentFallback-design.md](../specs/2026-08-13-CkGoap-WsParentFallback-design.md)

### Critical context — read before reviewing

- **Campaign doc** (repo state, key classification table, ruled-out hypotheses): [Source/CkGoap/CONTINUATION_PROMPT_WsKeySubScoping.md](../../Source/CkGoap/CONTINUATION_PROMPT_WsKeySubScoping.md)
- `Source/CkGoap/Public/CkGoap/Algorithm/CkGoap_WorldState.h` — `WorldState_MaxKeys`, `FWorldState`, `FConstraintSet`, `FKeyRegistry`; the fixed-array O(1) design the change must not break.
- `Source/CkGoap/Public/CkGoap/Action/CkGoap_Action_Processor.cpp` — action setup (key registration :160, at-capacity warning :169, silent drops :192/:199) and the planner Plan branch (WS snapshot + override flatten :427-447).
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Utils.cpp` — `Add`/`Create`/`Find`, deferred `Set_Value`, `Get_Value`, override-stack effective reads.
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Processor.cpp` — the SetValue request drain the write-forwarding lands in.
- `Source/CkGoap/Public/CkGoap/Planner/CkGoap_Planner_Utils.cpp:668-710` — `DoResolveChildWorldStateFromParent`, the existing WS-source resolution the parent link composes with.
- `Plugins/CkFoundation/CLAUDE.md` + `Source/CLAUDE.md` — doctrine of record (fragment/friend conventions, `Get_`/`Request_` API shape, request/completion-guard idiom).
- Existing test shapes to be extended: `Plugins/CkTests/Script/CkGoap/CkAutoTest_Goap_Planner_WSInheritance.as`, `_WSOverride.as`, `_WSOverrideStack_BasicPushPop.as`.

Engine/env: UnrealEngine-Angelscript 5.7.x, EnTT 3.16.0 vendored (verified 2026-07-02 per root CLAUDE.md).

### Design decisions already locked in (do NOT relitigate unless you see a real problem)

1. **The cap stays 64.** 64→128 was tried and reverted by the maintainer; the observed test failures were deferred-`Set_Value` same-stack asserts, not registry overflow (disproven by rebuild + the at-capacity warning never firing).
2. **Parent-fallback over hoisting shared gates to the composite** — the `IsStuck` within-trip re-routing granularity is the named cost being avoided.
3. **The ~33-movable / ~28-shared key classification** (campaign doc §3) is the working set for the BB migration; per-key refs get re-verified at migration time, not at this review.
4. **`Set_Value` stays a deferred request** — no synchronous-write side door gets added for tests' convenience.
5. **Framework prerequisite first, BB migration second; pilot = Brainwash** (2 keys, one writer file, existing autotest) before ShoppingTrip (16 keys).

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- **Import-aliasing itself** (spec §2-3): is "parent owns truth, local slot is a per-plan snapshot alias" the right contract, or does the dual representation (parent value + local alias slot) create a coherence class of bugs the spec hasn't named?
- **Residency determinism** (spec §3.2): the argument rests on "top-level action setup precedes sub-planner activation." Is that airtight against every activation path (`DoResolveChildWorldStateFromParent`'s override → parent → lifetime-owner fallbacks), or is there a path where a sub-action's setup can outrun the parent catalog's?
- **Write forwarding by direct-apply inside the drain** (spec §3.4): the child's drain mutates the ancestor's `Values`/`ChangeLog`/`Subscribers` while the same processor's view may later visit that ancestor. The spec argues this is safe because only non-view fragments are touched cross-entity. Confirm against the EnTT view iteration + `AddOrGet<ChangeLog>` during iteration.
- **Chain generality**: fragment supports a parent *chain* (depth-capped) but BB uses one level. Right call, or should the first cut hard-require depth 1?

#### B. Convention compliance

- New fragment (`FFragment_Goap_WorldState_ParentLink`) — friend pattern, `CK_GENERATED_BODY`, `CK_PROPERTY_GET`, placement in `CkGoap_WorldState_Fragment.h` — consistent with the five sibling WS fragments?
- Utils API shape: parent arg on `Create` vs a separate `Create_WithParent` — which matches house style for optional composition args?
- The Verbose-not-Warning logging rule for expected boundary conditions (the AutoTest harness escalates Warnings) — spec keeps it; confirm the new-local-key-under-parent log (spec §6) should also be Verbose.

#### C. Load-bearing claims to spot-check in code

- `CkGoap_WorldState.h:142,305` — the fixed 64-slot arrays that force single-index-space (the spec's central constraint).
- `CkGoap_Action_Processor.cpp:136-142` — setup deferral until WS-source resolution (the determinism argument's foundation).
- `CkGoap_Action_Processor.cpp:427-447` — the snapshot + override flatten the merge step extends.
- `CkGoap_WorldState_Processor.cpp:64-109` — the drain the write-forwarding lands in.
- `CkGoap_Planner_Utils.cpp:139-144` and `CkGoap_Planner_Processor.cpp:436-441` — the two subscribe sites that must both become chain-aware.

#### D. Test coverage

- Do the 8 planned AutoTests (spec §7) cover the contract? Named candidates for gaps: promoted-planner activation path (vs top-level), capacity warning on a sub-registry, teardown (parent WS destroyed before sub-WS), and save/load once §5's open check resolves.
- Is "all WS asserts behind a settle" sufficiently enforced by test authoring convention, given that exact trap already burned this campaign once?

#### E. Risks the spec calls out — sized correctly?

- Residency-ordering hazard mitigated by BB-side pre-registration of ~28 shared keys (spec §6) — is a game-side mitigation acceptable for a framework-level hazard, or does the framework owe a harder guarantee (e.g. refuse/ensure on a sub-local registration of a tag the parent later registers)?
- `ReplanCause` diagnostic gap and signal-fires-on-owner-only (spec §5) — acceptable for this pass?

#### F. Forward-compat

- Per-sub-WS GOAP debugger view is deferred — does the fragment shape (ParentLink + `_ImportedTags`) give the debugger what it will need?
- Save/load: WS persistence is unverified (spec §5). If CkGoap WS values turn out to persist, does import-aliasing survive a rebuild+hydrate load (registry rebuilds at setup; imported truth lives in the parent)?

### Output format — fill in the CTO Review Response section below

Be direct. If the design is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers tied to a spec section or file, not vague concerns.

---

## CTO Review Response

### Verdict

`CHANGES REQUESTED`

The **shape is right** — import-aliasing is the correct answer to the fixed-array constraint (confirmed `CkGoap_WorldState.h:142,305`; memcmp/CRC identity genuinely forces one index space per search state), direct-apply write forwarding is the right variant and specifically safer than re-enqueueing, and the snapshot-merge/chain-subscribe mechanics are minimal and leave A* untouched. The blockers are all in the **residency-determinism story**, whose central claim is disproven by the code the spec itself cites. The fixes are spec-text plus one small API addition plus two tests — not a redesign.

### Blocking issues

1. **§3.2's determinism argument is factually wrong: WS-source resolution is eager, not activation-gated.** The spec claims "a promoted sub-tree's action setup defers until the activation walk resolves its WS source … and activation requires the parent to have planned." The code says the opposite, on purpose: `CkGoap_Planner_Internal.h:19-20` — *"Eager, so the child's Setup can run before any parent plan is requested"* — and `DoCreateOrFindActionEntity` invokes `DoResolveChildWorldStateFromParent` at AddAction time for both promoted hosts (`CkGoap_Planner_Utils.cpp:748`) and top-level actions (`:756`). Worse, BB's intended wiring is exactly the eager branch: a sub-planner carrying `_WorldStateSource_Override` (the natural way to point it at the sub-WS) resolves at `:678-683`, its child actions inherit that resolved handle at their own AddAction, and every action in the tree clears the `:139` validity gate in `FProcessor_Goap_Action_Setup` in the **first setup pass after construction** — where iteration order between top-level and sub-action setups is arbitrary. So the ordering "top-level setup registers shared keys before sub-classification runs" does not hold. Activation gates *planning*, not *setup* — and classification happens at setup. The activation-time resolver (`CkGoap_Planner_Processor.cpp:395-427`) is a second-chance fallback, not the primary path. §3.2 must stop relying on this ordering; §6's mitigation becomes the primary guarantee, not a residual patch.

2. **The §6 mitigation, as specced, is itself ordering-dependent.** Pre-registration via `Request_RegisterKey` (`CkGoap_WorldState_Utils.cpp:167-176`) is a deferred request drained by `FProcessor_Goap_WorldState_HandleRequests`. Whether that drain runs before the first `FProcessor_Goap_Action_Setup` pass for a same-frame-constructed NPC is a scheduler-ordering property the spec neither verifies nor pins. Fix: make pre-registration **synchronous**. Preferred shape: a `_PreRegisteredKeys` array on `FCk_Fragment_Goap_WorldState_ParamsData` — the params struct is empty today and `DoStampWorldStateFragments` (`CkGoap_WorldState_Utils.cpp:48-49`) carries a comment expecting its first knob — registered inside `Add`/`Create` at composition time. This does **not** violate locked decision #4: `Set_Value` stays deferred; key registration is monotonic, value-free, and is *already* performed synchronously by action setup via `Get_MutableRegistry().FindOrRegister` (`CkGoap_Action_Processor.cpp:146-147,160-161`). With registration synchronous at WS composition, the residency classification is airtight regardless of processor tick order, and the answer to "is a game-side mitigation acceptable?" becomes yes — the framework cannot know a game's shared-key set, but it must offer an ordering-proof way to declare it, which the deferred request is not.

3. **Parent-teardown semantics are undefined.** The spec never says what an *imported* key reads or writes when `_Parent` no longer resolves (parent WS entity destroyed before the sub-WS). The local alias slot exists but holds a stale last-merged snapshot; the read path as written ("imported → parent's effective value") has no defined fallback. Pick a contract (recommend: dead parent → reads return `false` i.e. the miss-at-chain-root contract, writes fall through to local with the same Verbose diagnostic as registry-full; do *not* silently serve the stale alias slot as truth), write it in §3.3/§3.4, and add the teardown AutoTest the brief already names as a gap. BB's nested lifetimes make real exposure low, but a framework contract can't leave this to whatever the handle-validity checks happen to do.

### Non-blocking suggestions

1. **Name the cross-handle write-ordering caveat in §5.** Today, all writes to one WS land in one `Requests` fragment and drain FIFO. Post-forwarding, a same-frame write to the same shared key via the child handle and via the parent handle resolve in **entity-iteration order**, not enqueue order — last-wins becomes nondeterministic across handles. Narrow (cross-frame it converges), but this campaign was already burned once by deferred-write timing; name it and state the convention: one writer handle per key per frame (BB's classification table effectively guarantees this — sensors write shared keys via the shared handle only).
2. **Add a 9th AutoTest — adversarial ordering.** Sub-action references a shared-gate tag with pre-registration deliberately omitted → assert local (mis)classification is observable and the Verbose diagnostic fires; repeat with pre-registration → assert import. This pins the §6 hazard as a regression test instead of a doc paragraph.
3. **Unify the twin resolvers before extending them.** `DoResolveChildWorldStateFromParent` (`CkGoap_Planner_Utils.cpp:668-710`, has the early-out at `:675`) and `DoResolveAndAssignWorldStateSource` (`CkGoap_Planner_Processor.cpp:395-427`, no early-out) are near-duplicates. Any chain-aware change must land in both or they drift — fold them into one shared internal before touching either.
4. **Chain-aware unsubscribe.** §3.6 makes both *subscribe* sites chain-aware but is silent on `DoSubscribeActionToWorldState`'s twin `DoUnsubscribeActionFromWorldState` (`CkGoap_Planner_Processor.cpp:447-455`). A deactivated sub-planner still subscribed to the parent WS gets dirtied by every shared-gate flip → spurious replan pressure. Make unsubscribe walk the same chain.
5. **Cycle guard should `CK_ENSURE`, not silently stop.** A cycle in ParentLink is authoring error, not a boundary condition; the depth-capped walker idiom already exists (`Goap_Planner_GetParentPlanner`, `CkGoap_Action_Processor.cpp:109-118`).
6. **Keep chain generality (don't hard-require depth 1).** The walker-with-cap costs nothing extra, the cycle guard covers misuse, and hard-coding depth 1 buys a second framework migration the first time anyone nests. Right call as specced.
7. **On-demand desync audit as the harder detector.** The "refuse/ensure when the parent later registers a tag a child holds as local" guarantee the brief floats would need a reverse child index the design doesn't have (ParentLink is one-directional; note `Create` already Records children via `FRecordOfGoapWorldStates` at `CkGoap_WorldState_Utils.cpp:103-105` — a possible reverse path). Cheaper and sufficient: a cold-path audit (console command / the deferred debugger view) that walks parented WS entities and flags any tag local-non-imported in a child AND resident in an ancestor. That is the true desync detector, at zero hot-path cost.

### Convention compliance spot-checks performed

- `Source/CkGoap/Public/CkGoap/Algorithm/CkGoap_WorldState.h` (full) — 64-slot arrays at `:142,305` confirmed; `FindOrRegister` silent-reject at `:329` confirmed.
- `Source/CkGoap/Public/CkGoap/Action/CkGoap_Action_Processor.cpp:100-229, 360-560` — setup deferral `:136-142`, registration loop `:160-161`, capacity warning `:169-174`, silent drops `:192/:199`, Plan-branch snapshot+flatten `:427-447`, goal local-`Find` `:540-555` all as cited.
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Processor.cpp` (full) — drain `:64-109`; cross-entity subscriber dirtying precedent at `:97-106`; Verbose-not-Warning rationale `:77-79`.
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Utils.cpp:40-190` — `Add`/`Create` `:57-108`, `Get_Value` `:124-154`, `Request_RegisterKey` `:167-176`, empty-Params comment `:48-49`.
- `Source/CkGoap/Public/CkGoap/WorldState/CkGoap_WorldState_Fragment.h` (full) — the five sibling fragments: `CK_GENERATED_BODY` + friend + `CK_PROPERTY_GET` pattern confirmed. Note for the new fragment: use the **global-scope-qualified** `friend class ::UCk_Utils_Goap_WorldState_UE;` spelling (per the header's own comment at `:14-16`; `OverrideStack`/`ChangeLog` do, `KeyRegistry`/`Values`/`Subscribers` predate it). Placement in this header is correct.
- `Source/CkGoap/Public/CkGoap/Planner/CkGoap_Planner_Utils.cpp:100-178, 650-760` and `CkGoap_Planner_Processor.cpp:395-480` — both subscribe sites confirmed; eager resolution call sites `:748,756`; `CkGoap_Planner_Internal.h:19-20` eager-by-design comment.
- **API-shape calls:** optional parent arg on `Create` (not a separate `Create_WithParent`) matches house style for optional composition args — approved. Verbose (not Warning) for new-local-key-under-parent — approved, and it is not even a boundary condition: it is the *normal* path for all ~33 sub-local keys; Warning would spam and fail the AutoTest harness.

### Design / architecture observations

- **Import-aliasing dual representation (brief A-1):** the "parent value + local alias slot" pair is coherent *because* the local slot is only ever read by the plan-snapshot merge — `Get_Value` routes imported tags to the parent, and writes forward. The one coherence hole is the dead-parent case (blocking issue 3); the second is the cross-handle same-frame ordering (suggestion 1). No other coherence class found.
- **Direct-apply inside the drain (A-3): confirmed safe, and the right variant.** The view iterates `(KeyRegistry, Values, Subscribers, Requests)`; the forwarded write mutates the *ancestor's* `Values`/`Subscribers` **data** (no pool-structure change), `AddOrGet<ChangeLog>` touches a pool the view doesn't iterate (EnTT-safe), and subscriber tag-dirtying cross-entity is already done in this exact drain today (`:105`). The critical property direct-apply buys: the forwarded write never touches the ancestor's `Requests` fragment — re-enqueueing would interleave with the ancestor's own `CopyAndRemove` drain and reintroduce exactly the deferred-write ordering bugs this campaign already paid for. Keep direct-apply; the implementation must preserve "never touch an ancestor's `Requests`" as an invariant.
- **Residency determinism (A-2): not airtight — see blocking issues 1-2.** With synchronous pre-registration it becomes airtight *by construction* rather than by ordering argument, which is strictly stronger than what the spec originally claimed.
- **Test 3 addition:** also assert that the local alias slot never leaks into `Get_Value` (i.e. after a plan-snapshot merge, a subsequent parent-side change is still what the sub handle reads) — that's the read-path half of the dual-representation coherence contract.
- **Save/load (F):** posture is right — verify before implementing. If WS values do persist, `_ImportedTags` and the registry both rebuild at action setup, and truth lives in the parent, so import-aliasing survives rebuild+hydrate provided `ParentLink` is re-established by `DoConstruct` replay (it is composition, so it will be). Restored-handle tolerance rules apply to `_Parent` like any persisted handle.
- **`ReplanCause` gap and owner-only signals (E-2): accepted.** Owner-only broadcast is not merely acceptable — it is *correct* (single truth → single broadcast site); mirroring signals onto sub handles would recreate the desync class this design exists to kill.

### Sign-off conditions (only if "CHANGES REQUESTED")

1. Rewrite §3.2/§6: drop the activation-gating determinism claim; make **synchronous pre-registration at WS composition** (e.g. `_PreRegisteredKeys` on `FCk_Fragment_Goap_WorldState_ParamsData`, registered in `Add`/`Create`) the primary residency guarantee. BB's shared-gate list moves from a construction-time request loop to the shared WS's creation params.
2. Define the dead-parent contract for imported keys (reads, writes, diagnostic) in §3.3/§3.4, and add the parent-teardown AutoTest.
3. Add the adversarial-ordering AutoTest (suggestion 2) to the §7 list.

These three land → GREEN-LIGHT, including the BB pilot sequencing as specced (Brainwash → ShoppingTrip). Suggestions 3 and 4 (resolver unification, chain-aware unsubscribe) are strongly recommended for the implementation pass but do not gate the spec.

---

### Reviewer

- **Name:** CTO review (Claude Fable 5, on behalf of Adam)
- **Date:** 2026-08-13
