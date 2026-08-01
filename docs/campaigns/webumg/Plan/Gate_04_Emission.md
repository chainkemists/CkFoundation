# Gate 4 — Emission

> **Status:** 🟡 In progress (entered 2026-08-01 — Adam approved entry after the Gate 3 close). Prepared 2026-08-01 so the Gate 3 exit conversation can see
> the next gate's shape; every section below is revisable at entry, and entry itself requires
> Gate 3 closed by Adam (brief §1 rule 2).
> **Depends on:** Gate 3 ✅ (pending Adam's four exit calls)
> **Estimate:** re-date at entry.

## Goal (brief §9, phase 4)

After this gate: an **editor module** converts a `*.ckui.json` bundle into the DECISION 2 output
form — a **reflected DataAsset** consumed by a runtime widget builder — with DECISION 3 read-only
regeneration. Exit: **round-trip** (convert → open in editor → render → pixel-diff against the
golden) stays within the Gate-3-ratified threshold, and regeneration is **idempotent** (re-convert
of an unchanged page produces an identical asset; duplicate `data-ck-name` is a hard emit error).

## Entry criteria (fill at entry — do not assume)

- [ ] Gate 3 exit checklist fully closed (threshold, shadows, text tolerance, §8 policy — Adam).
- [ ] Baseline re-captured on entry HEAD (current: 908/908 full suite; 33 pattern-lane tests).
- [ ] Re-read DECISION 2/3 wording in DECISIONS.md before shaping the asset — do not re-derive
      from memory.

## Work items (draft)

1. **Reflected IR asset form** — `UCkWebUmg_PageAsset` (name TBD against house conventions:
   `_DA` suffix rules, `CK_PROPERTY` encapsulation, three-environments rule now attaches — the
   Gate 2 exit deviation note says BP/AS surface arrives HERE). The plain-aggregate IR structs
   stay the loader's form; the asset form is a projection, not a replacement.
2. **Importer/factory (editor module `CkWebUmgEditor`)** — `.ckui.json` (+ `ckui-assets/`) →
   PageAsset + imported `UTexture2D`s. Baked-texture strategy (gradients/shadows/borders) must
   decide its asset form here: bake-at-import into persistent textures vs bake-at-build transient
   (today's behavior). Present both with size/fidelity numbers — likely [DECISION]-shaped.
3. **Runtime consumption** — builder path from PageAsset (not just loader structs); UMG wrapper
   (`UCk_WebUmgPage_UE`?) hosting the Slate tree; the harness gains an asset-round-trip lane.
4. **`data-ck-*` emission surface** — named-widget lookup (`data-ck-name` → widget), bind/slot
   surface per DATA_CK_SPEC.md; duplicate names hard-fail the import (D3).
5. **Read-only regeneration (D3)** — regenerated assets are not hand-editable; re-import
   idempotence proven byte-wise or property-wise; a change-detection story (source hash on the
   asset).
6. **Round-trip harness** — convert corpus → build from ASSETS → pixel-diff vs goldens at the
   ratified threshold; idempotence test (double-import, diff).

## Known forks to surface early (not decide silently)

- Baked textures: import-time persistent vs build-time transient (memory vs fidelity-simplicity).
- Asset granularity: one PageAsset per page vs shared style/texture assets.
- Where the emitter lives: `CkWebUmgEditor` module in CkFoundation vs a standalone plugin.
- Font config: the harness's OS-face mapping is machine-local by design; shipped assets need a
  declared font asset mapping (emitter config per SCHEMA.md's `fonts` note).

## Exit criteria (draft — finalize at entry)

- [ ] Corpus round-trip through ASSETS within the ratified thresholds (pixel + text).
- [ ] Regeneration idempotent; duplicate `data-ck-name` hard-fails with a diagnostic naming both nodes.
- [ ] Three-environments rule satisfied for the new public surface (C++/BP/AS) — this is the gate
      where the deferred Gate-2 deviation comes due.
- [ ] Module Claude.md rows + tier-table entries for any new module; docs same-commit rule.
