# PHASE 2 — Rename bundle: persistence vocabulary + slot renames + named entry points (build 2)

**Purely mechanical — NOTHING in this phase changes behavior.** If any rename forces a semantic choice,
STOP → Blockers. One build at the end. Three commits.

This phase ABSORBS the parity campaign's PHASE_6
(`docs/campaigns/saveload-v3-parity/PHASE_6.md`) — read it in full first; its rename table, do-NOT-rename
list, and 6B deletion steps are authoritative and are NOT restated here in full.

## Entry criteria

1. Phase 1 committed; both trees clean; gates at Phase-1 exit counts (PROGRESS.md).
2. Add to `saveload-v3-parity/PROGRESS.md` rows 6A/6B: `ABSORBED by saveload-v3-ergonomics Phase 2
   (2026-07-XX)` — one-line edit, `git add -f` if needed, folded into this phase's first commit message
   footer. This prevents the parity executor from double-running the rename.

## Step 1 — Parity 6A vocab renames (execute PHASE_6.md §6A verbatim)

| Old | New |
|---|---|
| `FCk_ReplicatedFragmentHandlerRegistry` | `FCk_PersistenceHandlerRegistry` |
| `ECk_RepFragment_ApplyResult` | `ECk_Persistence_ApplyResult` |
| `FGroup_Hydration` | `FGroup_DeferredApply` |

Follow PHASE_6.md's rules 1–4 exactly: check AS/generated-script exposure per its rule 1 (planner
verification 2026-07-14: the registry/FHandler/enum are pure C++, zero hits in `Script/` — expect the same;
re-verify), do NOT rename the types on its rule-2 list, NO file/folder moves (rule 3), the two dispatch
processors keep their names (rule 4). Enumerate hit counts BEFORE and AFTER with
`rg --no-ignore -c` and record them in PROGRESS.md.

## Step 2 — FHandler slot renames (this campaign's addition)

In `CkReplicatedFragmentContainer.h` `FHandler`: `Apply` → `NetApply`, `Remove` → `NetRemove`.
`Produce` and `HydrationApply` KEEP their names (design decision — see PROMPT.md "Chosen approach").

Call-site inventory (verified 2026-07-14; re-enumerate with `rg --no-ignore -n "\.Apply\b|->Apply\b|\.Remove\b|->Remove\b" Source/CkEcs` plus the registrars):
- `CkReplicatedFragmentContainer.cpp:162,192,218` — presence checks in FastArray callbacks.
- `CkReplicatedFragmentContainer_Processor.cpp:49` (`Remove` invoke), `:73` (`Apply` invoke). The
  `HydrationApply` invoke at `:132` is untouched.
- `CkEntityReplicationDriver_Fragment.cpp:104` — presence check.
- Registration-time ensure in `Register`/`ResolvePending` (mentions Produce/HydrationApply — verify wording
  still correct).
- All 24 registrars' designated initializers (`.Apply =` → `.NetApply =`). The shared attribute headers
  (`CkAttribute_RestorePersistence.h`, `CkAttribute_RefillPersistence.h`) and
  `CkStateMachine_Replication.cpp` / `CkRenderTarget_Replication.cpp` are included in this sweep.
- Update the FHandler slot comments: `NetApply` = "NET-receive apply … never runs on the loading
  authority"; `NetRemove` likewise.

Gate for Steps 1–2 combined (compile-enforced — the build catches any missed site):
`rg --no-ignore -n "FCk_ReplicatedFragmentHandlerRegistry|ECk_RepFragment_ApplyResult|FGroup_Hydration\b" Source/` → **0**;
`rg --no-ignore -n "\.Apply = |\.Remove = " Source/` → **0** (all converted).

## Step 3 — Named registration entry points (this campaign's addition)

Add to the (renamed) `FCk_PersistenceHandlerRegistry` in `CkReplicatedFragmentContainer.h`, with bodies in
the `.inl.h` beside `RegisterLazyTyped`:

```cpp
public:
    using FApplyFn   = TFunction<ECk_Persistence_ApplyResult(FCk_Handle& Entity,
                            const FInstancedStruct& NewData, const TOptional<FInstancedStruct>& OldData)>;
    using FRemoveFn  = TFunction<void(FCk_Handle& Entity)>;
    using FProduceFn = TFunction<TOptional<FInstancedStruct>(FCk_Handle& Entity)>;

    // Wire-only participation (never in the save file).
    template <typename T_RepData>
    static auto Register_NetOnly(FApplyFn InNetApply, FRemoveFn InNetRemove = {}) -> void;

    // Save-only participation (never rides a replicated container). Both params REQUIRED by signature —
    // the Produce-without-HydrationApply invalid shape is now uncompilable, not just ensured.
    template <typename T_RepData>
    static auto Register_SaveOnly(FProduceFn InProduce, FApplyFn InHydrationApply) -> void;

    // Both transports, one authority-safe applier serving NetApply AND HydrationApply.
    template <typename T_RepData>
    static auto Register_NetAndSave_SharedApply(FProduceFn InProduce, FApplyFn InSharedApply,
                                                FRemoveFn InNetRemove = {}) -> void;

    // Both transports, distinct appliers (net Apply is client-coupled — the TagSet shape).
    template <typename T_RepData>
    static auto Register_NetAndSave_SplitApply(FProduceFn InProduce, FApplyFn InNetApply,
                                               FApplyFn InHydrationApply, FRemoveFn InNetRemove = {}) -> void;
```

