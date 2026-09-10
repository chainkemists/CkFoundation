# Gate119 — seven-gym baseline and presentation contract

Date: 2026-09-10. Entry: Gate118 planning. Owner: main campaign session.

## Verified entry

All on `feature/ck-navigation`: root `fd9f6abf7d0c9d4de95d1f98b714d11c48ca7101`, Foundation `ea6f8bf0b714bd4a398022c87f72a8e3f2d06ccb`, Tests `cfd3ea1704ba45f5ec1feca48b4ac46ac541dc35`. Status exactly matches the continuation prompt. No editor/Toolbox process was present. The settings header has no semantic diff under `--ignore-space-at-eol`.

Read and spot-checked Gate111 (474 passed / 474, zero failures/skipped/contaminated) and Gate115 (522/522, zero failures/skipped/contaminated) in their original Toolbox logs. These are historical entry evidence, not fresh verification of new presentation code.

## Baseline prepared before presentation edits

No rendered baseline has been captured in this session. The six original GroundNav viewpoints and controls are inventoried below; PathNetwork has no original fixed overview. Source-inferred issues are primitive-only geometry, long duplicated captions and debug clutter; they are not visual-review findings. The reference inspected was `D:/Repos/JoltNav/Screenshots/2026-08-31_02-04_1.png`: clear navigable surface/scene separation and one short caption.

The new presentation must start in **Baseline** mode, preserve original materials and GroundNav viewpoints, and expose explicit **Hero**, **Diagnostic**, and **Baseline** controls. This lets the operator collect the unchanged scene before selecting the presentation candidate without switching revisions or creating another checkout. PathNetwork receives a prepared overview; its former uncontrolled camera cannot be reconstructed as an original screenshot.

| Gym | Original camera offset / rotation | Original controls | Baseline screenshot / verdict |
|---|---|---|---|
| Tuning Range | (300,-900,550) / (-22,75,0) | R/T/B/F/G/N/J/M/L/K/P/O/V/Y/X | OPEN |
| Walk | (2600,-2400,2000) / (-30.5,143.8,0) | 1, T | OPEN |
| Links | (750,-950,700) / (-24,128,0) | U, T | OPEN |
| Dynamic Obstacle | (2000,-1700,1500) / (-33,118,0) | 3, 4, T | OPEN |
| Markup | (2200,-2000,1800) / (-31,138,0) | 2, T | OPEN |
| GroundNav vs Recast | (2400,-1800,1700) / (-29.5,143.1,0) | 1, 2, T | OPEN |
| PathNetwork | no original fixed camera; prepare full-floor overview | R, B, Z | OPEN |

