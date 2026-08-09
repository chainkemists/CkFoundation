# Phase 2 — the ButtonId map: stable button identity derived from EI resolved mappings

> **Status:** ✅ CLOSED (2026-08-08, same day) — scoped gate 28/28 (all 6 `InputButtonMap`
> tests by name), doc subsection landed, comment audit clean. Full-suite delta-zero DEFERRED
> to campaign end per [P2-D4] (maintainer directive; entry baseline `1027/1025/2` remains the
> final diff anchor). **Depends on:** Phase 1 (✅) + 1b (✅); the 0F gate is settled
> by [P2-D1] below.
> **Scope of record:** PROMPT.md phase-index row 2: "ButtonId map derived from EI resolved
> mappings; stability, uniqueness, re-derive on `OnSettingsChanged`; two-tier button space."
> **The original design PROMPT that defined "two-tier button space" was superseded and its file
> lost (uncommitted).** The button model below is a RECONSTRUCTION ruled by the orchestrator from
> the surviving references (PROMPT.md:23, DESIGN_InputLayering.md:79+:224, D16, success
> criterion 4) — flagged for maintainer review like [P1-D4].

## Rulings at phase open

- **[P2-D1] 0F is settled by the maintainer's resume directive** ("move on to the next
  phase(s)", 2026-08-08). The 0F question was whether CkIntent deriving a ButtonId map from EI
  resolved mappings and re-deriving on `OnSettingsChanged` conflicts with the CkGameSettings
  keybinding page. The maintainer owns both campaigns; the map is a READ-ONLY consumer of the
  same store the page writes through, and `OnSettingsChanged` is the existing broadcast channel
  both already share (the page's writes are exactly what the re-derive listens for). No conflict
  identified. The defect-escalation list to the CkGameSettings context (1-5, 7; #13 now FIXED)
  stands on the human queue.
- **[P2-D2] The button model (the "two-tier button space" reconstruction):**
  - A **ButtonId is a stable identity for a pressable thing**, decoupled from the physical
    `FKey` that currently produces it. Identity is `(Tier, FName)` — NOT a dense int. Dense
    packing for ring-buffer bitmasks is the compiled set's job at bake time (Phase 4, D8),
    where the set knows exactly which buttons it references. *Revisit if Phase 3's frame
    record needs dense global indices before Phase 4 exists.*
  - **Tier 1 — Mapped:** one ButtonId per EI player-mappable MAPPING NAME (the FName the
    settings store keys on — stable across rebinds by construction). Its FKey association
    follows the player's resolved mappings and is re-derived on `OnSettingsChanged`. This is
    what makes success criterion 4 (rebind moves the intent with no definition edit) possible.
  - **Tier 2 — Physical:** one ButtonId per raw `FKey` with no EI mapping (identity = the
    key's FName). Fixed association, never touched by re-derive. This is the D16 /
    prototyping / synthetic-test tier, and the honest home of anti-pattern-#13 consumers.
  - **Stability:** an identity, once derived or registered, never changes for the map's
    lifetime. Re-derive updates FKey↔ButtonId ASSOCIATIONS only, never identities. Cross-map
    determinism falls out of name-keyed identity (no assignment order to drift).
  - **Uniqueness:** one ButtonId per (tier, name). The FKey→ButtonId direction is
    **one-to-many by design** — two mappings legitimately share a key (different categories),
    and duplicate bindings exist in the wild (defect #13's residue proved it). The lookup
    returns ALL ButtonIds for a key; a consumer wanting "the" button must say which tier/name.
- **[P2-D3]** The map is a feature composed on the input-SOURCE entity (mimic `InputBias`:
  `Add` only, no `Create` — a map on a child entity would have no player identity). Opt-in in
  v1; Phase 3 composes it when it composes the sampler.

## The unit (single dispatch)

**Feature `InputButtonMap`** on the input-source entity (quartet, mimic the landed `InputBias`
shapes): ParamsData (tier-2 physical-button declarations may ride it; tier-1 derives from the
live profile, not params); a current fragment holding identity→association both ways; deferred
requests `Request_Rederive` (tier-1 refresh from the resolved mappings) and
`Request_RegisterPhysicalButton` (tier-2, idempotent per house Result rules); a processor in
`FGroup_Input_Collect`-adjacent ordering (before Route — consumers of routed input may resolve
buttons same-frame; exact group placement is the unit's proposal, STOP on a cycle). Re-derive
trigger: the engine seam from `OnSettingsChanged` into a deferred request — proposal owed by
the unit within D25 (the existing `ULocalPlayerSubsystem`s are the permitted seams; a NEW
subsystem is a STOP). Utils queries: `Get_ButtonIdsForKey` (all matches), `TryGet_KeyForButton`
(tier-1: current resolved key; tier-2: the fixed key), `Get_AllButtons`.

→ **verify (AutoTests, using the 1a-0 test assets):** derive-on-add produces tier-1 identities
for all 4 authored mapping names with default-key associations; rebind (`RemapKey`) then
re-derive moves the association while the identity compares equal to the pre-rebind capture;
4 names → 4 distinct identities; two mappings remapped onto ONE key → key lookup returns both;
tier-2 registration is stable, fixed, and untouched by re-derive; re-derive/edit visibility
follows the house deferred-request contract (assert the boundary, mimic
`CaptureEditLandsNextFrame`).

## Exit criteria

- [ ] Full suite green + delta-zero vs the Phase-2 entry baseline + the new test names
- [ ] `CkInput/Claude.md` gains one subsection documenting the button space (the contract
      Phase 3's sampler consumes)
- [ ] PROGRESS decision log + dated entry current; comment audit run

## NOT in this phase

No sampler/ring/frame record (Phase 3), no octant/hysteresis/SOCD (Phase 3), no wiring of
layer CAPTURES to ButtonIds (DESIGN_InputLayering milestone 2 — lands when captures gain a
ButtonId match mode, Phase 3+), no dense index assignment (Phase 4 bake), no persistence of
the map (identities are derivation-deterministic; there is nothing to save), no new
subsystems.
