# Campaign: Spec naming & fragment granularity

**Authoritative spec:** [docs/specs/2026-08-05-config-naming-and-fragment-granularity-design.md](../../specs/2026-08-05-config-naming-and-fragment-granularity-design.md)
**Started:** 2026-08-05 · **Mandate:** maintainer — "Implement all of the above to completion" (standing recommendations accepted for F2–F6; F1=`Spec`, F7=keep `FFragment_X_Params` for the immutable runtime fragment).

## Mission

1. Rename the reflected authoring struct pattern `FCk_Fragment_<X>_ParamsData` → `FCk_<X>_Spec`
   across CkFoundation + CkTests + CkGameplayDebugger + superproject script, with CoreRedirects.
2. Apply the data-placement doctrine (spec §4): unpack Specs at `Add()`; config fragments earned by
   steady-state reads only; one home per datum; split monolithic Currents; retire `_Current`
   naming in favor of bare primary-state / purpose-named fragments.

## Hard rules

- Build/test ONLY via `./CkAuto/UnrealToolbox.exe` from the CkPlugins root (config: follow
  last-built = Development; do NOT pass --config).
- UFUNCTION **parameter names** and UPROPERTY **member names** never rename in this campaign (BP
  pin/data stability) — only TYPES rename.
- Every struct rename gets a `+StructRedirects` line in
  `Plugins/CkFoundation/Config/DefaultCkFoundation.ini` BEFORE any asset-loading step.
- AS sweep scope per rename: `Plugins/CkFoundation/Script/` (hand-written only — `Script/Generated/`
  regenerates), `Plugins/CkTests/Script/`, superproject `Script/`. BusterBlock repos are OUT of
  reach this campaign — their sweep happens when they bump the submodule (documented cost).
- Stage only files this campaign touched. Untracked `Tools/`, `docs/digests/` in CkFoundation are
  NOT ours.
- Each phase ends with the full gate (build + tests) diffed against the recorded baseline.

## Phases

| Phase | Content | Gate |
|---|---|---|
| P1 | Timer pilot: rename `FCk_Timer_Spec`/`FCk_MultipleTimer_Spec`; fragments → `FFragment_Timer` (chrono) + residue `FFragment_Timer_Params{_Behavior}`; unpack at Add/AddOrReplace; handlers read `FTag_Timer_Countdown` (fixes stale-direction bug); stat-id from label; Has anchor → `FFragment_Timer`; consumer + AS sweep; CoreRedirects | build + Timer tests vs baseline |
| P2 | Doctrine docs: root CLAUDE.md, Source/CLAUDE.md, CkEcs/Claude.md, Script/CLAUDE.md, CkTimer/Claude.md, DECISIONS.md entry | n/a (docs) |
| P3 | Track A bulk rename: scripted rename of remaining ~122 structs + CoreRedirects + AS sweeps | build + full test suite vs baseline |
| P4 | Track B shrinks: AudioTrack (config residue; drop dead library soft-ptrs from Params fragment), Probe (construction half dissolves into tags), Pmg Current/Params dedup; dead Params view members (Probe_UpdateTransform, Probe_EndPlay, Tween_HandleYoyoDelays); Transform dead alias removal | build + Audio/Probe/Tween/Pmg tests |
| P5 | Monolith splits, case-by-case: VoiceTalker, Camera, AudioTrack Current, VatProxy, Homing, Minimap/Compass scratch | per-feature gates |
| P6 | Identity-anchor audit: all 27 Params-keyed `Has()` sites resolved (moved or justified) + final full gate + BP resave list `[EDITOR-VERIFY]` | full suite |

## Baseline (fill in from first toolbox run)

See PROGRESS.md.
