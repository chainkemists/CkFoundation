# PHASE 1 — restore-processor census + the blocking divergence (2026-07-11)

Produced by the Phase-0 executor session (Opus) before stopping on a divergence. This is the research the
resolution session needs. Per-feature recipes were extracted by a 10-agent read-only census (all cited to
file:line); the **Inventory Spatial** case was hand-verified against `CkInventory_Spatial_Processor.cpp:62-175`.

## BLOCKER (why Phase 1 stopped) — see PROGRESS.md §Blockers [B1]

PHASE_1.md's model: all 12 `*_ReplicateOnRestore` processors are *container re-seeds* (read live Current-state →
build `FCk_RepData_*` → `TryAddContainerFragment` + optional `MayRequireReplication` re-arm), so a generic
`Produce`/`SeedContainer` re-drive replaces them and §1.4 deletes them "behavior-neutral under Model A".

**That is true for 8 of the 12, and FALSE for 4.** These four restore processors also perform **non-container
reconstitution / cross-entity orchestration** that the `Produce`/`SeedContainer` contract does not model and the
generic re-drive (§1.3) cannot perform — and which §1.4's wholesale deletion would DROP, breaking restore under
Model A (Phase 1 is still Model A):

| Feature | Non-container work the restore processor does (verified/cited) | Why Produce/SeedContainer can't house it |
|---|---|---|
| **Inventory Spatial** | re-derive shape tag + PreviousItems cache; **wait on CkGrid `FProcessor_2dGridSystem_RestoreRecompose`**; **re-stamp every item onto the rebuilt grid** (`Request_PlaceItemOnGrid`, multi-tick retry); **re-replicate each item entity** (`Request_TryReplicateExisting`, multi-tick retry). `CkInventory_Spatial_Processor.cpp:79-174` (HAND-VERIFIED) | cross-processor ordering dep; child-entity re-replication ≠ "seed a container"; payload is only trustworthy AFTER the re-stamp, but the re-drive calls Produce BEFORE SeedContainer → circular |
| **Inventory DataOnly** | re-derive shape tag + cache; per-item entity re-replication (`Request_TryReplicateExisting`, retry) | same child-re-replication problem; owner-hosted container |
| **RenderTarget** | first-pass-only (`NOT HadCurrent`) runtime reconstitution: AddOrGet Current/PixelSync/ClientStaging, strip transient pixel tags, re-derive label, re-seed Interval chrono, **re-create `UTextureRenderTarget2D`**, restore `_NextBatchSeq`, **repaint by replaying the batch ring** (`DoApplyBatch`). Owner(ContextOwner)-hosted container, channel-merge by SyncName. `CkRenderTarget_Processor.cpp:397-510` (agent-cited) | must run exactly ONCE, not per retry (repaint accumulates alpha); re-creating a UObject + replaying a draw ring is not "seed a container"; PHASE_4B already lists "RenderTarget re-author" |
| **2dGridOccupancy** | unconditional `AddOrGet<FFragment_2dGridOccupancy_Current>()` for **ALL** grids incl. local-only (so StampCells rebuilds); wait on `FProcessor_2dGridSystem_RestoreRecompose`. `Ck2dGridOccupancy_Processor.cpp:122-165` (agent-cited) | the derived re-seed must fire even where Produce returns UNSET (local-only grid) — but the re-drive skips SeedContainer when Produce is unset |

