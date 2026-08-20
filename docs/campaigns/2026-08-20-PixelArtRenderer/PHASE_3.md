# PHASE 3 — CkCamera orthographic support

> Entry: independent of Phases 1–2 (may run in parallel on the same branch, but sequence commits).
> Load `ck-macros-and-codegen` + read `Source/CkCamera/CLAUDE.md` before edits. This deliverable
> is useful standalone — keep it free of any CkPixelArt reference (zero coupling; CkCamera must
> not depend on or know about the pixel-art modules).

## Executable spec

AS AutoTest written FIRST (red — the API doesn't exist), in CkTests
`Script/CkCamera/CkAutoTest_Camera_OrthoProjection.as` (follow the harness rules in
`ck-tests-authoring-and-running` / the existing CkCamera test files' shape):

1. Compose a camera on a test entity with a profile whose sensor sets
   `_ProjectionMode = Orthographic`, `_OrthoWidth = 1024`, explicit near/far.
2. Settle one frame; assert `utils_camera::Get_ViewInfo(...)` reports
   `ProjectionMode == Orthographic`, `OrthoWidth == 1024`,
   `bAutoCalculateOrthoPlanes == false`, and the explicit plane values.
3. Acquire a layer modifier on OrthoWidth, drive it to 2048 at full alpha, settle; assert the
   composed `Get_Profile()` and `_ViewInfo` read 2048.
4. Reset/remove the layer; assert 1024 returns.

Toolbox: `--test --test-pattern Camera --discover-fresh` (record which pattern actually matches
the new test name; a zero-match run is stale-green — see `reference_toolbox_test_discovery_gotchas`).

## Steps

1. **New enum** `ECk_Camera_ProjectionMode : uint8 { Perspective = 0, Orthographic }` (UENUM,
   0-entry rule, `CK_DEFINE_CUSTOM_FORMATTER_ENUM`) in `CkCameraProfile.h`.
2. **Profile leaves** on `FCk_CameraProfile_Sensor` (`CkCameraProfile.h:117-139`, beside `_FOV`):
   `_ProjectionMode` (enum, default Perspective), `_OrthoWidth` (float, default 1024.f),
   `_OrthoNearClipPlane` (float, default 0.f), `_OrthoFarClipPlane` (float, default 100000.f).
   All with `CK_PROPERTY` (modifiers must be able to mutate a running profile — house note in
   the file). UPROPERTY on its own line, no column alignment.
3. **OrthoWidth = blendable attribute** — the mechanical five-site pattern (verified sites,
   [RESEARCH_Codebase.md](RESEARCH_Codebase.md) §2):
   1. tag `TAG_Camera_Sensor_OrthoWidth` (`CkCamera_GameplayTags.h:25-27` + `.cpp`);
   2. handle field on `ck::FFragment_Camera_Sensor` (`CkCamera_Fragment.h:200-207`);
   3. `AddFloat` line in `DoMaterializeAttributes`' Sensor block (`CkCamera_Utils.cpp:527-533`);
   4. `ReadFloat` in `Get_Profile`'s Sensor block (`CkCamera_Utils.cpp:637-645`);
   5. `Acquire_CameraModifier_OrthoWidth` beside FOV (`CkCameraLayer_Utils.h:85-91` + the
      `DoAcquire_Float` plumbing its siblings use).
4. **Mode + planes = non-blending plain fields** on `FFragment_Camera_Current`
   (`CkCamera_Fragment.h:93-102` block), seeded in `DoMaterializeAttributes`' bool/plain block
   (`CkCamera_Utils.cpp:584-599`), re-read in `Get_Profile`, mutable via requests:
   - `FCk_Request_Camera_SetProjectionMode` (carries mode + optional plane overrides —
     request STRUCT, not loose params, per Source/CLAUDE.md rule) +
     `UCk_Utils_Camera_UE::Request_SetProjectionMode(UPARAM(ref) FCk_Handle_Camera&, const FCk_Request_Camera_SetProjectionMode&, const FCk_Delegate_Request_OnCompleted&)`
     — immediate mutator shape (fires completion synchronously per the root CLAUDE.md
     "Immediate mutators" contract; mimic `Request_Set_ConstrainAspectRatio`,
     `CkCamera_Utils.cpp:283-293`).
5. **ViewInfo assembly** (`CkCamera_Processor.cpp:293-304`, the ONLY legal write site): when
   mode is Orthographic — `ViewInfo.ProjectionMode = ECameraProjectionMode::Orthographic;
   ViewInfo.OrthoWidth = <composed>; ViewInfo.bAutoCalculateOrthoPlanes = false;
   ViewInfo.OrthoNearClipPlane/OrthoFarClipPlane = <composed>;` (explicit planes per D4 — the
   engine's auto far plane is view-rect-dependent, `CameraStackTypes.cpp:386-473`).
6. **Three environments**: BP surface comes from the UFUNCTIONs; AS wrappers regenerate on next
   editor boot (note in PROGRESS.md that `Script/Generated/utils_camera.as` must show the new
   functions after regen — grep it; if absent, see `ck-angelscript-interop`).
7. Run the spec test green; scoped `--test-pattern Camera`; commit (CkCamera commit separate
   from CkTests commit; CkTests must not merge/land first — same discipline as prior campaigns).

## Exit criteria

- The AutoTest above green under a fresh-discovery run.
- `rg -n "OrthoWidth" Source/CkCamera` shows the five pattern sites + processor write.
- `rg -in "pixelart" Source/CkCamera` → 0 hits (zero coupling).
- Full suite delta-zero vs baseline; `Script/Generated/utils_camera.as` contains
  `Request_SetProjectionMode` after regen.

## Fences

- `_ViewInfo` is written in `FProcessor_Camera_UpdatePOV` ONLY — friend-gated; do not add
  writers (`CkCamera_Fragment.h:72-74`).
- Do not enable `bAutoCalculateOrthoPlanes` "for convenience" — D4's whole point is
  resolution-independent depth range.
- Do not make the planes attributes (they don't blend; plain-field pattern).
- Do not touch `UCk_CameraComponent::GetCameraView` — the whole-struct assign already carries
  the new fields for free.
- ECk enum: never reorder entries after ship (index contract in serialized assets).
