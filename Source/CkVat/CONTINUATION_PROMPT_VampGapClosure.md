# CONTINUATION — CkVat: verify passes + VAMP gap closure

> **Written:** 2026-07-10. **Dies when:** the gap items below land (or are re-chartered in
> [PLAN.md](PLAN.md)) — then delete this file; PROGRESS.md is the living log, this is a snapshot.
> **Read order for zero context:** this file → [PROMPT.md](PROMPT.md) (mission + locked decisions)
> → [PROGRESS.md](PROGRESS.md) (current state, evidence trail) → the two contracts:
> [Plan/Gate_01_Bake.md](Plan/Gate_01_Bake.md) (texture layout) and
> [Plan/Gate_02_Material.md](Plan/Gate_02_Material.md) (per-instance 12-float slots).

**One-liner:** CkVat (vertex-animation-texture support, VAMP-informed) is code-complete through
Gates 0-3 with Gate 4 opened; the next session runs/reacts to the human [EDITOR-VERIFY] passes and
closes the audited VAMP-parity gaps listed below, in priority order.

## Repo state (verified 2026-07-10, all UNPUSHED)

- **CkFoundation** `feature/vat-feature` @ `27a588872`, 18 commits over dev base `545be1a53`, tree
  clean. Highlights: `f69f9095a` shared `ck::anim_bake` core (CkAnimation/AnimBake) · `08940084c`
  Iskm bake refactored onto it · `eb2087ee3`/`07a437601` CkVat+CkVatEditor modules · `6575ac566`
  baker · `4b0c51bf5` CkUsf WPO extension (UV1/UV2/LocalPosition/instance basis) · `9540e2cca`
  Vat looks + generated masters (committed Content) · `6e9752a60` ISM transient-factory
  custom-data overload · `e17c489a1` runtime hookup · `660b59b13` FMemMark fix · `06116ea56`
  Vat→VatProxy rename · `27a588872` CkIskmRenderer style sweep.
- **CkTests** `feature/vat-feature` @ `71af417`, 3 commits over dev `3b06550`, tree clean:
  AnimBake unit tests + `Ck_AutoTest_VatProxy_ApiSurface` + OFPA churn/prune.
  **MERGE ORDER CONSTRAINT: CkTests branch references `utils_vat_proxy` — it must NOT merge to
  CkTests dev before CkFoundation's branch merges** (would break everyone's AS compile).
- Test evidence on these exact tips: Iskm suite **30/30** (`--test-pattern "IskmRenderer"
  --no-nullrhi`), AnimBake unit tests **2/2**, VatProxy_ApiSurface **1/1**.
