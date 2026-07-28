# Request completion delegates — mission brief (PROMPT.md)

> **Written:** 2026-07-25. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **TOMBSTONED 2026-07-27 — death condition met.** The campaign shipped and root
> [CLAUDE.md](../../../CLAUDE.md) § "Request completion" carries the contract. Superseded by that
> section; everything below is kept for history only and must not be treated as current instruction.
> The gate structure it describes was collapsed into a single repo-wide rollout at maintainer
> request — see PROGRESS.md's Current state.

## Goal

Every `FCk_Request_*` across every CkFoundation feature can report completion back to the
requester with success/failure. A caller that passes a completion delegate to any `Request_*`
util is GUARANTEED the delegate fires exactly once with one of: `Succeeded`, `Failed`,
`Failed_NotEnqueued` (rejected synchronously at the Utils boundary), or `Failed_Cancelled`
(owner torn down before the request was processed). A caller that passes no delegate pays
nothing (one `IsBound()` check).

## Success criteria

1. Every `Request_*` UFUNCTION on every feature Utils accepts a trailing completion delegate
   (optional in BP via `AutoCreateRefTerm`; defaultable in AS/C++).
2. Every request handler fires the completion signal via `ck::TRequestResultGuard` (or direct
   broadcast at the completion point for async-latency requests).
3. Teardown guarantee: pending (queued, unprocessed) requests on a destroyed owner fire
   `Failed_Cancelled` — verified by an AutoTest per rollout gate.
