# Gate 00 — Core infrastructure + CkTimer pilot

> **2026-07-25 addendum — orchestrator rulings after first executor pass** (supersedes the
> conflicting step text below; does not change steps 1, 2, 7):
> - Step 3's "existing validation ensures stay / sync-rejection early-out" presumed validation
>   CkTimer does not have. Per **G0-D10**: do NOT add validation; CkTimer fires no
>   `Failed_NotEnqueued`. Step 5's `FailedNotEnqueued` test is DROPPED here (moves to Gate 01);
>   expected Total = **baseline + 3**.
> - Step 3's two immediate mutators (`Request_ChangeCountDirection`, `Request_ReverseDirection`):
>   per **G0-D11**, add the trailing delegate and fire it synchronously —
>   `InDelegate.ExecuteIfBound(InTimerEntity, ECk_Request_OperationResult::Succeeded);` after the
>   inline mutation. No request entity, no signal.
> - Canonical UFUNCTION shape per **G0-D9**: `AutoCreateRefTerm` PLUS `= FCk_Delegate_Request_OnCompleted()`
>   default (UHT acceptance measured by this gate's first build — if UHT rejects, STOP).
> - Entry criteria remain blocked on the toolbox engine selection (see PROGRESS.md Blockers).
>
> **Status:** 🟡 In progress — implemented at source level (uncompiled); gate runs + G0-D11
> remainder outstanding
> **Depends on:** — (first gate)
> **Estimate:** 1 executor session — re-date at entry; record actual at exit
> **Executor tier:** opus (fully specified; design already ruled — see PROMPT.md locked decisions)

## Goal

After this gate: the shared completion contract exists in CkEcs (`ECk_Request_OperationResult`,
`FCk_Delegate_Request_OnCompleted`, `UUtils_Signal_RequestCompleted`, `ck::request::FireCancelledForPending`),
and CkTimer — the canonical simple feature — uses it end-to-end: every `Request_*` on
`UCk_Utils_Timer_UE` accepts a completion delegate that fires `Succeeded` on drain,
`Failed_NotEnqueued` on sync rejection, and `Failed_Cancelled` on owner teardown, proven by
green AutoTests.

## Entry criteria (pre-flight — run these, don't assume them)

- [ ] `git -C Plugins/CkFoundation status` clean of foreign edits (enumerate any dirty paths you
      didn't author as "left untouched"); record HEAD hash here.
- [ ] **Baseline captured** via the `/build-test` skill (UnrealToolbox — NEVER Build.bat/UnrealEditor-Cmd
      directly): full `--build` then `--test` (omit `--config` on test-only; use `--no-nullrhi`).
      Record: pass/fail counts + every failing test name, dated, in PROGRESS.md. Known context:
      ~10/18 crowd tests are deterministically red pre-existing (foreign workstream — see memory);
      they stay red and are excluded from the delta.
- [ ] Editor not running for this project (poll the `Saved/Logs/CkPlugins.log` write lock; wait
      until free).
- [ ] Anchors re-verified on current HEAD: `CkRequest_Data.h:136-168` guard exists;
      `CkSignal_Macros.h:48` `CK_SIGNAL_BIND_REQUEST_FULFILLED` exists;
      `CkInventory_Utils.h:243-252` delegate-param UFUNCTION shape.

## Work items

Each step names the proven pattern it replicates. Load `Plugins/CkFoundation:ck-macros-and-codegen`
before step 1; `ck-tests-authoring-and-running` before step 5.

### 1. New header `Source/CkEcs/Public/CkEcs/Request/CkRequest_Completion.h` — NEW INFRASTRUCTURE (small, fully specified)

Contents (exact; includes `CkRequest_Completion.generated.h` last; needs `CkSignal_Macros.h`,
`CkPayload.h`, algorithm/visitor headers per house include order):

```cpp
UENUM(BlueprintType)
enum class ECk_Request_OperationResult : uint8
{
    Succeeded,
    Failed,
    Failed_NotEnqueued UMETA(DisplayName = "Failed (Not Enqueued)"),
    Failed_Cancelled   UMETA(DisplayName = "Failed (Cancelled)")
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Request_OperationResult);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_Request_OnCompleted,
    FCk_Handle, InRequestOwner,
    ECk_Request_OperationResult, InResult);

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, RequestCompleted,
        FCk_Delegate_Request_OnCompleted, FCk_Handle, ECk_Request_OperationResult);
}
```

-> verify: mimic where existing signals place the `namespace ck` wrapper (see any
`CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` site, e.g. CkInventory_Fragment.h) — if house style
defines signals in `_Fragment.h` files inside `namespace ck`, match it exactly. If the macro
does not expand cleanly inside a namespace here → check how CkInventory does it and copy;
anything else → STOP.

### 2. Shared cancel helper, same header

```cpp
namespace ck::request
{
    // Fires Failed_Cancelled for every pending request that has a bound completion signal.
    // T_RequestList = TArray<std::variant<...>> of FCk_Request_Base-shaped entries.
    template <typename T_RequestList>
    auto FireCancelledForPending(const FCk_Handle& InOwner, const T_RequestList& InRequests) -> void;
}
```

Body: iterate via `ck::algo::ForEachRequest(..., ck::Visitor(...), policy::DontResetContainer{})`
(pattern: `CkTimer_Processor.cpp:48-60`); per request: `if (InRequest.Get_IsRequestHandleValid())`
→ `UUtils_Signal_RequestCompleted::Broadcast(InRequest.GetAndDestroyRequestHandle(),
MakePayload(InOwner, ECk_Request_OperationResult::Failed_Cancelled))`.
-> verify: CkEcs compiles standalone (`--build`), zero behavior change anywhere (nothing calls it yet).

### 3. CkTimer Utils — trailing delegate on every `Request_*` UFUNCTION

Pattern: `CkInventory_Utils.h:243-252` exactly — `meta = (AutoCreateRefTerm = "InDelegate")`,
delegate is the LAST param, concrete return type unchanged. Functions (re-grep at entry:
`rg -n 'Request_' Source/CkTimer/Public/CkTimer/CkTimer_Utils.h`): Stop, Pause, Resume(if present),
Jump, Consume, ChangeCountDirection, and any other `Request_*` present. Body pattern per PROMPT.md
"Utils boundary" block:

- Existing validation ensures stay; on the sync-rejection early-out add
  `InDelegate.ExecuteIfBound(InTimerEntity, ECk_Request_OperationResult::Failed_NotEnqueued);`
  before the `return`.
- After validation: `if (InDelegate.IsBound()) { CK_SIGNAL_BIND_REQUEST_FULFILLED(
  ck::UUtils_Signal_RequestCompleted, Request.PopulateRequestHandle(InTimerEntity), InDelegate); }`
  — note the request must now be a named local constructed BEFORE enqueue (the current code
  constructs it inline in `Emplace`); enqueue that same local so the populated handle travels
  into the queue (mimic how `CkInventory_Utils.cpp:205-216` uses `InRequest` for both).
-> verify: CkTimer compiles; `rg -c 'InDelegate' Source/CkTimer/Public/CkTimer/CkTimer_Utils.h`
   equals the number of `Request_*` UFUNCTIONs.

### 4. CkTimer processor — result guard + cancel path

- In `FProcessor_Timer_HandleRequests::ForEachEntity`'s drain lambda
  (`CkTimer_Processor.cpp:50-60`): replace the manual
  `if (InRequest.Get_IsRequestHandleValid()) { GetAndDestroyRequestHandle(); }` epilogue with the
  guard pattern (`CkInventory_RequestHandlers.cpp:65-101`): declare
  `auto Result = ECk_Request_OperationResult::Failed;` then
  `const auto Guard = ck::MakeRequestResultGuard<ck::UUtils_Signal_RequestCompleted>(InRequest,
  [&]{ return ck::MakePayload(FCk_Handle{InTimerEntity}, Result); });` before `DoHandleRequest`,
  set `Result = Succeeded` after it returns normally. Guard declared AFTER the locals it captures
  (header comment `CkRequest_Data.h:131-132`).
  - `DoHandleRequest` overloads that can reject (invalid manipulate state etc.): if any early-out
    exists inside them today, thread the result out (return value or out-param — pick the house
    shape: check whether Inventory handlers return their Result enum; mimic). If CkTimer's
    handlers cannot fail → `Succeeded` unconditionally after the call is correct; note it.
- **Cancel path**: find CkTimer's teardown processor
  (`rg -n 'EndPlay|Destructor|Teardown' Source/CkTimer`). If an EndPlay/Destructor processor
  exists → add `ck::request::FireCancelledForPending(InTimerEntity, RequestsFragment._Requests)`
  where the fragment is still alive. If none exists → add
  `FProcessor_Timer_CancelPendingRequests` mimicking an existing minimal EndPlay processor
  (find one via `rg -ln 'FProcessor_.*_EndPlay' Source/ | head -5`, copy its group/scheduling
  declarations exactly); registered via `CK_REGISTER_PROCESSOR`. If the destruction-group wiring
  is ambiguous (which group runs during entity teardown) → STOP and record the observation.
-> verify: full plugin compiles; existing timer AutoTests still green (targeted run).

### 5. AutoTests (CkTests submodule — `Plugins/CkTests`)

Load `ck-tests-authoring-and-running` first. New AS autotests (naming per existing
`Ck.AS.*` conventions; new tests need `--discover-fresh` — toolbox caches discovery):

| Test | Asserts |
|---|---|
| `...Timer.RequestCompletion.SucceedsOnDrain` | delegate fires once, `Succeeded`, owner handle matches, fires AFTER the request is processed (state observably changed) |
| `...Timer.RequestCompletion.FailedNotEnqueued` | call `Request_*` on an invalid/unsuitable timer handle → delegate fires synchronously with `Failed_NotEnqueued` |
| `...Timer.RequestCompletion.CancelledOnTeardown` | enqueue with delegate, destroy owner same frame before drain → delegate fires `Failed_Cancelled` |
| `...Timer.RequestCompletion.NoDelegateNoOp` | `Request_*` without delegate: behavior identical to before (state change only; smoke) |

AS trap fences (from memory, verified incidents): delegate bind must match const-ref param
signature exactly; no `NOT` macro in AS; no adjacent string literals; `_TimeoutSeconds` goes on
the actor wrapper. ck::Warning during a test fails it even after FinishSuccess.
-> verify: 4 new tests discovered (`--discover-fresh`) and green; expected Total = baseline + 4.

### 6. AS surface check — decision gate

Confirm AS callers can omit the delegate: find an existing AS call site of
`utils_inventory::Request_AddItem` (`rg --no-ignore -n 'Request_AddItem' Plugins/CkFoundation/Script Plugins/CkTests/Script`)
and check whether it passes a delegate.
- If AS allows omission (param optional) → record evidence, continue.
- If AS REQUIRES the delegate argument → STOP. This changes the rollout cost for every AS call
  site framework-wide; the orchestrator must rule before Gate 01.

### 7. Docs (same commit as the last work item)

- Root `Plugins/CkFoundation/CLAUDE.md` §Requests: add the completion-delegate contract
  (3-4 lines: trailing delegate, IsBound-gated bind, guard, guaranteed-fire, the four results).
- `Source/CkEcs/CLAUDE.md` §Signals: add `UUtils_Signal_RequestCompleted` + cancel helper.
- `Source/CkTimer/Claude.md`: note the pilot contract.
- PROGRESS.md: dated entry + status board flip (this gate's row AND this file's Status header,
  same commit).

## Expected observations at the gate — branches

| I will run | I expect | If instead | Response |
|---|---|---|---|
| `--build` after step 2 | clean compile, 0 new warnings | UHT chokes on delegate/enum in new header | check include order + `.generated.h` last; compare against `CkNet_Fragment_Data.h`; 2 failures → STOP |
| targeted Timer tests after step 4 | baseline-green set stays green | any previously-green timer test red | your change regressed it — revert the step, diagnose, re-sequence (restore-known-good rule) |
| `--test --discover-fresh` after step 5 | baseline + 4, all 4 new green | new tests absent | relink needed: touch + rebuild (memory: new automation tests need relink) |
| `CancelledOnTeardown` | `Failed_Cancelled` fires | delegate never fires | the cancel path isn't reached during teardown — record WHICH processor group ran/didn't (this is the load-bearing unknown of the gate); STOP, report verbatim |

## Fences (do NOT)

- Do NOT touch any module other than CkEcs, CkTimer, CkTests, and the three doc files.
- Do NOT add back-compat overloads or `_WithCallback` variants — signature change in place (G0-D4 doctrine).
- Do NOT `static_assert(sizeof)` or binary-copy request structs (config-dependent vptr — PROMPT.md ruled-out table).
- Do NOT bind/populate outside the `IsBound()` gate (G0-D8).
- Do NOT use stock `ensure`/`check`; `CK_ENSURE_IF_NOT` with a separate ordinary early-out branch; no side effects inside the ensure expression.
- Do NOT edit `Script/` while a test run is in flight; do NOT build while tests run.
- Do NOT commit or push — the orchestrator gates and ships (report a commit-ready summary instead).

## Exit criteria — ALL land in the SAME commit-ready state as the last work item

- [ ] CkEcs + CkTimer + CkTests compile; full `--test` (no `--config`, `--no-nullrhi`,
      `--discover-fresh`) green with Total = baseline + 4 and the SAME failing set as baseline
      (delta-zero on pre-existing reds).
- [ ] All 4 new tests green; step-6 AS-surface evidence recorded.
- [ ] Doc updates from step 7 done.
- [ ] PROGRESS.md dated entry: what ran, counts vs baseline, confirmed/inferred split, any
      observation that didn't match this doc (verbatim).
- [ ] Return to orchestrator: files touched (paths), the diff summary, gate counts, and every
      STOP/observation — evidence, not prose.

---

## Addendum 2 (2026-07-26) — G0-D13 transport rework: the implementation contract

**Supersedes:** steps 1-4's signal-transport internals above (the `PopulateRequestHandle` +
`CK_SIGNAL_BIND_REQUEST_FULFILLED` + `TRequestResultGuard` path for the DEFAULT case). **Does not
change:** the public UFUNCTION surface (G0-D9b), the immediate-mutator shape (G0-D11), the four
AutoTests' intent, the fences, or the exit criteria. Ruling + rationale: PROGRESS.md decision G0-D13.

### Header layout [G0-D13a — orchestrator-ruled]

**`CkEcs/Request/CkRequest_Completion.h`** —
- KEEP: `ECk_Request_OperationResult` (+ `CK_DEFINE_CUSTOM_FORMATTER_ENUM`),
  `FCk_Delegate_Request_OnCompleted`.
- REMOVE: the `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, RequestCompleted, ...)` block,
  and the now-unneeded `CkSignal_Macros.h` + `CkRequest_Data.h` includes (the latter MUST go —
  `CkRequest_Data.h` will include THIS header; a cycle will not compile).
- REWORK `ck::request::FireCancelledForPending`: body becomes a single unconditional
  `InRequest.TryFireCompletion(InOwner, ECk_Request_OperationResult::Failed_Cancelled);` per
  request (keep `algo::ForEachRequest` + `Visitor` + `policy::DontResetContainer`). NO
  `Get_IsRequestHandleValid()` short-circuit — that hole is exactly what INV-B2 proved.
- ADD `ck::TCompletionGuard<TRequest>` + `ck::MakeCompletionGuard(InRequest, InOwner, InResult)`:
  RAII guard, destructor calls `_Request.TryFireCompletion(_Owner, _Result)`; holds
  `const TRequest&`, `FCk_Handle _Owner` (by value), `const ECk_Request_OperationResult& _Result`
  (by reference — declared-after-locals invariant carries over verbatim from `TRequestResultGuard`,
  including the deleted copy/move set).

**`CkEcs/Request/CkRequest_Data.h`** —
- `#include "CkEcs/Request/CkRequest_Completion.h"` (before the USTRUCT).
- `FCk_Request_Base` gains: `private: mutable FCk_Delegate_Request_OnCompleted _CompletionDelegate;`
  (NON-reflected — no UPROPERTY; UHT ignores it) and three const methods (bodies in the existing
  .cpp, matching `PopulateRequestHandle`'s mutable+const idiom):
  - `Set_CompletionDelegate(const FCk_Delegate_Request_OnCompleted& InDelegate) const -> void`
  - `TryFireCompletion(const FCk_Handle& InOwner, ECk_Request_OperationResult InResult) const -> void`
    — `ExecuteIfBound(InOwner, InResult)` then `Unbind()`; the unbind IS the exactly-once guarantee.
  - `Get_HasCompletionDelegate() const -> bool` (`IsBound()`)
- UNTOUCHED: `ck::FRequest_Base` (C++-only base — Gate 08 revisits), `PopulateRequestHandle` /
  `GetAndDestroyRequestHandle` / `Request_TransferHandleToOther` (bespoke Inventory/Resolver path),
  `TRequestResultGuard`/`MakeRequestResultGuard` (Inventory still uses them).

### Call-site changes

**`CkTimer_Utils.cpp`** (9 deferred `Request_*`): replace each
`CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_RequestCompleted, Request.PopulateRequestHandle(InTimerEntity), InDelegate);`
with `Request.Set_CompletionDelegate(InDelegate);` — still inside the `if (InDelegate.IsBound())`
gate (G0-D8's letter survives; its request-entity rationale is moot). Immediate mutators (G0-D11
`ExecuteIfBound` shape) unchanged.

**`CkTimer_Processor.cpp` drain** (`FProcessor_Timer_HandleRequests`): per-request lambda becomes
```cpp
auto Result = ECk_Request_OperationResult::Failed;
const auto Guard = ck::MakeCompletionGuard(InRequest, InHandle, Result);
DoHandleRequest(InHandle, InCurrent, InRequest);
Result = ECk_Request_OperationResult::Succeeded;
```
(delete the `MakeRequestResultGuard<ck::UUtils_Signal_RequestCompleted>` epilogue). Note the drain
iterates a COPY of the requests container — the delegate rides the copy; that is correct and needs
no change.

**`CkTimer_Processor.h`**: `FProcessor_Timer_HandleRequests` view gains
`TExclude<FTag_DestroyEntity_Initiate>` alongside `CK_IGNORE_PENDING_KILL` (tags live in
`CkHandle.h`; macro definition at `CkEntityLifetime_Fragment.h:28-32` deliberately omits Initiate —
this processor opts INTO the stricter filter; do NOT edit the macro).
`FProcessor_Timer_CancelPendingRequests` stays exactly as registered (`FGroup_EndPlay`,
`CK_IF_END_PLAY`); update its header comment (says "completion signal" → delegate).

### VERIFY-STEP (gate, before the build) — Initiate synchronicity

The exclusion filter is deterministic ONLY if `FTag_DestroyEntity_Initiate` is applied
synchronously on the `Request_DestroyEntity` call stack (enqueue-then-destroy on one stack ⇒ the
drain that frame already sees the tag ⇒ skip ⇒ EndPlay cancel fires `Failed_Cancelled`). Read
`UCk_Utils_EntityLifetime_UE::Request_DestroyEntity` and its processor: if Initiate is applied
synchronously (or before any `FGroup_Gameplay_*` drain can run) → proceed. If it is DEFERRED to a
later slot than the Timer drain → STOP, report the exact application site verbatim; do not improvise.

### Residue greps (exit)

- `rg -n 'UUtils_Signal_RequestCompleted' Plugins/CkFoundation Plugins/CkTests` → 0 hits (docs included).
- `CK_SIGNAL_BIND_REQUEST_FULFILLED` remains ONLY in `CkSignal_Macros.h` (definition) + bespoke
  feature paths (Inventory) — none in CkTimer.
- The four AS tests need NO edits (they consume the unchanged public delegate param) — verify by
  read, not assumption; `CancelledOnTeardown` is now expected DETERMINISTICALLY green.

### Docs (rewrite, same pass)

Root `CLAUDE.md` §Requests "Request completion" block, `Source/CkEcs/CLAUDE.md` §Signals
subsection, `Source/CkTimer/CLAUDE.md` reference section — all three currently teach the signal
transport and are WRONG under G0-D13: delegate-on-struct at the Utils boundary, completion guard
in the drain, direct-fire teardown cancel, no request entity and no signal in the default case;
bespoke signal paths (Inventory) remain the documented exception.
