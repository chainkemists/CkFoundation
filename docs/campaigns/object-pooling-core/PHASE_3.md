# Phase 3 — TStrongObjectPtr sweep conversions (non-pooled vends)

> **Status:** ✅ Done (2026-07-11)
> **Depends on:** Phase 2 ✅
> **Drift notes (code wins):**
> - Subsystem gained an `InOuter` param: DestroyOnRelease vends honor the caller's Outer
>   (CkUnrealComponent outers scene components to the ComponentHost ACTOR — nav octree needs
>   `GetOwner()`); Recycle-pool vends stay world-outered (a recycled instance outlives its first
>   caller).
> - Unpin ordering everywhere: `TryReleaseToPool` runs IMMEDIATELY BEFORE `DestroyComponent`
>   (destroy may mark the object garbage, failing the release's validity gate; no GC can run
>   between the calls). CkUI wrapper releases after `RemoveFromParent` (no destroy involved).
> - PMG components lost their `RF_Transient` flag (subsystem vends RF_NoFlags) — accepted:
>   runtime-world objects are never level-serialized.
> - Same-day API trim (user-driven): per-use `FInstancedStruct` removed from the entire acquire
>   chain — OnAcquiredFromPool carries no payload; per-use data flows through the synchronous
>   caller / EntityScript spawn-params injection + Construct.

## Goal

After this phase: the six swept members are `TWeakObjectPtr`, their objects vended (pinned) by the
ObjectPooling subsystem as non-pooled instances and unpinned at feature teardown. No recycling —
lifetime hoisting only. (User-confirmed scope, decision 4.)

## Entry criteria

- [ ] Phase 2 exit re-verified on current HEAD (hash: ______).
- [ ] Baseline: full suite counts on Phase-2 binary; targeted suites for the four touched modules.
- [ ] Per-member creation/teardown sites re-read on current HEAD (audit from 2026-07-11 may have drifted).

## Work items (one commit per module; each: create via subsystem vend → member weak → teardown unpins)

| Member | File | Create site | Teardown site |
|---|---|---|---|
| 1. `_Component` | `CkUnrealComponent/.../CkUnrealComponent_Fragment.h:52` | `_Processor.cpp:88,97` (`Request_CreateNewObject<UActorComponent>`) | `_Processor.cpp:187` (`DestroyComponent` + Reset) |
| 2. `_MeshComponent` ×2 | `CkPmg/.../CkPmg_Fragment.h:55,:247` | `CkPmg_Processor.cpp:238,303` + 6 shape processors (raw `NewObject`) | `CkPmg_Processor.cpp:465,730` |
| 3. `_AudioComponent` | `CkAudio/.../CkAudioTrack_Fragment.h:41` | `_Processor.cpp:64,231` (raw `NewObject`, NOT SpawnSound) | `_Processor.cpp:668-671` |
| 4. `_WidgetComponent` | `CkUI/.../CkWorldSpaceWidget_Fragment.h:57` | `CkWorldSpaceWidget_Utils.cpp:178,201` | widget teardown path |
| 5. `_WrapperWidget` | `CkUI/.../CkWorldSpaceWidget_Fragment.h:55` | `CkWorldSpaceWidget_Fragment_Data.cpp:21` | same — NOTE: `AddToViewport` may add a viewport-held ref; verify removal order |

Rules:
- Raw `NewObject` sites convert to the pooling-aware `Request_CreateNewObject` (no pool params =
  force-new vend, pinned). `RegisterComponentWithWorld` / component registration flow unchanged.
- Teardown: `DestroyComponent()` first, then `TryReleaseToPool` (unpin). Unpin before destroy would
  let GC race the destroy — keep the order and note it in code only if non-obvious.
- Must-stay-strong members (Niagara/render targets/transient textures/asset pins:
  `_NiagaraComponent`, `_Target`, `_UploadTexture` ×2, `_FontOverride`, `_SlotToMaterial`,
  `_ContentWidgetHardRef`, `_ReplicatedObjects`, processor `_Instance`, subsystem `_WorldActors`)
  are NOT touched — decision recorded, do not re-litigate.

## Expected observations at the gate

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Targeted suites (Pmg/Audio/UI/UnrealComponent) + full suite | == baseline | New reds in a touched module | A/B-stash that module's commit; likely teardown-order (unpin vs destroy) |
| Forced full GC with live PMG text + audio track + world-space widget (gym or autotest) | All visuals/audio persist | Component vanishes | The vend path missed that create site |

## Exit criteria — same commit as last work item

- [x] Zero remaining `TStrongObjectPtr` on the six members — VERIFIED 2026-07-11: `rg --no-ignore`
      over the 4 modules leaves only the intentional keeps (`_ContentWidgetHardRef`, PMG static
      `BundledFont`, `_FontOverride` asset pin).
- [x] Suite diff vs entry baseline — VERIFIED: 1048/1040/8 with IDENTICAL failing names on BOTH the
      sweep-only binary and the final (sweep + per-use-param-removal) binary. Four consecutive
      identical runs across the campaign.
- [x] PROGRESS.md dated entry.
- Forced-GC gym observation (expected-observations row 2) — [DEFERRED-TO-P4/P5 autotests]; the
  13-min suite's natural GC over 1040 green tests (PMG/Audio/UI/UnrealComponent suites included)
  stands as interim evidence.
