# AS Literal-Asset Write-Back — CTO Review

**Date:** 2026-08-28
**Plan reviewed:** [PLAN.md](../campaigns/2026-08-28-as-literal-asset-writeback/PLAN.md) (rev 1)
**Verdict:** CHANGES REQUESTED → **GREEN-LIGHT** after rev 2 applied both sign-off conditions.

---

## Blocking issues raised

### 1. The patch-set predicate was wrong (plan §5.1/§5.2/§2)

Rev 1 selected properties to write by diffing the live instance against the **class CDO**. Three
linked defects, one root cause — conflating "differs from CDO" with "the user edited it":

- The no-churn claim was false. A hand-authored accessor line exists *because* its value differs
  from the CDO (`_Skeleton = assets::load::SK_Mannequin();` vs a null CDO,
  `CkIskmRenderer_Assets.as:31`). Every such line would be flagged changed and have its RHS
  regenerated on every write-back.
- §5.1 ("equal → not touched") contradicted §5.2 row 3 ("now equals CDO, line exists → delete"),
  which was unreachable under the stated rule.
- **Fatal in practice.** All three canonical literal-asset patterns in `Script/ARCHITECTURE.md` §13
  populate a container via `.Add()`. Containers are deferred in v1, so CDO-diffing put them in the
  patch set, failed to resolve them, and aborted the whole write. Rev 1 could not have written back
  a scalar edit on any flagship in-repo asset.

**Resolution (rev 2):** patch-set predicate redefined as "differs from the value the current file
text produces", built by re-running `__Init_<Name>` onto a scratch instance at button press
(mechanism already in-tree, `CkDeferredAssetInit_AngelScript.cpp:345-375`). Containers populated in
the body now match the baseline and never enter the patch set. The requester's "differs from class
CDO" is preserved as the **button-enable** condition only. Documented as plan Trap C.

*Note:* the reviewer offered `GetPostLiteralAssetSetup()` (`AngelscriptCodeModule.h:45`) as the
snapshot seam. Rev 2 uses scratch re-init as primary instead, because CkFoundation's Phase-2
deferred-asset heal re-runs `__Init_` after load, which would stale a load-time snapshot. The
delegate is recorded as the alternative.

### 2. Stale-snapshot write could revert a saved source edit (plan §5.2)

The patch is computed from a file read at button press; the confirmation dialog can stay open for
minutes; the write is a whole-file replace. A VS Code *save* in that window (distinct from the
unsaved-buffer conflict rev 1 already covered), or a watcher-driven reload that replaces the live
object via the `REPLACED_ASSET_` path (`Bind_UObject.cpp:420-427`), would be clobbered by content
derived from the stale snapshot — failing the plan's own Trap B standard.

**Resolution (rev 2):** confirm-time freshness guard added to §5.2 — after the user confirms,
re-read and verify the on-disk bytes still match the snapshot; if not, discard and restart the diff.

---

## Non-blocking suggestions — all applied in rev 2

1. **U1 settled**, removing it as a risk: `TSoftObjectPtr` has null-handle `ImplicitConstructor` and
   `opAssign(T handle_only Object)` binds (`Bind_TSoftObjectPtr.cpp:442`, `:454`), `TSoftClassPtr`
   likewise (`:590`). Now plan Fact 20.
2. Fact 6 corrected — `AssetEditor.<ClassName>Editor.ToolBar` holds only for simple asset editors
   with one edited object; `GetToolMenuAppName()` otherwise falls back to `GetToolkitFName()`.
   Design unaffected (we hook the parent), wording fixed so nobody keys off the per-editor name.
3. Enable-condition cost — a toolbar enabled-attribute is a polled `TAttribute`. Now specifies
   caching with a dirty flag off `FCoreUObjectDelegates::OnObjectPropertyChanged`.
4. Test gate expanded: CRLF/LF and BOM preservation, braces inside strings/comments, commented-out
   `asset X of Y`, duplicate property assignment, trailing-comment preservation, property-name
   prefix collision (`_Mesh` vs `_MeshScale`), empty-body insert, duplicate asset path across
   generated files, plus a pair-recursion test tier.
5. `FText` — plain-literal emission documented as accepted lossy behaviour (loses localization
   namespace/key), surfaced in the confirmation dialog.
6. File layout specified, mirroring `SelfHeal/`; namespace `ck::angelscriptgenerator::write_back`.
7. Optional hoist of the pair-walker into `CkReflection_Utils` — recorded as optional, not mandated.
8. Both-fail outcome of the file-resolution chain stated explicitly (loud abort, file untouched).

Also folded in: asset-name uniqueness is enforced **globally across modules**, not per module
(`AngelscriptPreprocessor.cpp:3974-3999`) — stronger than rev 1 claimed.

---

## Spot-checks performed by the reviewer

All ~18 rev-1 facts were checked at their citations. **None refuted**; one understated (global vs
per-module uniqueness), one over-generalised (Fact 6). Files opened included
`AngelscriptPreprocessor.cpp`, `Bind_UObject.cpp`, `Bind_TSoftObjectPtr.cpp`,
`AngelscriptEditorModule.cpp`, `AngelscriptManager.{h,cpp}`, `AssetEditorToolkit.cpp`,
`CkReflection_Utils.cpp`, `CkAssetRegistrySubsystem.{h,cpp}`, `CkAssetReferenceProvider.h`,
`CkAngelscriptGenerator_StubSynthesizer.cpp`, `Test_StubSynthesizer.cpp`, `CkAutoTestMapPopulator.cpp`,
the `.as` sources cited for Facts 14-16, the Build.cs files for Facts 17-18, and
`Script/ARCHITECTURE.md` §13.

Independently re-verified by the plan author before applying: `Bind_TSoftObjectPtr.cpp:442,454`
(U1) and `AngelscriptCodeModule.h:45` (`GetPostLiteralAssetSetup`).

---

## Design observations retained

- The three-stage decomposition survives the fix intact — only stage 1's comparand changed. Stages
  2/3, the loud-skip taxonomy, the §5.4 pair-recursion, and the deferred-item seams all carry over.
- Home in `CkAngelscriptGenerator` is correct. The registry-inversion fallback is **not** a better
  long-term shape: write-back is irreducibly AS-specific, and routing it through CkCore would
  generalise an interface with exactly one conceivable implementor. Kept as documented fallback.
- The rejected UI alternatives are rejected for the right reasons; dynamic-section-on-parent is how
  the engine extends its own asset-editor toolbars in this fork.
- Settled decisions §9.1, §9.2, §9.4, §9.5, §9.7 all sound against the code. §9.6 (all-or-nothing)
  survives — the predicate fix re-scopes the universe it quantifies over to where it is coherent.
