# CkVat — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-07-10 (branch `feature/vat-feature`, base `545be1a53`, NOT pushed):** Gates 0-3
code-complete, Gate 4 open (first tests landed in CkTests `feature/vat-feature` — merge-order
constraint: CkFoundation first). **VAMP gap closure in progress** (priority list in
CONTINUATION_PROMPT_VampGapClosure.md): gaps 2 (root-motion/retargeting bake toggles), 3 (Bake
button + bake-staleness detection), and 4-vertex (tangent-space VAT normals in the pixel shader)
landed 2026-07-10; gap 4-bone (bone-mode normals) DEFERRED — no per-instance basis in the
non-Nanite PS (evidence in Gate_02 Deferred).
**Baseline on record:** Iskm autotests **30/30** + AnimBake 2/2 + VatProxy_ApiSurface 1/1 + Usf 4/4
(2026-07-10 logs: Test-{Iskm,AnimBake,VatProxy,Usf}.log).
**Next action:** the human [EDITOR-VERIFY] passes (bake a real collection via the NEW details-panel
Bake button, spawn+play via `utils_vat_proxy`, check lighting responds to animation in Vertex mode),
then gap 5 (bone-influence options + weight-texture storage).
**Blocked on:** [EDITOR-VERIFY] (human) for all visual claims; Gate-4 end-to-end autotest needs one
editor-baked collection committed as CkTests content.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-09 | New CkVat+CkVatEditor; both bake modes v1; ISM+CkUsf playback vehicle | Maintainer picks (see PROMPT locked table) | — |
| 2026-07-09 | Shared core home = CkAnimation/AnimBake, namespace `ck::anim_bake` | Semantic host; already a declared (previously unused) Iskm dep — makes it real | If CkAnimation gains heavy editor-only weight, reconsider split |
| 2026-07-09 | No dependency on `perf-iskm-lod` for v1 | Atlas Texture2D + CkUsf auto per-instance slots (both on dev) suffice | Gate 2, if look authoring wants Texture2DArray |
| 2026-07-09 | Iskm refactor keeps dev semantics exactly (skeleton-chain ref pose) | perf-iskm-lod's mesh-bind-pose fix belongs to that branch; porting it early would change dev behavior untested here | When perf-iskm-lod merges — fold its two bake deltas into `ck::anim_bake` call args |

## Dated entries (append-only, newest first)

