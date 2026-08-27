# Gate 0 — Scaffold + data surface

> **Status:** ✅ Done (2026-08-27) — code-complete and compile-green; [EDITOR-VERIFY] items below
> remain open for the maintainer and are re-checked at Gate 1 entry
> **Depends on:** design sign-off ✅ (2026-08-27, in-session)
> **Estimate:** 1 session — actual: same session as design

## Goal

After this gate: the CkVisualLod module exists, compiles in all three environments, and its full
DATA surface (handles, params, config asset, requests, signals, empty processors) is in place —
no mechanism behavior yet.

## Entry criteria (pre-flight — run these, don't assume them)

- [x] Design signed off (maintainer, 2026-08-27; DESIGN_CkVisualLod.md)
- [x] Baseline captured: build `Result: Succeeded` (754s) @ a9283eed9 / code tip 5d1ac9e83.
      NO test baseline — maintainer directive (2026-08-27): agent runs no test suites this
      campaign; compile gates + maintainer visual checks replace them
- [x] Code shapes verified against current doctrine (root CLAUDE.md + ck-macros-and-codegen read
      this session; ProcessorInjector retired; persistence = register nothing)

## Work items

Each replicates a named proven pattern:

1. Module boilerplate — `CkVisualLod.Build.cs` (CkModuleRules), `_Module.h/.cpp`, `_Log.h/.cpp`,
   uplugin entry, tier-table row. Pattern: `CkVisibleRange/` module root + `Source/CLAUDE.md`
   module-authoring rules.
2. Member quartet data surface — `CkVisualLod_Fragment_Data.h` (enums + formatters,
   `FCk_Handle_VisualLod`, `FCk_Fragment_VisualLod_ParamsData`, request structs, signal
   delegates), `CkVisualLod_Fragment.h` (tags, Params alias, `FFragment_VisualLod_Current`,
   `FFragment_VisualLod_Requests`, signal defines). Pattern: `CkVisibleRange_Fragment_Data.h` /
   `_Fragment.h`.
3. Arbiter quartet data surface — `CkVisualLodArbiter_*` siblings in the same directory
   (two sub-features, one module — pattern: CkAggro's `CkAggro_*` + `CkAggroTarget_*` split),
   incl. `UCk_VisualLodArbiter_Data : UCk_DataAsset_PDA` config asset (pattern:
   `UCk_IskmRenderer_Data`, CkIskmRenderer_Fragment_Data.h:109) with policy knobs +
   `TArray<FCk_VisualLod_CrowdConfig>`.
4. Utils skeletons — `Add`/`Has`/`Cast`/`CastChecked` + typesafe-handle plumbing
   (`CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE`), request-enqueue stubs with completion delegates.
   Pattern: `CkTimer_Utils` + add-a-new-x §3.3/§3.4.
5. Processor skeletons — HandleRequests ×2, `FProcessor_VisualLodArbiter_Update`,
   `FProcessor_VisualLod_EndPlay`, CancelPendingRequests ×2; registered, bodies minimal.
   Pattern: `CkVisibleRange_Processor.*`.
6. Initial module `CLAUDE.md` + tier-table row in `Source/CLAUDE.md`.

NEW INFRASTRUCTURE — none in this gate (mechanism lands in Gate 1).

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| `--build --generate` (build-only — NO test phase, maintainer directive 2026-08-27) | build green (UHT + new module compile) | UHT/compile errors | fix; `ck-debugging-playbook` if macro-shaped |
| `rg 'CK_REGISTER_PROCESSOR\(ck::FProcessor_VisualLod'` | 8 registration lines (4 member + 4 arbiter) | missing | add — an unregistered processor is a silent no-op |
| [EDITOR-VERIFY] maintainer: open the editor once | boots clean — no ensures/AS-bind errors naming VisualLod; BP shows the Add/Request/BindTo nodes | boot ensures | report verbatim; fix before Gate 1 |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [x] Expected observations confirmed: build green (3 attempts — INDEX_NONE auto-deduction fix,
      Has_Any link fix — final `=== Build succeeded ===`, 0 errors); 8/8 registrations grep-verified
- [x] Change class 2 (additive API); test evidence intentionally waived per maintainer directive
- [x] [EDITOR-VERIFY] (maintainer, before or at Gate 1 exit): open the CkPlugins editor once —
      (1) boots with no ensures or AS-bind errors naming VisualLod; (2) BP palette shows
      "[Ck][VisualLod] Add Visual Lod", the Request/BindTo nodes, and the `<AsVisualLod>` autocast;
      (3) `utils_visual_lod` / `utils_visual_lod_arbiter` resolve in AS after script regen
- [x] PLAN.md status row AND this Status header updated — both, this commit
- [x] PROGRESS.md dated entry appended
