# Gate 5 — Ergonomics

> **Status:** 🟡 In progress (entered 2026-08-01 — Adam approved the Gate 4 exit).
> **Depends on:** Gate 4 ✅
> **Estimate:** re-date as work items land.

## Goal (brief §9, phase 5)

After this gate: `data-ck-*` handling end to end, a live-reload preview path, and a conversion
report UI — proven by converting **a non-trivial screen from an actual project** (Grimveil or
Rewind 99) and wiring it to real gameplay data.

## Work items (draft)

1. **Conversion report** — the IR's `unsupported[]`/`diagnostics[]` surface as a typed report on
   import (struct on the PageAsset + log summary); no silent drops end-to-end (brief §1 rule 5).
2. **`data-ck-bind` / `data-ck-slot` consumption** — DATA_CK_SPEC.md's remaining attributes
   (only `name` is consumed today); binding surface design against a real consumer.
3. **AS consumption script** (inherited from Gate 4) — a CkTests-tree `.as` exercising
   PageAsset + WidgetsByCkName; closes the three-environments partial.
4. **Live-reload preview** — watch the `.html`, re-extract, rebuild the preview widget in-editor
   ([EDITOR-VERIFY] heavy; extraction is a Node process the editor must spawn).
5. **Editor UX** (inherited) — import context-menu/factory over the programmatic entry point.
   Factory + reimport handler VERIFIED 2026-08-02 (58/58 incl. FactoryReimport). Tools-menu entry
   ("Tools → CkWebUmg → Import Web Page…", file dialog → html/json import into `/Game/WebUmg`)
   code landed 2026-08-02 — registration compiles; the click itself is [EDITOR-VERIFY]: open the
   Tools menu, pick a corpus .html, confirm the asset appears. `<title>` now captured
   extractor→IR→asset (`_PageTitle`, display metadata; naming stays the collision-safe basename).
6. **Real-screen conversion [BLOCKED on Adam]** — needs the actual project mockup supplied.

Added 2026-08-02 from the UIBridge reference intake (PriorArt §7, DECISIONS entry same date):

7. **Hit-test defaults — ✅ DONE 2026-08-02 (59/59):** decorative nodes (no `data-ck-name`/`bind`)
   build as `SelfHitTestInvisible` (subtree stays traversable), named/bound nodes stay `Visible`,
   `visibility:hidden` wins. Enforced page-wide by `CkTests.UnitTests.CkWebUmg.HitTestDefaults`
   (smoke corpus; vacuous-pass guarded — requires ≥1 node of each class).
8. **Validation pre-pass** — a read-only validate mode over the IR (loader + report machinery
   already exist): errors block import, warnings don't, never touches the project.
9. **Live-reload debounce spec** (folds into item 4 when built) — wait for a quiet period after
   the last file write (extractor writes textures before the json); the live path always
   regenerates read-only output, so a save can never destroy work (D3 makes this free for us).

## Exit criteria

- [ ] A real project screen converts, renders within the ratified thresholds, and wires to
      gameplay data through data-ck-* — demonstrated, not asserted.
- [ ] Conversion report shows every dropped/unsupported property for that screen.
- [ ] Docs + PLAN row + this header, same commit; PROGRESS dated entry.