**The architecture decision the plan did not make:** where does this reconstitution/orchestration go? Candidates
(NOT chosen here — this is the maintainer/CTO's call): (a) keep slimmed `*_ReplicateOnRestore` processors for these
4 features doing ONLY the reconstitution, and use Produce for the container payload; (b) add a third re-drive hook
(`Reconstitute(Entity)`, runs unconditionally with retry, distinct from the container path) to the generic driver;
(c) accept that under Model B (Phase 3, Construct re-runs) most of this becomes moot and DEFER these 4 features'
deletion to Phase 3/4 while migrating only the 8 clean ones in Phase 1. Note (c) interacts with N1 in the CTO
addendum and with PHASE_4B (RenderTarget) / the grid RestoreRecompose ordering.

Also relevant: **AnimPlan** is a soft case — its restore is owner-resident *empty-seed-and-defer* (reads nothing,
seeds an empty `FCk_RepData_AnimPlans` on the LifetimeOwner, re-arms so the Replicate pass fills it), asymmetric to
MontagePlayer's self-resident payload-seed. It fits the model IF Produce may emit an empty/default payload and
SeedContainer is parameterized on container-host (self vs owner). Not itself blocking, but the container-host
parameterization it forces is shared with Inventory/RenderTarget.

## The 8 clean features — Produce/SeedContainer recipes (ready to implement once [B1] is resolved)

`Produce(Entity) -> TOptional<FInstancedStruct>`: gate on feature presence (return unset if absent), else emit the
RepData. `SeedContainer` default = typed `TryAddContainerFragment<RepData>`; extras noted. Delete the named
`*_ReplicateOnRestore` processor class + its `CK_REGISTER_PROCESSOR` + its `FTag_*_RestoreReplicated` done-tag
(all `CK_DEFINE_ECS_TAG_TRANSIENT`). **KEEP** each feature's per-frame `*_Replicate` processor and its `Apply`
registrar. Never touch `ck::FTag_Snapshot_JustRestored` (multi-consumer).

| Feature | RepData | Produce reads | SeedContainer extras | Delete |
|---|---|---|---|---|
| Velocity | `FCk_RepData_Velocity{Get_CurrentVelocity()}` | `FFragment_Velocity_Current` | none | `FProcessor_Velocity_ReplicateOnRestore`, `FTag_Velocity_RestoreReplicated` (CkVelocity_Processor.cpp:32 reg; Fragment.h:35 tag). KEEP `FProcessor_Velocity_Replicate`. |
| Acceleration | `FCk_RepData_Acceleration{Get_CurrentAcceleration()}` | `FFragment_Acceleration_Current` | none | `ck::FProcessor_Acceleration_ReplicateOnRestore`, `FTag_Acceleration_RestoreReplicated` (Processor.cpp:20; Fragment.h:34) |
| Team | `FCk_RepData_Team{Get_TeamID()}` (read the FRAGMENT `FFragment_TeamInfo::Get_TeamID()`, NOT `UCk_Utils_Team_UE::Get_ID` which reads the not-yet-restored tag) | `FFragment_TeamInfo` | **re-derive `FTag_TeamID<>` from TeamID UNCONDITIONALLY, before/independent of the driver gate** (was `DoRestoreTeamIDTag`, CkTeam_Processor.cpp:118-147). Team has NO steady-state Replicate — the seeded entry is the only push. | `FProcessor_Team_ReplicateOnRestore`, `FTag_Team_RestoreReplicated` |
| Player | `FCk_RepData_Player{Get_PlayerID()}` | `FFragment_PlayerInfo` | **re-derive `FTag_PlayerID<>` UNCONDITIONALLY (pre-driver-gate)** (was `DoRestorePlayerIDTag`, CkPlayer_Processor.cpp:43-68 — a PRIVATE STATIC on the deleted class; relocate it) | `FProcessor_Player_ReplicateOnRestore`, `FTag_Player_RestoreReplicated` |
| Attribute ×5 (Float/Integer/Byte/Vector/Rotator) | `FCk_RepData_<Kind>Attributes{Attributes=[...]}` — one `BaseFinal{name,base,final,component}` per PRESENT component (Current always; Min if `Has<...Min>`; Max if `Has<...Max>`). Logic lives in `TProcessor_Attribute_Replicate::ForEachEntity` (`CkAttribute_Processor.inl.h:221-255`), NOT in the restore processor (which reads nothing). Name via `UCk_Utils_GameplayLabel_UE::Get_Label`. | `TFragment_<Kind>Attribute<Current>` | resolve `Get_LifetimeOwner` (container is OWNER-hosted); gate on owner driver; **`AddOrGet<FTag_MayRequireReplication>` on Current (always), Min/Max (if present)**; **UPSERT-merge into owner container keyed by (name,component)** — NOT overwrite (multiple attrs share one owner container). Shared template `ck::TProcessor_Attribute_ReplicateOnRestore_All` (`CkAttribute_ReplicateOnRestore.h:38-100`) + 5 per-kind aliases + 5 `CK_REGISTER_PROCESSOR` (each `Ck<Kind>Attribute_Processor.cpp:16`) + shared `FTag_Attribute_RestoreReplicated` (ReplicateOnRestore.h:17). | all of the above |
| TagSet | `FCk_RepData_TagSet{Tags=Get_Tags()}` (shape from `FProcessor_TagSet_Replicate`, CkTagSet_Processor.cpp:131; restore reads nothing) | `ck::FFragment_TagSet` | `AddOrGet<FTag_TagSet_MayRequireReplication>` (self-resident) | `FProcessor_TagSet_ReplicateOnRestore` (reg :19), `FTag_TagSet_RestoreReplicated` |
| MontagePlayer | `FCk_RepData_MontagePlayer{Get_State()}` (self-resident) | `FFragment_MontagePlayer_Current` | driver gate on self; `AddOrGet<FTag_MontagePlayer_MayRequireReplication>` | `ck::FProcessor_MontagePlayer_ReplicateOnRestore` (reg :25), `FTag_MontagePlayer_RestoreReplicated` |
| AnimPlan (soft — see above) | today: EMPTY `FCk_RepData_AnimPlans{}` (owner-resident, defers to Replicate). Real payload (if the model wants it): upsert `FCk_AnimPlan_State{Params.AnimGoal, Current.AnimCluster, Current.AnimState}` on owner. | `FFragment_AnimPlan_Params/Current` | owner-resolve + owner-driver gate; **`AddOrGet<FTag_AnimPlan_MayRequireReplication>` on InHandle (NOT owner)** | `ck::FProcessor_AnimPlan_ReplicateOnRestore` (reg :16), `FTag_AnimPlan_RestoreReplicated` |

StateMachine is correctly SKIP (Phase 4A). Full agent output (with every citation) is in the session transcript
workflow `wf_a21ede26-419`; distilled here.

## 1.1 registry-extension design (implemented then reverted with the divergence — re-usable verbatim)

The registry extension itself is sound and orthogonal to [B1]; it was implemented and reverted only to keep the
tree at the gated-green Phase-0 boundary (it was never build-verified — the template needs an instantiation to
compile-prove, which requires a migration). Re-apply when [B1] is resolved:

- `CkReplicatedFragmentContainer.h`: `#include "CkCore/Enums/CkEnums.h"` (for `ECk_AddedOrNot`); add
  `enum class ECk_PersistenceTransport : uint8 { Net=1<<0, Save=1<<1, NetAndSave=Net|Save };`; add to `FHandler`:
  `TFunction<TOptional<FInstancedStruct>(FCk_Handle&)> Produce;`,
  `TFunction<ECk_AddedOrNot(FCk_Handle&, const FInstancedStruct&)> SeedContainer;`,
  `ECk_PersistenceTransport Transport = ECk_PersistenceTransport::Net;`; declare
  `template <typename T_RepData> static auto RegisterLazyTyped(FHandler) -> void;`.
- **Include-surface decision (the one §1.1 delegated to the executor):** the registry header CANNOT include
  `CkNet_Utils.h` (it includes the registry header — cycle). So the `RegisterLazyTyped<T>` body goes in a new
  `CkReplicatedFragmentContainer.inl.h` (no includes; matches the house `.inl.h` convention) that is `#include`d
  **at the very bottom of `CkNet_Utils.h`**, where `UCk_Utils_Net_UE::TryAddContainerFragment` and the registry
  class are both complete. Every feature registrar `.cpp` already includes `CkNet_Utils.h`, so it picks up
  `RegisterLazyTyped` with ZERO new include surface. Body: if `NOT InHandler.SeedContainer`, synthesize the
  default captureless `[](E,D){ return UCk_Utils_Net_UE::TryAddContainerFragment<T_RepData>(E, D.Get<T_RepData>()); }`;
  then `RegisterLazy([]{ return T_RepData::StaticStruct(); }, MoveTemp(InHandler))`. (Captureless → fence 1/2 safe.)