GroundNav offsets are relative to the scene origin (station footprint plus each controller's `k_SceneOffset`). The original controllers retry camera placement after possession. The benchmark capsule 42/192 is **not** applied to these gyms; each existing fixture keeps its own profile.

## Shared presentation contract — G119-D1

- Compose one reusable navigation presentation layer over the existing gym controller/panel. Keep all existing keys and row dispatch indices. Use Home=Hero, End=Diagnostic, Backspace=Baseline; these are additive controls. No changes to gym geometry, collision, Jolt bake input, provider selection, route endpoints, walker observation, verdicts, or restart semantics.
- Baseline keeps the old scene look. Hero and Diagnostic use a fixed fitted 16:9 oblique camera and one concise caption. Hero has a small screen-space title, actual provider and live verdict; Diagnostic retains the existing full control panel and field diagnostics. The state belongs to the current controller, never to project settings.
- Keep current agent/route/link colors; do not make provider color replace per-walker identity. Navy/slate scene surfaces and restrained amber feature accents may be applied only to explicitly registered gym meshes. Store and restore original materials. Add no decorative collision and perform no presentation-triggered bake or path request.
- Preserve diagnostic draw selection: changing presentation never silently changes the tuning row or the selected provider. Long world captions may be suppressed only in Hero; feature markers and state-changing overlays remain truthful.
- Capture records log the real provider, scene/view mode, camera, resolution and current verdict. A capture request is not evidence of a saved file; the operator verifies the PNG and supplies revision/settings/log identity. Do not label a missing/pending verdict as passing. PathNetwork's existing arrival/failure log remains its behavioral evidence; do not invent a new pass criterion.
- Baseline: capture seven initial views first, before selecting Hero. Candidate acceptance: seven 1920x1080 (or higher exact 16:9) Hero + Diagnostic pairs; 10–20 second clips for Links, Obstacle, Markup, provider switch and PathNetwork rebuild. Record every action and current verdict. Human judgment of the visual bar remains required.

## Execution and gates

1. Implement shared helpers first, then integrate the six GroundNav gyms, then PathNetwork. Verify by code review that behavior and collision are unchanged.
2. Add focused runtime coverage for presentation controls, material restoration and rejection of an unpossessed camera request; include compatible GroundNav/PathNetwork coverage in the final local gates. New-test discovery and the Toolbox's separate family execution require a separately counted boot plan, recorded in PROGRESS.md. AS-only changes reuse current binaries. Toolbox launches detached with its progress window enabled. Camera framing, actual input routing and rendered panel acceptance remain part of the human walkthrough.
3. Stop at the showcase acceptance boundary until fresh captures and human sign-off exist. Benchmark implementation follows this gate. Actual timing needs the same Development-game artifact on the build machine, at least three alternating eligible pairs and traces.
4. Editor snap/undo/LiveExtract, actual WP/Data Layers and manifest cook, real-map paths, Development/Test/Shipping packages, then Recast-first BusterBlock migration remain open in that order. No local cook/package, Git delivery, or sibling-checkout mutation is part of this slice.

## Pre-edit SHA256 inventory

First thirteen entries are preserved unrelated dirt; remaining entries identify the baseline presentation source.

| Path (relative to root) | SHA256 |
|---|---|
| `Config/DefaultGameplayTags.ini` | `58F2D444B94D264451A60827CA59E1D2E7DFD46E9D0FF0B5DF7ECB5BC397AAC8` |
| `Script/Generated/CkPlugins_EntitySpawnParams.as` | `EE096C5A51786CF852E29B096BDA7D0B05BA926B7890238455F48A26591F01C5` |
| `CONTINUATION_PROMPT_CkNavigationReplacementCampaign.md` | `902339898335636EAC7FB49382B53B840A6BEBCB4643D5F1D11FB0E606D12708` |
| `Plugins/CkFoundation/Source/CkNavigation/Public/CkNavigation/Settings/CkNav_ProjectSettings.h` | `23614676EA7FC14488CDAFB26FF60D43139B464F5D9E585921EB9916E5037B7F` |
| `Plugins/CkTests/Script/CkGroundNav/CONTINUATION_PROMPT_GroundNavTuningRange.md` | `C66E2A33E0AD7F71F00F9376D052E9F1AC60572B6FB1820BD2788CC4B94F683A` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltCell_-1_0.ckexport` | `3245792258EEF88DF904F7BAD62DFF5D6F6EEA28671F6B2E5B9D7509A7A24C77` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltCell_-1_0.uasset` | `FFB9E6D27B4A832DF3066C51E7DA7EDBE8A1807C702FCB71E383EDD832FD1B40` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltCell_0_-1.ckexport` | `36EF21D513AEA5F238C743FC5E6F2646494267455F1BFE38B51061EE42B3ECFA` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltCell_0_-1.uasset` | `EAF5C2D160FA29E3DFF6F1AF26E437A38AF2A5A0E6989D0E89453BC2CA22023A` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltCell_0_0.ckexport` | `1BF762B42D057C847323E19DEE02F4EB914C0C65473F8CD710397EE2D4646870` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltCell_0_0.uasset` | `577381E90E093CA282063884F48AAD89B68AF32DF6BD978AFB4E1B2EB96CAA98` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltIndex.ckexport` | `42C80782E172A47C568E7CD03CA01672258DF781AC021C103ADF9491E0CD4562` |
| `Content/CkJoltData/CkTests/GroundNavAcceptance/Maps/GroundNavStreamingAcceptance/JoltIndex.uasset` | `334286206ACF3A84782094A199C51463FF3BBA598917A2FFDE5EC538A4674F60` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Common.as` | `DEEDF7AA685CC36E97D759E25ECA8F416FC717796D0C14BC1D4A16F1F92FD6FC` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Demo.as` | `FD12AE85F0A34C02C0ED407EAF66F68FEB84963E948CB2EA280B9C86C01B7CD3` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Links_GameMode.as` | `4A7269302BAF0F288C3FE5BBCD6AF20A4ED45C09D079C7EB2A6946C1F90F97CA` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Links_PlayerController.as` | `5DD27565EA6C910DA5CFA097381E7A555B5ED93A4A0B65C9D9F2C51567EAB09A` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Markup_GameMode.as` | `761B595A6566A3AA3BCE3187F86E54FCF027773C1579CA2EE9A384FD669671EF` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Markup_PlayerController.as` | `E872A687896A3CAA3BA56559C46FAB7AA7ECC0F683BD043C96FDA5B96D8E02F5` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Obstacle_GameMode.as` | `5453A112968DEB84188609F22082A56CE6CD9A4624B7B8AC85F7D2B4D528D0AD` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Obstacle_PlayerController.as` | `BE8A13C2F356ACC7792885FA93779C38A608EAF62F65C9569F0498F85B34513C` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Parity_GameMode.as` | `1346D5D419981AC17BBE986B722F786882BF21A49994B0953881349678AF7162` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Parity_PlayerController.as` | `C1005EC2BDE44A5684EC6D21406519F91DC67346D136185AC1F4636710F37833` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_TuningRange_GameMode.as` | `962CAFD4AFABC385AA7E7CAC44A2F4954B006D53FDCB42DE1D0F9C02CA546E8B` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_TuningRange_PlayerController.as` | `77FCC097B8E090AE84801752DF84214AFB2A50C2F510C2F626148985D3379942` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Walk_GameMode.as` | `B26E8BBD4648FB6312EB0EA582CC634B9413E861BE10B49783816142BA537B2F` |
| `Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Walk_PlayerController.as` | `98CA85E2385CEDF75E04800E23C10FC0D07AE5524A30D233FC4543531E060BFF` |
| `Plugins/CkTests/Script/CkPathNetwork/CkPathNetworkGym_Following_PlayerController.as` | `781729E6549062F33C84BDB390DF843E0132F6F6A3D0FDB28598865A8FC60A6D` |
