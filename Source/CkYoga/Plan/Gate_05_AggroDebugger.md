# Gate 05: Aggro debugger conventional migration

**Status:** accepted for the scoped authored Aggro migration. R6 compiled the final fixture and passed the real authority PIE path 1/1 with zero failed, skipped or contaminated tests. Fresh wide/narrow captures are visually accepted. Remaining conventional migrations, dynamic Visual LOD, browser parity and broader environment acceptance stay open.

## Evidence checkpoint (R1-R6)

- R1: build failed before editor boot while correcting Projects/FName fixes. Toolbox SHA256 `D7F6B3B83DEC6DA2963FDB1898FF54D7079264E78BA65453D178ED3D0547CAF5`.
- R2: build succeeded with two starts; fixture exposed target-expiry/invalid-getter ensures and unknown mount state. Toolbox SHA256 `E20C5296B9DA93B975EB53D43EF587C8AA86ADA1425D1991D329FF73FFC48F6B`; runtime SHA256 `3AD1D0A925CCCFEA87DDF5F65098B8898C940904E6359F85A309BCA2DFC8D3D2`.
- R3: one cached start failed fast on text-node padding admission. Toolbox SHA256 `03229A47289D7D336E82D5EE4F8C74C86BC878B17AAC017076B7B4569698F718`; runtime SHA256 `84CC567DBA620CEF5B7B80258FD2B3B81E167AA5E4D6967099CE7B62504A4D04`.
- R4: one cached start failed on overview region-root native sizing. Toolbox SHA256 `C92E371E709213F0DEA46453B1D9091413F31E59494294AF87C9ADBD41DEBD0D`; runtime SHA256 `36FCD01AC8092F7DC73E61003FDAB745E1D31EB93938B37BEB921F1CC29E4A21`.
- R5: one cached start reached PIE/teardown with no ensure contamination. Focused result was 0/1 because two UI-state composites failed. Toolbox SHA256 `7648ECCB7A1FA8D296C831CA269FD35BC11AA0148A0B098C373A30407FEAE92F`; runtime SHA256 `BD613FF49FCD2DA9E309351E06B7BFD1620D0A3F893DD88A18FB6B35A658BA5A`. Fresh captures `Saved/Automation/AggroDebugger/AuthoredPie-Wide.png` (SHA256 `72FDF93FFEC0F5F907D30594A45290330BF7A765FC11128D3E53B83424CC9EA9`) and `AuthoredPie-Narrow.png` (SHA256 `CF4531D1A573E482D962E6D47B82013DEECEC8FDC80340CFCAD984023F63EACE`) were visually readable, non-overlapping and responsive, but do not establish acceptance while the test is red.
- R6: incremental Development Editor build compiled the final test helper in four actions, then the focused `Ck.AggroDebugger.Authored.PIE` gate passed 1/1 in 1m16s with exit0 and zero failed/skipped/contaminated tests. The build invalidated the cached discovery list, so Toolbox used one focused lane editor plus one concurrent inline-discovery editor: two actual starts, within the final-gate budget. Toolbox SHA256 `DD868BEB5613262BEA72070A3008DE5F6108624978930C52C084F2E64FBA39D1`; actual focused runtime archive `AggroDebugger-R6-Authored-PIE-Editor.log` has one success and zero automation errors, ensures, fatals, access violations, relevant AngelScript compile diagnostics, parser/resource errors, wait timeouts or full-reload contamination; SHA256 `3373076922439FCBD47E891C5CF8FDE324280ED47E842CA78EAA4D7EBF714320`. The focused and discovery editors emitted only the expected generator skip warnings for null `GEditor`/secondary discovery ownership. Fresh `AuthoredPie-Wide.png` (966x646, SHA256 `F41E38CC71A6FFFC46845802C45404991EB2DC453DC72F985B3E6DEFD79E273D`) and `AuthoredPie-Narrow.png` (526x626, SHA256 `B87EA2472DEECCA2A14C0B13993446BC6E5B7D4B407B5FA12562E7B29F2C1339`) were inspected: controls remain reachable, owner/target rows and both meters are readable, and the narrow layout wraps without overlap or clipping. This accepts the scoped migration; deferred campaign work remains deferred.

## Scope

Migrate the conventional Aggro debugger presentation to one retained authored
view while preserving the native `SCkDebug_WindowChrome`, read-only collector,
refresh gate and authority-only semantics. HTML/CSS owns the engaged-only
control, dual searches, overview/status presentation, owner sections, target
rows, meters, empty states and responsive scrolling. C++ owns collection,
filtering, stable-key projection, live values and lifecycle admission.

## Decision

`[G5-AGGRO-D1]` Use one flat keyed repeat and the existing registered
`debug-meter`; do not retain the old owner/target subtree as a permanent native
port and do not add a new adapter. The registry's `debug-meter` constructs the
same `SCkDebug_MeterBar` used by the native window and supports its current
fraction, fill, size and tooltip contract. Aggro does not use the optional
marker. A flat repeat can express owner header/meta, owner-idle and target
record variants without unsupported nested collections.

## Required invariants

- Filter is case-insensitive owner-name OR any target-name admission and keeps
  the whole owner group; it never filters individual target siblings.
- Engaged-only requires a valid active tracked handle. Highlight remains a
  retained draft with its current no-op presentation behavior.
- Threat and score meters remain normalized per admitted owner. Preserve the
  exact threat state/color, score, distance, decay, forget and seen-age text.
- Stable keys use session/world generation plus owner entity number/version;
  target keys append target entity number/version. Display names are never
  identity. Equal-score order is unspecified.
- Value/rank refreshes with unchanged membership retain collection records and
  repeat widgets where their key survives. Add/remove changes membership
  atomically.
- Session/world invalidation increments generation before clearing the
  collector/projection and detaching the view. Held actions reject after
  invalidation. Do not leave snapshot handles alive until a later Tick.
- Missing world and authority world with no owners remain distinct, explicit
  states. Multi-world selection, controller/package/accessibility and browser
  parity are deferred and must not be claimed.

## Files

- `Plugins/CkGameplayDebugger/Source/CkAggroDebugger/Public/CkAggroDebugger/Window/SCkAggroDebuggerWindow.{h,cpp}`
- `Plugins/CkGameplayDebugger/Source/CkAggroDebugger/CkAggroDebugger.Build.cs`
- `Plugins/CkGameplayDebugger/Resources/UI/AggroDebugger.ui.html/.css`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkAggro/Test_AggroDebugger_AuthoredPie.spec.cpp`
- CkTests build/plugin metadata only if the real fixture requires an explicit
  dependency or staging declaration.

## Acceptance

1. A real authority PIE fixture creates an Aggro owner and at least two targets
   through public `UCk_Utils_Aggro_UE` APIs; no private cache injection.
2. Mounted controls route engaged-only and filter/highlight. Owner matching
   keeps all siblings; unavailable/empty authority states are explicit.
3. Real threat/selection changes update text/meters and reorder ranks without
   replacing surviving record/item identity or losing scroll. Target removal
   changes membership atomically.
4. Compatible/rejected installed-resource reload preserves/rejects correctly;
   session/world/owner release makes held callbacks inert and clears handles
   before EndPIE.
5. Fresh wide/narrow rendered captures show readable, non-overlapping Chrome,
   owner and target rows with scroll reachability.
6. Final gate uses UnrealToolbox only. Because C++/Build.cs/new test discovery
   change, budget at most two editor starts for one build-plus-focused-test
   invocation; inspect the actual runtime archive and captures.

