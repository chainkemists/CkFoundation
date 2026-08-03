# Generator and self-heal operations

Reference for `ck-angelscript-interop`: running the AS generator, repairing codegen, recovering a broken binding set.

## 3. Generator + self-heal operations

What `CkAngelscriptGenerator` (editor-only module) runs, when — canonical mechanism doc:
`Source/CkAngelscriptGenerator/Claude.md` (read it before touching the module):

| When | What | Output |
|---|---|---|
| Editor boot (AS Early bind) | `FCkAngelscriptWrapperGenerator` | 268 `utils_*.as` + `cvar/collision/physicalsurface/_index.as` (CkFoundation `Script/Generated/`) |
| After each successful AS compile | `FCkAngelscriptEntityScriptParamsGenerator` | `<Plugin>_EntitySpawnParams.as` per plugin (BP-generated classes excluded) |
| After each successful AS compile | `FCkAutoTestWrapperGenerator` | `<Plugin>_AutoTestActors.as` (resilient `FSoftClassPath` lookup — drift is runtime-only, see catalog 3) |
| Post-compile / on demand | `UCkAssetRegistrySubsystem` | `*Assets.as` accessor files from `UCkAssetRegistryConfig` assets |
| On demand / self-heal deferred regen | `UCkDynamicHandleSubsystem` | `DynamicHandleTypes.json` |

All writes funnel through the Rev-12 ownership gate (catalog item 8). Generated output is
deterministic by contract: no timestamps, stable sorts, CRLF-on-write with LF-normalized compare.

**Manual maintenance buttons** — `UCkDynamicHandleSubsystem` (editor subsystem;
`Source/CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.h:39-41,67-71`):
- `GenerateHandleTypeRegistry()` (`CallInEditor`) — rewrite the JSON from all discovered
  `UCkDynamic_HandleDefinition` assets.
- `ForceRefreshDynamicHandleBindings()` (`CallInEditor`, DevelopmentOnly) — regen + re-register
  live AS bindings **without an editor restart** (also the fix when a definition's
  `RequiredFragments` changed and in-memory validators are stale).

**Escape hatches** (either short-circuits self-heal hook registration):
- `-NoCkAsRegen` launch flag — per-session, for inspecting raw Hazelight diagnostics without the
  dispatcher intervening.
- `UCk_AngelscriptGenerator_ProjectSettings_UE::_EnableAsBootstrapSelfHeal` (default `true`;
  `Source/CkAngelscriptGenerator/Settings/CkAngelscriptGenerator_Settings.h:36`) — project-wide,
  via Editor Settings → "AngelScript Generator" or `CkFoundation.ini`.

**Self-heal in one paragraph:** on a failed boot compile the dispatcher parses Hazelight's error
output (regexes pinned by snapshot tests `Tests/Test_AsErrorParser.cpp` — the engine-upgrade
canary; run `CkAngelscriptGenerator.UnitTests.AsErrorParser.*` before shipping an engine-plugin
upgrade), classifies recognized roots (missing `::Params`, missing `F<X>_SpawnParams`, missing
dynamic-handle type, missing asset accessor, adjacent literals), and writes recovery stubs to
sibling `_StubRecovery_*` files (never the canonicals; gitignored) from a **deferred modal-tick
callback** — writing synchronously inside the reload-error broadcast lands before the hot-reload
thread's mtime baseline and wedges the modal forever. Caps: 3 recovery cycles per bootstrap, 3
synthesis attempts per signature (convergence blacklist with a terminal banner naming the
callsites). Stubs are cleaned per-generator after the canonical regenerates; force-quit survivors
are swept at next StartupModule.

**Rev 9 → 12 hardening story** (dates; full detail in `Source/CkAngelscriptGenerator/Claude.md`):
1. Rev 9 (2026-05-11): descriptor-driven regen via a surgical engine-fork delegate — death-spiraled
   on first end-to-end test (595→595→0→17 classes); fork reverted same day.
2. Rev 10 (2026-05-12): error-driven self-heal dispatcher on the stock `GetReloadHadErrors`
   delegate; modal-tick deferral discovered empirically; 3-cycle bootstrap cap.
3. Rev 10.x (2026-05-13/17): asset-accessor Tier-3 fallback removed (refuse + banner); adjacent-
   literal diagnosis added.
4. Per-signature convergence cap after the 2026-05-21 `Params` dueling-overloads incident (724+
   recovery lines/session).
5. Rev 11 (post 2026-06-10/11 wedges): arg-category normalization, nullptr→UObject fallback +
   same-arity ambiguity gate, stale-canonical **quarantine** (delete-only, forensic copy to
   `Saved/CkSelfHeal/Quarantine/`).
6. Rev 12 (post 2026-06-12 two-instance ping-pong): cross-process single-writer OS file lock +
   real BPGC exclusion in the params generator.

---

