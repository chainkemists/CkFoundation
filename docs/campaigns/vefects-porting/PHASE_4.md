# Phase 4 — Light renderer + mesh facings + palettes → the explosion family

**Goal:** land C7 (light renderer), C8 (mesh facing modes + renderer-level mesh scale), the
FresnelBomb family, and the [C-D3] palette mechanism; then port ExplosionGround (**40**),
ExplosionOmni (**41**) — each carrying fire + ice palettes — and Bomb_Explosion (**42**).
Five new A/B pairs (Ground-fire, Ground-ice, Omni-fire, Omni-ice, Bomb) from three behaviors.

**Entry criteria:** Phase 3 closed (PROGRESS 2026-08-02: Particles 30/30, 25 templates,
24 pairs); baseline = those counts.

## Capability contracts (decisions MADE)

### C7 — light renderer kind
`ECk_ParticlesRenderer_Kind::Light` on the renderer spec (usable in RendererOverrides,
VisTag-gated — VERIFY first that `UNiagaraLightRendererProperties` carries
`RendererVisibilityTagBinding` in the 5.7 fork; if it does NOT, the [P3-D1] second-emitter
precedent applies: lights ride a dedicated LIGHT EMITTER SECTION mirroring the ribbon spec —
report which branch reality takes, with header citations, before implementing). Radius from
`Size.x`, color from `Particles.Color` (HDR carries intensity — the pack's light layers drive
brightness through color magnitude). No look/material (lights have none) — `LookName` must be
NAME_None for Light rows; invariant in RosterSanity.

### C8 — mesh facing modes + renderer mesh scale
`FCk_ParticlesRendererSpec` gains `MeshFacingMode` (enum mirroring Niagara's: Default,
Velocity, CameraPosition) and `MeshScale` (FVector, default (1,1,1)) — trailing members,
defaults preserve all existing aggregates. Builder maps them onto
`UNiagaraMeshRendererProperties` (verify the 5.7 property names: FacingMode, and the
renderer-level mesh scale on the mesh array entry or renderer — cite headers). The batch-B/G
in-behavior orientation compromises stay as-is (recorded §13); new explosion rows use the real
modes.

### FresnelBomb family
`FresnelBomb.ush` per the Bomb_Explosion sheet's §4 (three corpus instances): rim-lit
translucent bubble (fresnel power/intensity, tint, ParticleColor modulation). Smallest faithful
form; three look parameterizations per the delta table. Opt into the mesh-particle flag.

### [C-D3] palette mechanism
Per-layer palette TABLE in the behavior: `Get_<Effect>Palette(int32 PaletteId, int32 Layer)`
returning the layer's color keys — fire = palette 0, ice = palette 1, selected by a NEW user
param `User.PaletteId` wired like BehaviorId (template Map Get → DI input? NO — avoid a DI
signature change: the palette id rides the EXISTING BehaviorId space instead. RULING
[P4-D1]: ice variants get their OWN behavior ids (43 = GroundIce, 44 = OmniIce) whose behaviors
are thin wrappers — same layer code, palette table swapped — sharing every constant except the
color keys via a common include (`Behavior_ExplosionShared.ush` idiom, mirrored on CPU). This
keeps the one-int spawn contract, the roster-driven tests, and the gym pair registry unchanged
in shape. [C-D3]'s "two behaviors" becomes "two behaviors + two thin palette twins" — the
implementation-sharing intent is preserved by the shared include, and the A/B gym still gets
its four pairs.)

## Ports

- **Batch H (40–44):** ExplosionGround + GroundIce twin, ExplosionOmni + OmniIce twin (shared
  include per [P4-D1]; the fire/ice §5 key sets from the sheets' palette tables), then
  Bomb_Explosion (42→45 renumbering NOT allowed — ids are: 40 Ground, 41 GroundIce, 42 Omni,
  43 OmniIce, 44 Bomb_Explosion; allocate in THIS order). Ground/Omni carry the event-driven
  ribbon (E2 corpus data + C6c), the light layer (C7), CameraPosition-facing bubble meshes
  (C8 + FresnelBomb), sphere/spike meshes (exist). Bomb_Explosion is the 162-burst giant —
  verify the row builds before porting math (its sheet's §6 note).

## Exit criteria
Capability gates green on unchanged counts; batch H: lanes green (Particles 30→35), templates
non-inert (dual-emitter greps where ribbon/light emitters exist), pairs staged (24→29 — five
new); recipes §7–14; PROGRESS current. Phase closes with all five pairs gated.

## Fences
- No Lightning_Hit work (Phase 5).
- DissolveAdd semantics, behaviors 0–39, existing looks/rows: untouched.
- Palette twins share via include — duplicated layer math across fire/ice files is a defect.
- Anything unenumerated → STOP into PROGRESS Blockers.
