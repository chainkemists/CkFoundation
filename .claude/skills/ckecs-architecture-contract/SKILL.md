---
name: ckecs-architecture-contract
description: "Use when reasoning about why CkEcs is structured as it is or which invariants a change must preserve across handles, requests, signals, fragments, replication, and GC."
---

# CkEcs architecture contract — why it is shaped this way

This skill records the load-bearing architectural decisions of CkFoundation's ECS, the reasons
behind them (with the commits and file:lines that prove them), the invariants any change must
preserve, and the known-weak points stated plainly. Terminology (Entity/Fragment/Processor/
Handle/Request/Signal) is defined in the root doctrine's Lingo table — `Plugins/CkFoundation/CLAUDE.md`.
All file paths below are relative to `Plugins/CkFoundation/Source/`; all facts verified 2026-07-02
against submodule HEAD `7330c1bab` unless labeled INFERRED.

## When NOT to use this skill

| You want to... | Load instead |
|---|---|
| Mechanically add a fragment/processor/handle/request (skeletons, macro checklists) | `ck-macros-and-codegen` |
| EnTT theory, registry/storage internals, entity↔actor lifetime, GC interaction depth | `ckecs-domain-reference` |
| Debug a build/UHT/AS-compile failure | `ck-debugging-playbook` |
| Work the teardown/unbind defect cluster | `ck-lifecycle-teardown-campaign` |

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The contract, section by section | `references/contract-details.md` |

## 8. Invariants — what must hold

Each line is a contract; breaking one is an architecture change, not a refactor.

1. **A typed handle is exactly `sizeof(FCk_Handle)`** — never add members
   (`CkHandle_TypeSafe.h:76-80, 144-145`).
2. **Typed handles are minted only through `Cast`/`CastChecked`/`ck::StaticCast`** — the private
   ctor + friend is the whole point (§2). AS parent up-conversion is the one unchecked path.
3. **Typed handles are declared in `X_Fragment_Data.h`** (89/90 as of 2026-07-02; §2).
4. **All non-empty fragment storage is pointer-stable** (`CkHandle.h:71-77`) — fragment references
   survive until destruction completes; raw-EnTT code must tolerate tombstones (§5, A3).
5. **Entity destruction is a multi-frame pipeline** — Initiate → EndPlay → Teardown → Await →
   Finalize over ~3 frames (diagram: `CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Fragment.cpp:30-70`);
   normal processors exclude dying entities via `CK_IGNORE_PENDING_KILL`
   (`CkEntityLifetime_Fragment.h:37-41`); cleanup work runs in `CK_IF_END_PLAY`/`CK_IF_TEARING_DOWN`
   passes. Never destroy fragments/components inside a normal `ForEachEntity`.
6. **Requests are drained exactly once per HandleRequests pass via copy-then-reset**; re-entrant
   enqueues survive to the next pass; the dirty tag is removed only when the live list is empty
   (`CkTimer_Processor.cpp:48-66`).
7. **Request structs are one-shot** — `PopulateRequestHandle` at most once per instance
   (`CkRequest_Data.h:16-18`).
8. **Every request struct carries `CK_REQUEST_DEFINE_DEBUG_NAME`** (root doctrine; measured
   2026-07-02: 171 of 184 `FCk_Request_*` comply — the gap is legacy, not license).
9. **Signal payload args: no references, no raw pointers** (`CkSignal_Fragment.h:47-48`);
   broadcast publishes from the stored payload, never the in-flight arguments (inl.h:55-63).
10. **A fire-once bind that replays never connects** (`CkSignal_Utils.inl.h:104-106`) — code must
    not assume a bind left a connection behind.