4. Three environments verified per gate: C++, Blueprint, AngelScript (root non-negotiable #4).
5. Full toolbox `--build --test` gate green + delta-zero vs the Gate 00 baseline at campaign end.

## Constraints & locked decisions

Regenerated from PROGRESS.md's decision log at each gate — do not edit here without a dated note.

| # | Decision | Choice | Why |
|---|---|---|---|
| G0-D1 | Mechanism | Extend **Shape A**: `PopulateRequestHandle` + `CK_SIGNAL_BIND_REQUEST_FULFILLED` + `ck::TRequestResultGuard`, delegate bound at the Utils boundary on the request entity | Already the framework's designated idiom (`CkSignal_Macros.h:47-49`); reference implementation exists (CkInventory); zero-cost when unused |
| G0-D2 | Default result payload | New shared `ECk_Request_OperationResult { Succeeded, Failed, Failed_NotEnqueued, Failed_Cancelled }` in CkEcs; bespoke per-op enums stay/allowed where a feature has richer failure modes (CkInventory precedent) | Maintainer-ruled 2026-07-25 |
| G0-D3 | Scope | ALL `FCk_Request_*` structs, including trivial setters | Maintainer-ruled 2026-07-25; uniformity is the contract |
| G0-D4 | Shape C convergence | CkEqs `_OnComplete` / CkDialog `_OnComplete` members DELETED; delegate becomes trailing `Request_*` param like everywhere else. No back-compat shims | Maintainer-ruled 2026-07-25; house refactor doctrine |
| G0-D5 | Teardown | Guaranteed-fire contract: pending requests fire `Failed_Cancelled` via a shared cancel helper invoked from the feature's teardown path | Maintainer-ruled 2026-07-25 |
| G0-D6 | Generic default signal | ONE `CKECS_API` signal + dynamic delegate in CkEcs serves the default case for all features (request entities are per-request, so binds can never collide). Features may define bespoke signals for richer payloads | Fable-ruled 2026-07-25; cuts ~50 per-feature signal/delegate/enum triplets |
| G0-D7 | Network semantics | Completion is LOCAL-machine: the delegate reports the outcome of local processing. Cross-network completion (client informed of server-side outcome) is out of scope | Fable-ruled 2026-07-25; request entities don't replicate |
| G0-D8 | Bind-only-when-bound | `PopulateRequestHandle` + signal bind happen ONLY inside `if (InDelegate.IsBound())` (CkEqs precedent, `CkEqs_Processor.cpp:121-130`) | No per-request entity spawn for the 99% no-delegate case |

## Rejected approaches

| Rejected | Why |
|---|---|
| Delegate carried inside the request struct (Shape C) | Two API shapes forever; UHT-reflected delegate members bloat every request copy; converged instead (G0-D4) |
| Per-feature bespoke signal + enum everywhere | ~50 modules × (delegate + signal + enum + BindTo) boilerplate with no information gain for default requests (G0-D2/D6) |
| Response fragments / futures / polling | No precedent in the framework; signals with `Unbind` post-fire are the established one-shot notification; adds lifetime questions signals already answer |
| Central EnTT `on_destroy` hook firing cancels for all request fragments | Requires per-type registration anyway; broadcasting from registry-teardown callbacks is orderly only inside EndPlay-group processors — keep cancel in feature teardown paths |

## Non-goals

- **Cross-network completion** — a client caller learning the server's processing outcome (G0-D7). Future feature; would need a replicated request-correlation mechanism.
- **`CkCore` DebugDraw subsystem requests** (`Ck_Request_DebugDrawOnScreen_*`) — not ECS requests, no `_Requests` fragment, no processor. Out of scope.
- **Changing WHEN requests are processed** — the deferred-mutation contract (root non-negotiable #5) is untouched.
- **Entity-level `BindTo_On*` signals** — they stay as-is; they answer "did the entity change", not "was MY request handled".

## The contract being rolled out (canonical shapes)

Defined once in Gate 00 (CkEcs), consumed by every rollout gate. Exact signatures in
[GATE_00_Infrastructure.md](GATE_00_Infrastructure.md).

**Utils boundary** (mimics `CkInventory_Utils.cpp:192-216`, gated per G0-D8):

```cpp
auto UCk_Utils_Timer_UE::Request_Pause(FCk_Handle_Timer& InTimer,
    const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Timer
{
    // existing validation; on synchronous rejection:
    //   InDelegate.ExecuteIfBound(InTimer, ECk_Request_OperationResult::Failed_NotEnqueued);
    const auto Request = FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Pause};
    if (InDelegate.IsBound())
    { CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_RequestCompleted,
          Request.PopulateRequestHandle(InTimer), InDelegate); }
    // existing enqueue, unchanged
}
```

**Handler** (mimics `CkInventory_RequestHandlers.cpp:65-101`):

```cpp
auto Result = ECk_Request_OperationResult::Failed;
const auto Guard = ck::MakeRequestResultGuard<ck::UUtils_Signal_RequestCompleted>(
    InRequest, [&]{ return ck::MakePayload(OwnerHandle, Result); });
// ... existing handling; set Result = Succeeded on the success path
```

**Teardown** — the feature's EndPlay/Destructor path calls the shared cancel helper
(Gate 00 deliverable) over its `_Requests` fragment, firing `Failed_Cancelled` for every
pending entry with a valid request handle.

## Reading list (executor sessions load in this order)

1. This file, then [PROGRESS.md](PROGRESS.md) (current state — trust it over memory), then the
   current gate doc.
2. Skills: `ck-change-control` (always), `Plugins/CkFoundation:ck-macros-and-codegen` (before any
   fragment/signal/macro edit), `ck-tests-authoring-and-running` (before writing tests),
   `build-test` (before any build/test run), `ck-angelscript-interop` (if AS bindings misbehave).
3. Reference code, in full: `CkEcs/Request/CkRequest_Data.h` (+.cpp), `CkEcs/Signal/CkSignal_Macros.h`,
   `CkInventory/Inventory/CkInventory_Utils.cpp:150-420`, `CkInventory/Inventory/CkInventory_RequestHandlers.{h,cpp}`
   (result guard + `DispatchCancel`), `CkEqs/Query/CkEqs_Processor.cpp:100-140` (IsBound-gated bind),
   `CkTimer` quartet (canonical simple feature).
4. Root `Plugins/CkFoundation/CLAUDE.md` §Requests, §Signals; `Source/CkEcs/CLAUDE.md` §Signals.

## Glossary

- **Request entity / request handle** — the child entity `PopulateRequestHandle` creates under the
  request's target; the completion signal is bound to and broadcast on it, then it is destroyed
  (`GetAndDestroyRequestHandle`). One per request-with-delegate; never replicated.
- **Shape A/B/C/D** — survey taxonomy (see File inventory): A = request-handle+guard (Inventory),
  B = spawned request entity carries the request (SpawnActor et al.), C = delegate-in-struct
  (CkEqs/CkDialog, being deleted), D = entity-level `BindTo_On*` only.
- **Pending request** — an entry still sitting in a `FFragment_*_Requests` array (or equivalent)
  that no processor has drained yet.
- **Sync-failure path** — Utils-boundary validation rejecting a request before enqueue; delegate
  fires immediately with `Failed_NotEnqueued` on the game thread, same call stack.

## File inventory & feature census

The 2026-07-25 recon survey (Opus, verified spot-checks by Fable) found: 71 `FFragment_*_Requests`
fragments, ~250 `FCk_Request_*` structs, 23 `CK_SIGNAL_BIND_REQUEST_FULFILLED` call sites,
55 processors calling `GetAndDestroyRequestHandle()` as dead code (no one populates the handle).
Full per-module census with completion-shape classification lives in
[FEATURE_CENSUS.md](FEATURE_CENSUS.md). **Executors: the census is recon output — re-grep your
gate's modules at gate entry** (`rg -n 'struct FCk_Request_' Source/<Module>`,
`rg -n '_Requests' Source/<Module>`) **before trusting row-level detail.**

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| A `CK_REQUEST`/`RequestMany` codegen macro exists to hook | It does not; request plumbing is hand-rolled per feature. `CK_REQUEST_DEFINE_DEBUG_NAME` is the only request macro | recon §1.2, §5; `CkRequest_Data.h:111-121` |
| Generating dynamic delegates / UFUNCTIONs via custom macros | UHT parses source text and does not expand user macros — delegate decls and UFUNCTIONs stay hand-written | UHT behavior; existing codebase has zero macro-generated UHT types |
| Binary-serializing / sizeof-asserting request structs | Requests are polymorphic in Debug/DevEditor only (`#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING` vptr) — layout differs per config | `CkRequest_Data.h:46,95-103`; DECISIONS.md #27 |
| Reusing one request struct instance for two submissions | `PopulateRequestHandle` is one-shot; second call ensures and returns `{}` | `CkRequest_Data.h:17-18`, `CkRequest_Data.cpp:16-23` |
