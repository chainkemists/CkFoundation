# Gate119 — capture and visual review

Status: human/showcase decision CLOSED by user attestation on 2026-09-10 after all seven gyms were run. No rendered baseline, screenshot/clip archive, or fresh log artifact is claimed; the pre-change baseline was prepared, not rendered.

Use the existing `D:/Repos/CkPlugins_3` checkout. Identity and the pre-edit hashes are in `GATE119_SHOWCASE_BASELINE_AND_CONTRACT.md`. Do not switch revisions to obtain a baseline: the candidate starts in Baseline mode, preserving original scene materials and GroundNav startup cameras. PathNetwork had no authored baseline camera; its Backspace view is explicitly a prepared overview.

## Historical capture setup (run only if archival evidence is newly requested)

1. After the local gate has finished, open `/CkTests/TestGyms/TestGyms_CkTests_Level` in the editor. In Play settings, select **New Editor Window** and set a 1920 by 1080 viewport (or another exact 16:9 size). Keep resolution, RHI, scalability and FOV fixed across the entire set. Record these settings, machine, root/Foundation/Tests SHAs and dirty source identity.
2. Start PIE. Press Tab and select the named gym. Release movement keys and wait for its original camera retry and field build. Read the actual Verdict; record pending/failing states as such. PathNetwork reports arrivals/failures in the log and has no aggregate passing verdict.
3. Collect **all seven Baseline images first**. Every gym initially uses Baseline; Backspace restores it after another preset. For PathNetwork, press Backspace to set its prepared overview. Keep the existing H panel state consistent and record it. Do not press a tuning/provider/action key before that gym's initial baseline capture.

## Historical record and capture

The console command `Ck_NavShowcase_Record` logs `[NAV-SHOWCASE]` with scene, current provider, mode (0 Baseline / 1 Hero / 2 Diagnostic), actual viewport resolution, camera, rotation, FOV and the current verdict. `[NAV-SHOWCASE-CONTROL]` lines record all current controls and values. It records observations only; it does not mark acceptance, take a screenshot, or supply missing artifact identity.

For each state, run it, close the console and capture the unobstructed view. The engine supports `Shot SHOWUI filename=<name>`; for example:

```text
Ck_NavShowcase_Record
Shot SHOWUI filename=GroundNav_Walk_Baseline_GroundNav_20260910
```

`Shot` appends its own suffix so an existing file is not overwritten. Find the actual saved image in `Saved/Screenshots`, open it, and record its exact path/dimensions. Reject a console-obscured, clipped, stale or non-16:9 image; a console request or log marker alone does not prove a valid capture. Retain the fresh editor log before the next session rotates it.

## Historical candidate controls

- **Home** / `Ck_NavShowcase_Hero`: fitted oblique view, reversible navy/amber scene tint, short caption, live provider and verdict. Existing feature keys remain active.
- **End** / `Ck_NavShowcase_Diagnostic`: the same fitted view and tint with the full existing panel. Existing T drawing mode and provider selections stay exactly as selected.
- **Backspace** / `Ck_NavShowcase_Baseline`: restores original materials and the original GroundNav camera; PathNetwork uses its prepared overview.
- **H** retains the original persisted panel preference in Baseline. Hero and Diagnostic override panel display only for this controller; selecting these presets does not write that preference.

These controls do not reset behavior, bake a field, select a provider or restart walkers. Select the desired original feature action explicitly and wait for its actual result before capturing.

## Historical required matrix

Each row requires one Baseline image, one Hero image, one Diagnostic image, exact active controls, actual provider/verdict, settings/revision identity and fresh log. Baseline can show an unbuilt/pending initial state if recorded honestly; the accepted candidate must demonstrate the requested feature clearly.

| Gym | Candidate diagnostic/action | Additional evidence |
|---|---|---|
| Tuning Range | Y tiled field, T tiles/layers, one selected tuning row; show stairs, platform, catwalk and pinch | Record selected tunable and observed field/verdict change |
| Walk | 1 cycles 3/1/8 walkers; show pillars, ramp and refused island route | Capture the default three-role verdict; record each other count |
| Links | U enabled/disabled/re-enabled, T Links | 10–20 s clip of link state and walker response |
| Dynamic Obstacle | 3 drop/lift, 4 auto-repair, T field view | 10–20 s clip of obstacle, local repair and reroute; replacement box keeps tint |
| Markup | 2 paint/unpaint, show blocked strip and open gap | 10–20 s clip of detour and corridor restoration |
| GroundNav vs Recast | 1 provider, 2 Recast draw, T GroundNav view | Same camera for both providers plus a 10–20 s switch clip; missing Recast data is a failure, not parity |
| PathNetwork | Six-scenario overview, R restart, B live lane rebuild | 10–20 s rebuild clip, arrival/failure log and human observations for all six scenarios |

## Sign-off and historical capture conditions

The historical capture procedure asks the reviewer to record name/date and pass/fail per gym: feature understandable within about three seconds; title/provider/verdict readable; no caption clipping; routes/agents distinct from ground; visible geometry agrees with collision; original controls and baseline restoration work; no fresh script error or unexpected ensure. Inspect the image and clip themselves. A focused automation pass is only runtime-control evidence. The user attestation on 2026-09-10 closed the human/showcase decision after all seven gyms were run, but it does not assert that these capture artifacts or a fresh log were archived.

**The showcase decision is closed by attestation; do not infer an archived matrix from that fact.** Gate123 closes local fixture/harness acceptance: 5/5 in 57 s, four eligible real reports, schema 21/21 and no ensure failures, script errors, Pending Timeout or never-answered warnings. QueryBurst128 is 128 Ready for both providers; CrowdConvergence240's sample-boundary Stop240 removed Gate121d's historical post-terminal warning path, but its terminal distributions are not convergence or behavioral-equivalence proof. No performance result is claimed; build-machine matched pairs, provenance and traces remain deferred to BusterBlock integration. Gate122 PathDiagnostics remains 7/7 with zero ensure/script errors. Remaining editor snap/undo/LiveExtract, actual WP/Data Layers/cooked manifests/real routes, Development/Test/Shipping packages, and Recast-first BusterBlock migration retain their documented gates. Recast remains the A/B provider and rollback path. No local cook/package is permitted.