### 2026-07-10 — VAMP gaps 2, 3, 4-vertex closed (editor never launched by the human yet)
- **Gap 2 (root motion):** `_ExtractRootMotion` + `_DisableRetargeting` on the collection → both
  `FCk_AnimBake_SampleParams` fields in the baker. House naming (no `_b` prefix — the Iskm
  collection's `_bExtractRootMotion` is pre-existing drift, not copied).
- **Gap 3 (bake UX + staleness):** details-panel **Bake/Rebake button** on `UCk_VatCollection_Data`
  (`CkVatEditor/CkVatCollection_Details.{h,cpp}`, ResourceLoaderEditor precedent; module registers
  the class layout; Build.cs += PropertyEditor/Slate/SlateCore). Staleness: `_BakedInputsHash`
  (MD5 of skeleton/mesh/clip names+paths+PlayLengths/freq/mode/precision/lookup-channel/toggles)
  stamped by ApplyBakeResults; `Get_IsBakeStale()`; IsDataValid = **warning when unbaked, error
  when stale**; button relabels "Rebake (inputs changed)". Decision: auto-bake-on-edit REJECTED
  (bake saves packages to disk — hostile as a property-edit side effect); validation + one-click
  is the chosen shape (discussion 2026-07-10 with maintainer).
- **Gap 4 (VAT normals) — REDESIGNED, vertex mode only:** the continuation prompt's sketch
  (PS-side instance basis + `CkUsf_WorldToTangentNormal`; bone mode "rotate In.VertexNormal by the
  blended quat") is NOT implementable/correct: verified against the 5.7 fork that the non-Nanite
  PS has no per-instance transform (PS `TransformLocalVectorToWorld` = PRIMITIVE transform,
  MaterialTemplate.ush:1696; `InstanceId` only under `IS_NANITE_PASS`), and rotating a WORLD
  normal by the LOCAL-space quat is wrong under instance rotation. Landed instead: baker encodes
  `_Nrm` in the **bind-pose TANGENT frame** (B = cross(N,T)*sign, matching GenerateYAxis) —
  instance-transform-invariant, feeds the Normal pin directly, zero PS basis math. Pixel entry
  decodes via `In.UV1.x` + the same 12-float frame/crossfade math as the WPO. Generator gained
  opt-in `_PixelDataChannels` (TexCoord1/2 → PIXEL Custom node; other looks' masters untouched).
  `NrmVat` texture param inserted (uniform ⇒ per-instance slots 0-11 unchanged); subsystem seeds
  it (ensure if missing). **Bone-mode normals deferred** with options recorded in Gate_02.
- Ran: toolbox --build → Succeeded (one fix round: `GetPropertyUtilities()` returns TSharedRef —
  `.ToWeakPtr()` + Pin for the button lambda). Headless master regen: **`-ExecCmds` splits on
  COMMAS not semicolons** — `"Cmd X; QUIT_EDITOR"` = one command with arg `X;`, nothing runs,
  editor idles holding DLL locks (killed it); comma form generated + saved
  `M_CkUsf_Look_VatVertex.uasset` and exited cleanly (gen_looks_normals2.log "Generated master").
- Gates on the final binaries: **Usf 4/4, Iskm 30/30 (zero delta), AnimBake 2/2, VatProxy 1/1**
  (Test-{Usf,Iskm,AnimBake,VatProxy}.log); `CkVat_Looks_Assets.as` clean in every boot log.
- Unconfirmed (needs [EDITOR-VERIFY]): everything visual — esp. that lighting now tracks the
  animation in Vertex mode, and the tangent-frame encode's handedness on UV-mirrored islands
  (`cross(N,T)*sign` matches GenerateYAxis by construction, but mirrored-island lighting is the
  claim most likely wrong).
- Adjacent smell (flagged, untouched): `CkResourceLoaderEditor_Module.cpp:34` unregisters its
  CLASS layout via `UnregisterCustomPropertyTypeLayout` — wrong unregister API, harmless at
  shutdown but a copy-paste trap.

### 2026-07-09/10 — Gate 4 opening: first CkVat/anim-bake tests + a real bug caught
- Landed in CkTests (NEW branch `feature/vat-feature` there — the tests reference `utils_vat`,
  merging CkTests dev before CkFoundation's branch would break everyone's AS compile; commit
  `0b4a0fd`): `CkTests.UnitTests.CkAnimation.AnimBake.{LoopedLocalFrame,FrameLayoutAndSampling}`
  (pins the Gate-0 extraction: looped-frame math, layout contract, frame-0 identity) +
  `Ck_AutoTest_Vat_ApiSurface` (the no-baked-content surface, ProxyAdd-style scoping). Commit
  includes the populator's OFPA re-key from the `--discover-fresh` registration.
- **Real bug caught by the new unit test:** pose evaluation crashed (MemStack NumMarks assert) in
  standalone callers — the extracted core relied on the engine tick's FMemMark. Fixed: the core
  scopes a per-sequence `FMemMark` (`660b59b13`). This would have crashed the CkVatEditor baker in
  production paths.
- Results: AnimBake 2/2; Vat_ApiSurface 1/1; Iskm regression **30/30** — the fresh discovery
  surfaced `Create_MakesDistinctChild` (on dev since yesterday, silently skipped by the stale
  toolbox test cache in ALL earlier runs incl. the 29-test baseline) and it passes.
- Flake post-mortem (not a regression): one BatchedVisual red was caused by MY CkTests git
  branch/commit running while the test editor was live — git's CRLF rewrite touched .as mtimes →
  "AS Soft Reload during PIE" escalated to a failure. Clean re-run green. Rule reinforced: NO git
  ops touching .as while a test editor runs.
- Pre-existing, surfaced, untouched: `AutoTests_BB_MAP` has orphaned OFPA actor refs
  ("Failed to load Actor for External Actor Package .../AutoTests_BB_MAP/3/S6/...") — BB-side
  content inconsistency, predates this campaign.
- Remaining Gate 4: end-to-end bake→play→OnClipFinished autotest (needs one editor bake of a
  Mannequin collection committed as CkTests content — bake saves packages, unsuitable per-run),
  CkVat gym, AS/BP parity spot-check beyond wrapper regen.

### 2026-07-09 — Gates 2+3 code-complete (same session, continued)
- Gate 2 landed: `FCkUsf_VertexInput` += UV1/UV2/LocalPosition + per-instance local→world basis
  (filled via `TransformLocalVectorToWorld` — VERIFIED against MaterialTemplate.ush:1702-1709 that
  the VS overload multiplies by InstanceLocalToWorld under instancing); generator wires the new
  pins (TexCoord1/2 + PreSkinnedPosition nodes); validator reserves the new input names.
  `Source/CkVat/Shaders/CkVat/Vat.ush` (VatVertex + VatBone pixel/WPO pairs: 2-row frame interp,
  2-state crossfade, 4-influence quat skinning, weight3 = 1−r−g−b) + `/CkVat` shader mapping +
  AS look assets (`Script/CkVat/CkVat_Looks_Assets.as`, param order == slot contract).
- Ran: headless `Ck_Usf_GenerateLooks VatVertex/VatBone` via BusterBlockEditor-Cmd →
  **both masters generated, validated, saved** (`Content/CkUsf/GeneratedLooks/M_CkUsf_Look_Vat*.uasset`,
  gen_looks.log "Generated master for look [VatBone]"). Gotcha: `Quit` ExecCmd never fired (needs
  `QUIT_EDITOR`) — the editor idled an hour in the EOS tick loop holding DLL locks (LNK1104 on the
  next build) until killed after confirming the saves.
- Gate 3 landed: `UCk_Vat_Subsystem_UE` (shared MID from the generated master via
  `Get_GeneratedMasterObjectPath` — outline-subsystem precedent; uniforms seeded; transient ISM
  renderer via NEW `GetOrCreate_ForMeshWithMaterialsAndCustomData`, count in the cache key, Movable
  load-bearing); Setup composes IsmProxy + initial float push (+ RandomPerInstance phase offset);
  HandleRequests pushes per drained batch; FireSignals fires OnClipFinished once (positive-rate
  Once clips; negative-rate completion = recorded follow-up); crossfade source captures its own
  rate/loop. Collection gains `_BaseColorTexture` (Rendering category).
- Ran: toolbox --build → "Result: Succeeded" (build_gate3b.log; one fix round: TWeakObjectPtr needs
  the DEFINING include of UCk_IsmRenderer_Data in the processor TU).
- Ran: Iskm suite post-Gate-3 → **29 passed / 0 failed / 0 skipped** (tests_gate3.log) — fourth
  consecutive green run; only the documented pre-existing FProInstance AS noise in the boot log.

### 2026-07-09 — Gate 0 committed; Gate 1 baker landed (code-complete)
- Committed Gate 0 on `feature/vat-feature` (user-created): `f69f9095a` anim-bake core, `08940084c`
  Iskm refactor, `eb2087ee3` CkVat runtime, `07a437601` CkVatEditor+uplugin, `8649f328c` docs.
  NOT pushed. Note: repo `.gitignore:49` blanket-ignores `*.md` — docs force-added (existing
  tracked module docs prove the precedent).
- Gate 1: engine APIs verified at file:line against the 5.7 fork by a research agent BEFORE coding
  (FMeshDescription authoring, editor source model + FSoftSkinVertex **uint16** influences,
  ConvertMeshesToStaticMesh ceremony, TSF_RGBA16F/TC_HDR). Contract + layout spec:
  Plan/Gate_01_Bake.md.
- Landed + committed (`9e238e06c` bounds/ApplyBakeResults, `6575ac566` baker): full bake flow for
  BOTH modes; entry `UCkVat_BakerSubsystem::Bake_VatCollection`.
- Ran: toolbox --build → "Result: Succeeded" (build_gate1c.log; two fix rounds: missing collection
  include in the subsystem TU; unformattable unnamed-enum `MAX_MESH_TEXTURE_COORDS_MD` in an ensure).
- Ran: Iskm suite regression post-Gate-1 → **29 passed / 0 failed / 0 skipped** (tests_gate1.log) —
  zero delta vs baseline, third consecutive green run on this branch.
- Confirmed: layout contract in Gate_01_Bake.md cross-checked against the shipped encoder code
  (row-0 ref pose, lookup U formula, bone index/weight carriers, precision encodings) — consistent.
- Inferred (unconfirmed, needs [EDITOR-VERIFY]): the bake produces visually-correct data on real
  content. The one claim most likely wrong: **triangle winding** on the baked static mesh (kept
  source index order; if meshes render inside-out, swap corners 0/2 in BuildBakedStaticMesh).
  Also unconfirmed: mesh-bind-pose vs skeleton-ref-pose divergence on reoriented imports (dev
  semantics kept; the perf-iskm-lod fix folds in at merge — decision log).

### 2026-07-09 — Gate 0 implementation (same session, later)
- Ran: baseline Iskm tests → **29 ran, Failed: 0, Skipped: 0** (tests_baseline.log). The module doc
  says 27, the filename count is 28; the pattern additionally matched `SkmcPerf` → 29 is the number
  to diff against.
- Landed: `ck::anim_bake` core (`CkAnimation/Public/CkAnimation/AnimBake/CkAnimBake.{h,cpp}`) —
  BuildSkeletonData / BuildFrameLayout / SamplePoses / ComputeAnimatedBounds / Get_LoopedLocalFrame,
  extracted 1:1 from the Iskm bake (dev semantics kept — perf-iskm-lod's mesh-bind-pose fix NOT
  ported; see decision log).
- Landed: Iskm `Build_BakedPoseData` refactor — body-only delegation, public header untouched,
  `FCk_Iskm_BakedPose` output identical by construction. Compile-verified in isolation:
  build_extraction.log "Result: Succeeded" (35s incremental).
- Landed: CkVat scaffold — collection asset (incl. serialized `FCk_Vat_BakedClip` table + duplicate-
  name validation), typesafe handle, ParamsData, 3 requests, Current/Requests fragments,
  OnClipFinished signal, Setup + HandleRequests processors (playback state machine: crossfade
  bookkeeping, freeze-preserving Stop, position-preserving SetPlayRate), Utils surface, log/module
  plumbing, Claude.md. CkVatEditor scaffold (module plumbing only; baker = Gate 1). Both registered
  in CkFoundation.uplugin (CkVat Runtime/Default, CkVatEditor Editor/Default).
- Confirmed idioms against live code before writing: request enqueue (CkIskmProxy_Utils.cpp:344),
  CopyAndRemove drain (CkAnimPlan_Processor.cpp), CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE
  (CkAnimAsset_Utils.cpp:41), signal define arity (CkAnimPlan_Fragment.h:151-157), FCk_Time ops
  (CkTime.h:37-40,82).

### 2026-07-09 — Gate 0 session start
- Ran: toolbox `--build` (BB root) → "Result: Succeeded", "Target is up to date" (build_baseline.log).
- Ran: toolbox `--test --test-pattern "IskmRenderer" --no-nullrhi` → IN FLIGHT (baseline).
- Confirmed: dev clean at `545be1a53`; no editor process; 28 `CkAutoTest_IskmRenderer_*.as` in CkTests.
- Confirmed: research phase complete (5 digests + synthesis in session scratchpad; absorbed into PROMPT.md).
- Inferred (unconfirmed): Iskm suite currently green (module doc claims it) — the in-flight run confirms or corrects.

## Open items
| Item | Status | Next step |
|---|---|---|
| Baseline Iskm test counts | in flight | read log verdict, record here |
| Doc-drift follow-ups found in research (CkGraphics/Claude.md false claims; CkIskmRenderer Claude.md stale layout/framing; unused declared deps CkPhysics in Iskm Build.cs) | recorded, out of scope | separate chip/session |
**Rule: no completion claim anywhere in this file while any row here is unresolved.**
