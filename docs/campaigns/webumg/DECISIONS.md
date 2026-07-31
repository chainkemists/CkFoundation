# DECISIONS.md — CkWebUmg campaign

## DECISION 0 — Build vs. buy — **DECIDED: BUILD** (Adam, 2026-07-31)

Phase 0 recommendation accepted as presented ([PriorArt.md](PriorArt.md) § Build vs. buy): build per the brief's phase plan with scope reductions R1 (CDP extraction), R2 (vendor Yoga if DECISION 1 → C/D), R3 (buy one WebToUMG license as measured baseline — **Adam's purchase, still open**; feeds the Gate 2 threshold argument). The full pre-decision option analysis and movers are preserved in the Phase 0 version of this file at `D:\tmp\ckstyle-phase0\DECISIONS.md` and in PriorArt.md — not restated here.

## Naming — **DECIDED: CkWebUmg family** (Adam, 2026-07-31)

`CkWebUmg` (Runtime) + `CkWebUmgEditor` (UncookedOnly); CLI `ckwebumg-extract`; campaign dir `docs/campaigns/webumg/`. Chosen over CkCss/CkMockup/CkFlexUi. Reason `CkStyle` was unavailable: collides with `namespace CkStyle` (`Source/CkEditorTools/Public/CkEditorTools/Style/CkStyle.h:32`) and `CkStyleSettings.h`; `CkEditorStyle` module also exists (`CkFoundation.uplugin:370`). Harness module placement (CkTests plugin vs dedicated) is a Gate 2 decision — no `*Harness` module precedent exists.

## DECISION 1 — Layout runtime — **OPEN, due at Gate 1 exit with evidence**

Options A (bake to canvas) / B (lower to native panels) / C (Yoga-backed `SCkWebUmgFlexPanel`) / D (C + lowering pass) per brief §4. Evidence on file: Slate/Yoga measure-model reconciliation (PriorArt §3 — C's headline risk survived recon on paper; compiled proof pending); WebToUMG ships A/B-style lowering with known costs (single viewport, baked) ⟨V⟩. Gate 1 adds: IR-driven layout fixtures both engines must satisfy.

## DECISION 2 — Output form — **OPEN, due at Gate 1 exit**

Generated WBP assets / generated C++ UUserWidget / runtime build from DataAsset IR (brief §4). Evidence to gather in Gate 1: what the IR makes cheap or expensive per option.

## DECISION 3 — Regeneration model — **OPEN, due at Gate 1 exit**

Proposed: generated assets strictly read-only (`WBP_Foo_Generated`), hand-authoring in subclass/bound C++. Corroboration: WebToUMG independently converged on regenerate+preserve-logic+stable-names, merge "on roadmap" ⟨V⟩.

## DECISION 4 — v1 CSS surface — **OPEN, due at Gate 1 exit**

Brief §4 proposal is the working set (flexbox, box model, backgrounds/borders/radii, typography, images, absolute/relative, overflow, opacity, transform, :hover/:active/:disabled in; Grid, inline mixed boxes, pseudo-elements, animations, filter, clip-path, blend modes out — with diagnostics). Gate 1's extractor implements the diagnostic mechanism against this proposed set; corpus findings may amend it before ratification.

## Standing evidence notes (carried from Phase 0)

- Yoga vendoring cautions: pin `c766885`; `UseWebDefaults` on; never expose inert grid setters; extractor pre-sorts children by computed `order`; fix `pointScaleFactor` for deterministic harness comparisons (PriorArt §3).
- Engine CEF 128 as CDP backend: **probe done 2026-07-31** — feasible: `-cefdebug=<port>` enables CDP remote debugging (`WebBrowserSingleton.cpp:343-347`). Requires a running editor process hosting an offscreen SWebBrowser, so heavier than system Chrome; not adopted for Gate 1. Revisit only if the Node toolchain becomes a real friction (e.g. designer machines without Chrome/Node).
- Chrome-version drift: extraction rides installed Chrome (auto-updates); IR must record browser version + viewport + dpr; determinism claims are per-browser-version (Gate 1 risk table).
