# LiveTune coverage triage — CkFoundation

**Generated:** 2026-08-06 · **Regenerate:** `python docs/campaigns/2026-08-06-LiveTune-Sweep/triage.py`

LiveTune shipped with 3 features registered out of 114 params-data types. This is the mechanical
sizing of the remaining work: which features are a one-line opt-in, and which need real engineering.

## How a feature is classified

Every feature exposes an `FCk_Fragment_<X>_ParamsData`. Some also retain it on the entity as
`ck::FFragment_<X>_Params` (a `using` alias). Whether a processor reads that fragment *after* Add is
what decides the tier:

| Bucket | Count | Means | Tier |
|---|---:|---|---|
| **A — live-read** | 53 | Params fragment is read by a non-Setup processor, so it is consulted every tick | `ViaReplace` — one line, `.PostReplace` only if derived state is cached |
| **B — setup-baked** | 2 | Params fragment retained but read ONLY by a Setup pass | `ViaRequest` or `ViaRebuild` |
| **C1 — decomposed at Add** | 40 (21 real) | No params fragment exists at all; Add explodes the params into other fragments | `ViaRebuild` — the FloatAttribute shape |
| **C2 — utils-only** | 16 | Params fragment exists but no processor reads it (consumed in Utils) | Investigate: usually `ViaReplace` + `.PostReplace`, sometimes `ViaRequest` |
| **Registered** | 3 | Timer, FloatAttribute, Probe | — |

**19 of the 40 C1 entries are `Multiple*` bulk-add containers** (`MultipleTimer`,
`MultipleFloatAttribute`, …). Those are construction-time arrays of another feature's params — there
is no single entity whose params they are, and retuning one is ambiguous by construction. They are
**out of scope**; retune the individual features instead. That puts the real surface at **95**, not
114.

The classifier is a heuristic that sizes buckets. It does not replace per-feature verification — the
sweep confirms each one before registering it.

**Control that validates the method:** `FloatAttribute` is registered `ViaRebuild` and lands in C1
with zero params-fragment references, which is exactly why it could not be `ViaReplace`. The
classifier independently reproduces a tier decision that was reached by hand during the campaign.

---

## Bucket A — 53 live-read (`ViaReplace` candidates)

Evidence column is the non-Setup processor(s) that read the params fragment.