11. **Every UENUM gets `CK_DEFINE_CUSTOM_FORMATTER_ENUM`** (root doctrine; measured 318 formatter
    sites vs 380 UENUMs — close the gap when touching a file, don't widen it).
12. **Processors self-register exactly once, in their own `.cpp`** (`CK_REGISTER_PROCESSOR`,
    388 registrations measured 2026-07-02 (+1 commented example, +1 `#define`); the old
    ProcessorInjector mechanism is retired — root doctrine).
13. **Replication `Apply` never composes; `OnConstructed` ≠ values-applied** (§7).
14. **UE GC does not trace fragment members** — any UObject only a fragment points at WILL be
    collected unless rooted; pick `TStrongObjectPtr` (entity owns lifetime) vs `TWeakObjectPtr`
    (observation) per the root doctrine's ownership split (§9 below).
15. **Direct `_Member` writes only inside a fragment's declared friends** (§4) — everything else
    reads `Get_*` or enqueues a request.

---

## 9. Known-weak points — stated plainly

1. **GC blind spot (mitigated, not solved).** Fragment members are invisible to UE's GC; the
   ownership-split rule (TStrong/TWeak) is the mitigation, and packaged builds once crashed on
   exactly this class of bug — diagnosed `d77810096`, root-caused with a pre-GC rooting pass
   `feb08ee94`, tripwire-hardened `a8a93baac` (tripwire = detection, not the fix). Full incident
   history: `ck-failure-archaeology`.
2. **Teardown/unbind debt cluster (live defect).** `CkInteraction/Public/CkInteraction/InteractTarget/CkInteractTarget_Processor.cpp:222`
   carries the verbatim TODO *"This processor doesn't get called, can cause issues if teardown is
   mid interaction!!!"*; `CkRelationship/Public/CkRelationship/Team/CkTeam_Utils.cpp:376, 402, 428`
   carry three copies of *"figure out a bullet-proof way to remove the FTag_TeamListener if ALL the
   delegates have been unbound"*. This is the campaign target of `ck-lifecycle-teardown-campaign`
   — coordinate there before touching teardown paths.
3. **Request vtable config-variance.** `FCk_Request_Base`/`ck::FRequest_Base` are polymorphic only
   when `CK_DISABLE_ECS_HANDLE_DEBUGGING` is off (`CkRequest_Data.h:46-54, 95-103`) — request
   structs have a vptr (different `sizeof`) in Debug/Dev-editor and none in Test/Shipping. No code
   currently depends on request layout across configs; never memcpy, static_assert sizes, or
   serialize requests by layout (DECISIONS.md #27 documents this as a constraint, not endorsed
   design).
4. **`in_place_delete` costs iteration density framework-wide — by design.** Signal-local pointer
   stability (2c8319c1c, "proper fix upcoming") was later subsumed by the global
   `component_traits` specialization — introduced debug-gated (745507381, 2024-03-07),
   deliberately made unconditional in `06938bba3` (2026-02-17, "fragments are always pointer
   stable"; DECISIONS.md §45). The cost is real: owning groups are unavailable and tombstone
   storage can mask dangling-view bugs that swap-delete would surface — now a prioritization
   question (`ck-feature-frontier` candidate 5), no longer an open doctrine fork.
5. **`TOptional` in reflected surfaces is contested.** The doctrine teaches enum-mode + value; the
   newest modules (CkAudio, CkPmg) ship UPROPERTY `TOptional`s. Open as ADJUDICATIONS.md **A1** —
   match the file you are editing; do not churn either direction.

---

## Common mistakes

- Declaring a typed handle in `X_Fragment.h` — it belongs in `X_Fragment_Data.h` (§2).
- Treating a silent `Cast` failure as impossible — `Cast` returns an invalid handle without
  ensuring; only `CastChecked` is loud (§2).
- Trusting an AS parent-typed parameter to have validated the handle — up-conversion is unchecked
  (§2); the ensure fires later, deeper.
- Binding a late listener with `IgnorePayloadInFlight` and wondering why it never fires — a
  promise-read wants `FireIfPayloadInFlight` (§5, the 7f38dad33 lesson).
- Iterating `_Requests` in place or re-dispatching the copy without `DontResetContainer` — breaks
  the exactly-once/re-entrancy contract (§3).
- Reading team/attribute/SM values in `OnConstructed` on a client — values apply later; use
  `Promise_OnReplicationComplete` (§7).
- Composing a feature inside a replication `Apply` handler — return `NotReady` and let
  construction do it (§7).
- Storing a UObject only in a fragment and expecting it to survive GC (§9.1).
- Mutating another feature's `_Member` by adding yourself as a friend for convenience — the friend
  list is the audited write surface (§4).

---

## Provenance and maintenance

Verified 2026-07-02 against CkFoundation submodule HEAD `7330c1bab` (working tree: Source/ clean).
Re-verification commands (Git Bash, cwd `d:\Repos\BusterBlock\Plugins\CkFoundation`):

- Handle machinery lines: `rg -n "private:" Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h`
  (private ctor at the end of the TYPESAFE macro); `rg -n "static_assert" Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h`
- Typed-handle placement: `rg -c "CK_GENERATED_BODY_HANDLE_TYPESAFE\(" Source --glob '*.h'` then
  `rg -l ... | grep -v _Fragment_Data.h` (expect the base header + CkShape_Handle.h only)
- Derived handles: `rg -n "CK_GENERATED_BODY_HANDLE_DERIVED\(" Source --glob '*.h'` (7 uses + 1 definition)
- Signal policy line: `rg -n "ShouldFirePayloadInFlight" Source/CkEcs/Public/CkEcs/Signal/CkSignal_Utils.inl.h`
- Global pointer stability: `rg -n "component_traits" Source/CkEcs/Public/CkEcs/Handle/CkHandle.h`
- Origin commits: `git show --stat 7f38dad33 2c8319c1c 745507381 6b54d2e384 06938bba3 d77810096 feb08ee94 a8a93baac`
- Request one-shot + vtable variance: `rg -n "CK_DISABLE_ECS_HANDLE_DEBUGGING" Source/CkEcs/Public/CkEcs/Request/CkRequest_Data.h`
- Completion-bind sites: `rg -c "CK_SIGNAL_BIND_REQUEST_FULFILLED\(" Source --glob '*.{h,cpp}'` (19 + definition header)
- Friend counts: `rg -c "friend class FProcessor_" Source --glob '*_Fragment.h'` (384) and
  `"friend class UCk_Utils_"` (105)
- Request debug-name coverage: `rg -c "CK_REQUEST_DEFINE_DEBUG_NAME\(" Source --glob '*.h' | grep -v CkRequest_Data`
  (171) vs `rg -o "struct \w+ FCk_Request_\w+" Source --glob '*.h' -N | sort -u | wc -l` (184)
- Processor registrations: `rg -c "CK_REGISTER_PROCESSOR\(" Source --glob '*.cpp'` (sums to 388; a commented example in `CkAStar_Processor.h:45` and the `#define` are the only non-.cpp hits)
- Dispatch timeout: `rg -n "PendingApplyTimeoutSeconds" Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer_Processor.cpp`
- Weak-point TODOs: `rg -n "teardown is mid interaction" Source/CkInteraction` and
  `rg -n "bullet-proof way to remove the FTag_TeamListener" Source/CkRelationship`
- Formatter/UENUM coverage: `rg -c "CK_DEFINE_CUSTOM_FORMATTER_ENUM\(" Source --glob '*.h'` vs `rg -c "^UENUM" Source --glob '*.h'`
- Open forks: `.claude/reports/ADJUDICATIONS.md` (A1; A3 is resolved → DECISIONS.md §45) — re-read before repeating interim stances.

Tooling caveat (root doctrine): Grep/Glob tools can silently miss files under this plugin — on any
zero-match, re-check with `rg --no-ignore` before concluding absence.