(Two distinct names, deliberately NOT an overload set — the variants would differ only by `TFunction`
parameter shapes, and TFunction's converting constructor makes such overload resolution fragile. The name
also states the choice the author made, which is the point of this step.)

Each body constructs `FHandler{...}` and forwards to `RegisterLazyTyped<T_RepData>`. Keep `FHandler`,
`RegisterLazyTyped`, and `RegisterFallback` public (the Dynamic fallback at `CkDynamic_Module.cpp:22` stays
on `RegisterFallback`).

Then convert all 24 registrars to the named forms per this mapping (from the 2026-07-14 census):
- **Register_NetOnly:** GeometryCollectionOwner; Transform ×3 (Location/Rotation/Scale).
- **Register_SaveOnly:** EntityTag; Dynamic SaveData; EntityScript SaveFields; Timer; Float+Integer Refill.
- **Register_NetAndSave_SharedApply:** Team; Player; Velocity; Acceleration; MontagePlayer.
- **Register_NetAndSave_SplitApply:** TagSet; AnimPlan; the 5 attribute kinds; EntityCollection; Grid
  Occupancy; Inventory Spatial + DataOnly; StateMachine ×2; RenderTarget.

Also rename the two misleading save-only registrar instances:
`FIntegerAttributeRefillRepHandlerRegistrar`/`GInteger…` and `FFloatAttributeRefillRepHandlerRegistrar`/
`GFloat…` → `…RefillSaveHandlerRegistrar`. Leave all other registrar names alone (churn without payoff).

Gate: `rg --no-ignore -n "RegisterLazyTyped<" Source/ --glob '!**/CkReplicatedFragmentContainer*'` → **0**
(every feature registrar uses a named form; only the .inl.h implementation references it).

## Step 4 — Parity 6B: T_Policy Option A deletion

Execute `saveload-v3-parity/PHASE_6.md` §6B steps 1–5 verbatim (enumerate ~85 sites first and record the
count; collapse the `_WITH_POLICY` macros; delete `CkSnapshot_Policy.h`; fix the `CkCamera_Utils.cpp:344`
comment). Its fences apply unchanged.

## Step 5 — Build + gate (the phase's ONLY build)

1. Build → exit 0. Expect a long compile (6B touches holder/record macros → wide UHT/unity rebuild).
2. `--test --test-pattern "Ck.Snapshot"` and `--test --test-pattern "Ck.Net"` → **delta-zero vs the
   PROGRESS-recorded baseline (counts AND names)**. Any delta → STOP; this phase cannot change behavior,
   so any red is yours — revert the offending commit and diagnose. (No `--discover-fresh` needed — no
   tests added/removed.)
3. Grep gates from Steps 1–3 all at 0; 6B enumeration grep at 0.

## Commits (in order)

1. `refactor(CkEcs): rename persistence vocabulary — PersistenceHandlerRegistry / ECk_Persistence_ApplyResult / FGroup_DeferredApply + FHandler Apply->NetApply, Remove->NetRemove (absorbs saveload-v3-parity PHASE_6 6A)`
   (include the parity PROGRESS.md absorption note in this commit)
2. `refactor(CkEcs): named persistence registration shapes — Register_NetOnly / Register_SaveOnly / Register_NetAndSave; all 24 registrars converted`
3. `refactor(CkEcs|CkEcsExt|CkRecord): delete inert T_Policy snapshot-classification surface (Option A, absorbs saveload-v3-parity PHASE_6 6B)`

## Fences

- NOTHING changes behavior. A rename that forces a semantic choice → STOP.
- Do NOT rename `FCk_RepData_*`/`FCk_SaveData_*` (save TypePath + wire surface), `FCk_ReplicatedFragmentEntry/Array`,
  `FTag_RepFragments_PendingApply`, `FFragment_PendingHydration`, `FTag_Hydration_PendingApply`,
  `FProcessor_Hydration_Dispatch`, `FProcessor_ReplicatedFragments_Dispatch`.
- Do NOT move files or folders (parity PHASE_6 rule 3 — recorded kill reason).
- Do NOT rename `Produce` or `HydrationApply` slots.
- Do NOT hand-edit `Script/Generated/` (nothing there references these types — verify, don't assume).