| Feature | Read by |
|---|---|
| 2dGridBlocker | `FProcessor_2dGridBlocker_Requests` |
| 2dGridSystem | `FProcessor_2dGridSystem_DebugDrawAll` |
| Acceleration | `FProcessor_BulkAccelerationModifier_AddNewTargets` |
| AnimPlan | `FProcessor_AnimPlan_HandleRequests`, `_Replicate` |
| AudioDirector | `FProcessor_AudioDirector_EndPlay`, `_HandleRequests` |
| AudioTrack | `FProcessor_AudioTrack_DebugDraw_*` |
| AutoReorient | `FProcessor_AutoReorient_OrientTowardsVelocity` |
| BallisticMotion | `FProcessor_BallisticMotion_HandleImpacts`, `_HandleRequests`, `_UpdateTrajectory` |
| BulkAccelerationModifier | `FProcessor_BulkAccelerationModifier_HandleRequests` |
| BulkVelocityModifier | `FProcessor_BulkVelocityModifier_HandleRequests` |
| CameraShake | `FProcessor_CameraShake_HandleRequests` |
| Compass | `FProcessor_Compass_EndPlay`, `_HandleRequests`, `_Update` |
| CrowdAgent | `FProcessor_CrowdAgent_AccelClamp`, `_AvoidanceSample`, `_BlockDetect` |
| DialogEmitter | `FProcessor_DialogEmitter_EvaluateQueries`, `_HandleRequests` |
| EntityCollection | `FProcessor_EntityCollection_FireSignals`, `_Replicate` |
| FogOfWar | `FProcessor_FogOfWar_HandleRequests`, `_Update` |
| GeometryCollection | `FProcessor_GeometryCollection_CrumbleNonActiveClusters`, `_HandleRequests`, `_RemoveAllAnchors` |
| Homing | `FProcessor_Homing_HandleRequests`, `_Update` |
| InteractSource | `FProcessor_InteractSource_EndPlay`, `_HandleRequests` |
| InteractTarget | `FProcessor_InteractTarget_EndPlay`, `_HandleRequests` |
| Interaction | `FProcessor_Interaction_EndPlay`, `_HandleRequests` |
| Inventory | `FProcessor_Inventory_DataOnly_Replicate`, `_FireSignals`, `_Spatial_Replicate` |
| IskmProxy | `FProcessor_IskmProxy_HandleRequests` |
| IsmProxy | `FProcessor_IsmProxy_AddInstance`, `_EndPlay`, `_EnsureStaticNotMoved_DEBUG` |
| JoltBody | `FProcessor_JoltBody_EndPlay` |
| JoltCharacter | `FProcessor_JoltCharacter_EndPlay`, `_PreStep` |
| JoltConstraint | `FProcessor_JoltConstraint_HandleRequests` |
| Marker | `FProcessor_Marker_EndPlay`, `_HandleRequests`, `_UpdateTransform` |
| Minimap | `FProcessor_Minimap_EndPlay`, `_HandleRequests`, `_Update` |
| MontagePlayer | `FProcessor_MontagePlayer_HandleRequests`, `_MonitorAnimInstance` |
| PathNetwork | `FProcessor_PathNetwork_HandleRequests` |
| PathNetworkFollower | `FProcessor_PathNetworkFollower_HandleRequests`, `_InvalidateOnRebuild` |
| RaySense | `FProcessor_RaySense_*Sweep_Update` |
| RenderTarget | `FProcessor_RenderTarget_ApplyClientBatches`, `_ClientNetMaintenance`, `_DispatchPixelPayload` |
| ResolverDataBundle | `FProcessor_ResolverDataBundle_Calculate`, `_StartNewPhase` |
| ResolverSource | `FProcessor_ResolverSource_HandleRequests` |
| RewindHistory | `FProcessor_RewindHistory_Record` |
| Sensor | `FProcessor_Sensor_EndPlay`, `_HandleRequests`, `_UpdateTransform` |
| Sfx | `FProcessor_Sfx_HandleRequests` |
| ShapeBox / ShapeCapsule / ShapeCylinder / ShapeSphere | `FProcessor_Shape*_HandleRequests` |
| Tween | `FProcessor_Tween_ApplyToTransform`, `_HandleYoyoDelays`, `_Update` |
| VatProxy | `FProcessor_VatProxy_FireSignals`, `_HandleRequests` |
| Velocity | `FProcessor_BulkVelocityModifier_AddNewTargets` |
| VisibleRange | `FProcessor_VisibleRange_Update_Bucket` |
| VoiceChannel | `FProcessor_VoiceChannel_AssignIdx` |
| VoiceTalker | `FProcessor_VoiceTalker_Capture`, `_HandleRequests` |
| VoxelNavOccluder | `FProcessor_VoxelNavOccluder_Track` |
| VoxelNavPath | `FProcessor_CrowdAgent_OnVoxelPathResolved`, `FProcessor_VoxelNavPath_HandleRequests` |
| VoxelNavVolume | `FProcessor_VoxelNavVolume_Build`, `_HandleRequests`, `_Repair` |
| WorldSpaceWidget | `FProcessor_WorldSpaceWidget_EndPlay`, `_HandleRequests`, `_UpdateLocation` |

> **Caution — "read by a processor" is necessary, not sufficient.** Several rows above are carried
> only by an `_EndPlay` or `_Replicate` processor, which proves the fragment is retained but NOT that
> the params are consulted during normal simulation. Those need the same per-field check as anything
> else; see the Timer precedent in PROMPT.md §3.

## Bucket B — 2 setup-baked

| Feature | Read only by |
|---|---|
| UnrealComponent | `FProcessor_UnrealComponent_Setup` |
| VfxCue | `FProcessor_VfxCue_Setup` |

## Bucket C1 — decomposed at Add (`ViaRebuild` shape)

**In scope (21):** AccelerationModifier · Aggro · AggroTarget · ByteAttribute ·
ByteAttributeModifier · EntityTag · FloatAttributeModifier · FloatAttributeRefill · Goap ·
IntegerAttribute · IntegerAttributeModifier · IntegerAttributeRefill · Poi · Projectile ·
RotatorAttribute · RotatorAttributeModifier · StateMachine · VectorAttribute ·
VectorAttributeModifier · VelocityModifier · Velocity_MinMax

**Out of scope — `Multiple*` bulk-add containers (19):** MultipleAnimAsset · MultipleAnimPlan ·
MultipleByteAttribute · MultipleCameraShake · MultipleEntityCollection · MultipleFloatAttribute ·
MultipleIntegerAttribute · MultipleInteractSource · MultipleInteractTarget · MultipleInteraction ·
MultipleInventory_DataOnly · MultipleInventory_Spatial · MultipleMarker · MultipleRotatorAttribute ·
MultipleSensor · MultipleSfx · MultipleTimer · MultipleVectorAttribute · MultipleVfx

## Bucket C2 — 16 utils-only (investigate individually)

2dGridCell · 2dGridObject · 2dGridPlacement · AnimAsset · Camera · DialogLine · Goap_WorldState ·
Inventory_DataOnly · Inventory_Spatial · Pmg_Donut · PoiDisplayDefinition · PredictedVelocity ·
RenderStatus · ResolverTarget · Spline · Vfx