- Sibling repo context: `origin/feature/perf-iskm-lod` (another session's branch) edits the same
  CkIskmRenderer files the style sweep rewrote — its merge WILL conflict there; resolution =
  re-apply the style rules to their side. BB superproject untouched by this campaign.

## What the next session picks up (priority order)

1. **React to the human [EDITOR-VERIFY] passes** (steps live in Gate_01/Gate_03 docs):
   bake a `UCk_VatCollection_Data` for a Mannequin/BB mesh via `UCkVat_BakerSubsystem::
   Bake_VatCollection`, then spawn+play via `utils_vat_proxy`. Match outcomes against the branch
   table below. Until this happens, all visual claims are unverified.
2. ~~**Root-motion bake toggle**~~ **DONE 2026-07-10** (+ `_DisableRetargeting` for Iskm parity).
3. ~~**Bake UX button**~~ **DONE 2026-07-10** — details-panel Bake/Rebake button + bake-staleness
   detection (`_BakedInputsHash` / `Get_IsBakeStale` / IsDataValid warn-unbaked, error-stale).
4. ~~**VAT normals in the pixel shader**~~ **DONE for VERTEX mode 2026-07-10 — but NOT as sketched
   here**: the PS has no per-instance basis outside Nanite (verified in MaterialTemplate.ush), so
   the baker now encodes `_Nrm` in the bind-pose TANGENT frame (instance-invariant, feeds the
   Normal pin directly); generator gained opt-in `_PixelDataChannels`. **Bone-mode normals
   DEFERRED** — the quat rotation needs a shared frame the PS can't build; options recorded in
   Gate_02 "Deferred" (quat in custom-data slots 12-15 / local-TBN mesh channels / Nanite
   InstanceId). Do not re-attempt the "rotate In.VertexNormal by the blended quat" idea — wrong
   under instance rotation.
5. **Bone-influence options (1/2/4) + weight-TEXTURE storage** (VAMP parity; the storage option is
   the Nanite prerequisite): collection fields + baker paths + look variants.
6. **Nanite investigation gate** (unknowns — do NOT promise before verifying against the 5.7 fork
   source): WPO-on-Nanite limits, `PreSkinnedPosition` on Nanite, per-instance custom data on
   Nanite ISM, whether the lookup-UV survives Nanite clustering. VAMP's recipe: Bone mode +
   texture weights + special bake + tangent-space-normals off.
7. **Minor gaps** (each small, take opportunistically): Ultra 32-bit precision (check
   `TSF_RGBA32F` exists in the fork); configurable max texture sizes (currently
   `MaxTextureWidth=4096`, `MaxTextureRows=8192` constants in `CkVatBaker.cpp`); modular
   characters pattern (one VatProxy per entity today — document child-entity composition);
   morph-target baking (out unless BB needs it); explicit velocity data ONLY if verify shows
   TAA ghosting.
8. **End-to-end playback autotest** (chartered Gate 4): needs ONE editor-baked Mannequin
   collection committed as CkTests content (bake saves packages → unsuitable per-test-run), then
   a bake→play→OnClipFinished autotest + a CkVat gym.

## Branch table — [EDITOR-VERIFY] outcomes

| Observation | Probable cause | Where |
|---|---|---|
| Baked mesh renders inside-out | Triangle winding (source index order kept) | swap corners 0/2 in `BuildBakedStaticMesh`, `CkVatEditor/CkVatBaker.cpp` |
| Mesh renders but does not animate | Master material missing/stale, or custom data not reaching GPU | re-run `Ck_Usf_GenerateLooks`; confirm renderer Mobility=Movable (`CkVat_Subsystem.cpp` — Movable is load-bearing: ISM pushes custom data on the Movable path only) |
| Anim plays at wrong speed / completion doesn't match pose freeze | CPU (`Get_WorldTime`) vs GPU (`Time` material node) clock basis mismatch | `CkVatProxy_Processor.cpp` + material Time node semantics; both worst-case fixable by switching the packing to a shared basis |
| Pose distorted on reoriented-import meshes | Mesh bind pose ≠ skeleton ref pose (dev semantics kept deliberately) | fold perf-iskm-lod's mesh-bind fix into `ck::anim_bake` call args (decision log in PROGRESS.md) |
| Ghosting/smearing under TAA on fast clips | WPO velocity not applied (`r.Velocity.EnableVertexDeformation`) | if confirmed → explicit prev-frame velocity work (gap 7) |
| Bake ensure fires | Read the ensure — they're specific (vert count > 4096 in Vertex mode, rows > 8192, missing skeleton bone). NOTE: >4 influences is NO LONGER an ensure (2026-07-10) — it's a one-line Display summary (Mannequin content legitimately has 5-influence verts; the harness escalates warnings/ensures) | `CkVatBaker.cpp` |

## Critical files

- `Source/CkVat/Public/CkVat/Proxy/CkVatProxy_*.{h,cpp}` — feature quartet: playback state
  machine, requests, `FProcessor_VatProxy_{Setup,HandleRequests,FireSignals}`, float packing
  (`ck_vat_proxy_processor::Pack_CustomData`).
- `Source/CkVat/Public/CkVat/CkVat_Subsystem.{h,cpp}` — per-collection shared MID + transient ISM
  renderer; `NumPerInstanceFloats = 12`.
- `Source/CkVat/Public/CkVat/Collection/CkVatCollection_Data.{h,cpp}` — asset shape incl.
  serialized clip table + `ApplyBakeResults`.
- `Source/CkVatEditor/Public/CkVatEditor/CkVatBaker.cpp` — the whole bake (both modes).
- `Source/CkVat/Shaders/CkVat/Vat.ush` + `Script/CkVat/CkVat_Looks_Assets.as` — decode + look
  defs (param order == slot contract; masters in `Content/CkUsf/GeneratedLooks/M_CkUsf_Look_Vat*`).
- `Source/CkUsfEditor/.../CkUsf_Generator.cpp` — WPO wiring extension lives here; PS extension
  (gap 4) mirrors it.
- `Source/CkAnimation/Public/CkAnimation/AnimBake/CkAnimBake.{h,cpp}` — shared sampling core
  (owns its FMemMark; unit-tested in CkTests).
- `Plugins/CkTests/Script/CkVat/CkAutoTest_VatProxy_ApiSurface.as`,
  `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkAnimation/Test_AnimBake_Core.cpp`.

## Load-bearing contracts (one change = all sides)

Float packing (`Pack_CustomData`) ⇄ AS look per-instance declaration order ⇄
`UCk_Vat_Subsystem_UE::NumPerInstanceFloats` ⇄ Gate_02 table ⇄ `Vat.ush` decode. Special encodings:
**Rate==0 ⇒ the Time float holds the frozen clip-local time**; `RowCountB==0` ⇒ no fade source;
bone-mode weight3 = `1 − r − g − b` (VertexColor is float3 in the VS input).

## Ruled out — do not re-investigate (evidence in PROMPT.md + PROGRESS.md)

VAT inside the batched Iskm stack · custom vertex factory for v1 · replicating playback state
(renderer doctrine: gameplay owner re-drives) · waiting on/merging `perf-iskm-lod` (v1 needs
nothing from it) · per-entity MIDs (defeats instancing — shared MID per collection is deliberate)
· renaming the serialized `_b*` reservation UPROPERTYs on the Iskm AnimCollection (needs
CoreRedirects + that branch's coordination) · style-sweeping `CkIskmRendererVF` (no CkCore dep by
design — cannot use `ck::IsValid`/`NOT`).

## Gotchas accumulated (would be re-learned the hard way)

- Toolbox `--test` caches its list: **new/renamed tests are silently skipped without
  `--discover-fresh`** (a dev test was cache-hidden for a whole day of runs — baseline said 29,
  truth was 30).
- **Never run git ops touching `.as` while a test editor is live** — CRLF mtime churn triggers
  "AS Soft Reload during PIE", which the automation controller escalates to a test failure.
- Headless editor-cmd: `Quit` in `-ExecCmds` does NOT exit — use `QUIT_EDITOR` (an idle editor
  held DLL locks → LNK1104 for an hour). AND: `-ExecCmds` splits on **commas, not semicolons** —
  `-ExecCmds="Cmd A; QUIT_EDITOR"` is parsed as ONE command with a `A;` arg (nothing runs, editor
  idles); `-ExecCmds="Cmd A, QUIT_EDITOR"` runs both and exits cleanly (verified 2026-07-10).
- `CK_REGISTER_PROCESSOR` needs `CkEcs/Scheduler/CkProcessorRegistration.h`; editor-twin modules
  must list `CkEcs` in Build.cs (LNK2019 on SharedPCH otherwise); unnamed-enum constants are not
  fmt-formattable in `CK_ENSURE` (cast to int32); `TWeakObjectPtr` member init needs the DEFINING
  include of the pointee.
- Pose evaluation (`FCompactPose`) asserts without an enclosing `FMemMark` — the shared core now
  owns one per sequence (`660b59b13`); don't remove it.
- The repo `.gitignore` blanket-ignores `*.md` — campaign docs are force-added (`git add -f`),
  matching tracked-module-doc precedent.
- The AutoTests populator re-keys OFPA actor files on discovery runs — commit the delta with the
  test change (repo precedent: the "prune orphaned external actors" chores). `AutoTests_BB_MAP`
  has PRE-EXISTING orphaned actor refs (BB-side noise, not ours).
- `Get_LookMasterMaterial`-by-path (outline-subsystem precedent) is how runtime reaches generated
  masters; look defs are script-declared (in-memory, no .uasset).

## Suggested first message (for the human running the next session)

> I'm continuing the CkVat campaign. Read
> `Plugins/CkFoundation/Source/CkVat/CONTINUATION_PROMPT_VampGapClosure.md` fully first, then
> PROMPT/PROGRESS in the same folder. I've [run / not yet run] the [EDITOR-VERIFY] passes — here
> are my observations: [bake results / playback observations or "not yet"]. Work the priority
> list from the continuation prompt; match any visual anomalies against its branch table before
> changing code.
