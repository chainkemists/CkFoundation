# Feature census — request structs & completion shapes (recon snapshot 2026-07-25)

> **TOMBSTONED 2026-07-27 — superseded by the shipped rollout.** Every module below now carries the
> completion delegate, so the "Completion today" column is historical. Kept for the record of what
> the pre-rollout landscape looked like.
>
> **This census had a structural flaw worth remembering: it is keyed on modules that declare
> `FCk_Request_*` structs, so it MISSES modules whose `Request_*` mutate inline and therefore
> declare no struct.** That omitted CkAttribute, CkAStar, CkCue, CkDynamic, CkRecord, CkSubstep,
> CkUnrealComponent and CkUsf — all of which were in scope as immediate mutators and had to be
> caught by a second pass. **Do not use a module-level census as a completeness criterion.** Sweep
> per DECLARATION: every UFUNCTION `Request_*` taking an `FCk_Handle` that lacks the delegate.
>
> Original header: recon output (Opus survey, spot-checked by the orchestrator at the
> CkEcs/CkInventory/CkSignal anchors). Row-level detail is UNVERIFIED-per-row. Tier column from
> `Source/CLAUDE.md` module tier table.
>
> Completion key: **A** = request-handle + result guard (the target shape) · **B** = spawned
> request entity + signal · **C** = delegate-in-request-struct (to be deleted, G0-D4) ·
> **D** = entity-level `BindTo_On*` only · **—** = none.

| Module | Tier | Request structs (`FCk_Request_*`) | Completion today |
|---|---|---|---|
| CkEcs | T2 | `EntityScript_SpawnEntity`, `EntityScript_Replicate`, `ReplicationDriver_ReplicateEntity` (+ C++-only `ck::FRequest_EntityScript_Replicate`) | D + `Promise_*` |
| CkEcsExt | T3 | `SceneNode_UpdateRelativeTransform`; `Transform_{SetLocation,SetRotation,SetScale,SetTransform,SetLocationAndRotation,AddLocationOffset,AddRotationOffset,ForceRefresh}` | D |
| CkActor | T3 | `ActorModifier_{SpawnActor,AddActorComponent,RemoveActorComponent,AttachActor}` | B (3 of 4; `AttachActor` none) |
| CkActorRelay | T4 | `ActorRelay_AcquireChannel` | D (`Promise_OnAcquired`) |
| CkAggro | T4 | `AggroTarget_{AddThreat,SetThreat,MarkPerceived,MarkUnperceived,ResetPerception,Forget}`; `Aggro_{AddThreat,RemoveTarget,ClearAllTargets,SetActiveTarget,ClearActiveTarget}` | D |
| CkAnimation | T4 | `AnimPlan_{UpdateAnimCluster,UpdateAnimState}`; `MontagePlayer_{Play,Stop,Pause,Resume,JumpToSection}` | D |
| CkAudio | T4 | `AudioDirector_{AddTrack,StartTrack,StopTrack,StopAllTracks}`; `AudioTrack_{Play,Stop,SetVolume}` | D |
| CkCamera | T4 | `CameraShake_{PlayAtLocation,PlayOnTarget}`; `Camera_{AddLayer,RemoveLayer}` | — |
| CkChaos | T4 | `GeometryCollection_ApplyRadialStrain`; `GeometryCollectionOwner_ApplyRadialStrain_Replicated` | — |
| CkCompass | T4 | `Compass_{SetObserver,SetCategoryFilter,SetManualHeading}` | D |
| CkCrowd | T4 | `CrowdAgent_{MoveTo,FollowTarget,Stop,SetMaxSpeed}` | D |
| CkDialog | T4 | `DialogEmitter_{Query,StartCooldown,ClearCooldown,ClearAllCooldowns}` | C (`Query._OnComplete`) + D |
| CkEntityCollection | T4 | `EntityCollection_{AddEntities,RemoveEntities}` | D |
| CkEntityTag | T4 | `EntityTag_{Add,TryRemove,AddGameplayTag,TryRemoveGameplayTag,RestoreSet}`; `EntityTagQuery_{AddRequirement,RemoveRequirement}` | D |
| CkEqs | T4 | `Eqs_RunQuery` | C (`_OnComplete`) + D |
| CkFx | T4 | `Sfx_{PlayAtLocation,PlayAttached}`; `Vfx_{PlayAtLocation,PlayAttached,Stop}` | — |
| CkGoap | T4 | `Goap_Planner_{Plan,CancelPlan,SetGoal,SetActionCost,RegisterActionCostProvider,SetCostThreshold,SetReplanInterval,SetReplanPolicy,SetSearchBudget}`; `Goap_WorldState_{RegisterKey,SetValue}` | D |
| CkGraphics | T4 | `RenderStatus_QueryRenderedActors` | B |
| CkGrid | T4 | `2dGridBlocker_SetActive` | D |
| CkInteraction | T4 | `InteractSource_{StartInteraction,CancelInteraction}`; `InteractTarget_CancelInteraction`; `Interaction_EndInteraction`; `InteractionResolver_{AddInteractTarget,RemoveInteractTarget,RemoveAllTargetsByChannel,StartIntent,StopIntent}` | D |
| CkInventory | T4 | `Inventory_{AddItem,RemoveItem,StackItems,SplitStack,AddItemByDefinition,Sort,TransferItem_ToSpatial,TransferItem_ToDataOnly,MassTransfer}`; `Inventory_Spatial_RelocateItem`; `ItemQuery_QueryDefinitions` | **A — reference implementation** |
| CkIskmRenderer | T4 | 18 `IskmProxy_*` (PlayAnimation … EndRagdoll) | D |
| CkIsmRenderer | T4 | `IsmProxy_{EnableDisable,SetCustomInstanceData,SetCustomInstanceDataValue,SetCustomPrimitiveData}` | — |
| CkJolt | T4 | `JoltBody_{AddForce,AddForceAtLocation,AddImpulse,AddImpulseAtLocation,AddTorque,AddAngularImpulse,SetLinearVelocity,SetAngularVelocity,SetSleepState,Teleport}`; `JoltCharacter_{Move,Jump,Teleport}`; `JoltConstraint_{SetEnabled,Hinge_SetMotor,Distance_SetRange}` | D |
| CkLagCompensation | T4 | `LagCompProjectile_LaunchCompensated` | D |
| CkMinimap | T4 | `FogOfWar_{AddRevealer,RemoveRevealer,Reset,RevealAll,RevealLocation,SetExplored}`; `Minimap_{SetObserver,SetCategoryFilter,SetFogOfWar,SetRotationMode,SetViewExtent}` | D |
| CkNavigation | T4 | `Nav_FindPath` | D (`OnPathReady`/`OnPathFailed`, but `Request_FindPath` takes no delegate) |
| CkObjective | T4 | `Objective_{Start,Complete,Fail}`; `ObjectiveOwner_{AddObjective,RemoveObjective}` | D |
| CkOverlapBody | T4 | `Marker_{EnableDisable,Resize}`; `Sensor_{EnableDisable,Resize,OnBeginOverlap,OnEndOverlap,OnBeginOverlap_NonMarker,OnEndOverlap_NonMarker}` | D |
| CkPathNetwork | T4 | `PathNetwork_Rebuild`; `PathNetworkFollower_FindRoute` | D |
| CkPhysics | T4 | `BulkAccelerationModifier_{AddTarget,RemoveTarget}`; `BulkVelocityModifier_{AddTarget,RemoveTarget}` | — |
| CkPmg | T4 | `Pmg_DebugShape_{SetColor,SetDrawLines,SetDuration,SetEnableCollision,SetLineThickness,SetRenderMode,SetText}`; `Pmg_Donut_UpdateParams` | — |
| CkProjectile | T4 | `Projectile_CalculateAimAhead`; `BallisticMotion_{Launch,Stop}`; `Homing_{SetTargetEntity,SetTargetLocation,ClearTarget,EnableDisable,SetDesiredTimeToImpact}` | B (`CalculateAimAhead`); D rest |
| CkRaySense | T4 | `RaySense_EnableDisable` | D |
| CkRenderTarget | T4 | `RenderTarget_{Clear,DrawLine,DrawBox,DrawBorder,DrawText,DrawTexture,DrawMaterial,DrawPolygon,DrawTriangles,SyncPixels}` | D |
| CkResolver | T4 | `ResolverSource_InitiateNewResolution`; `ResolverTarget_InitiateNewResolution`; `ResolverDataBundle_{MetadataOperation,ModifierOperation}` | A (both `InitiateNewResolution`); — (DataBundle ops) |
| CkResourceLoader | T2 | `ResourceLoader_{LoadObject,LoadObjectBatch}` | B |
| CkShapes | T4 | `Shape{Box,Capsule,Cylinder,Sphere}_UpdateDimensions` | D |
| CkSpatialQuery | T4 | `Probe_{BeginOverlap,EndOverlap,EnableDisable}` (+ `Probe_OverlapUpdated`) | D |
| CkStateMachine | T4 | `Sm_{Start,Stop,Pause,Resume,Transition,AddOverrideState}`; `SmDebug_RecordTransition` | D (a rejected `Transition` is indistinguishable from a pending one today) |
| CkTagSet | T2 | `TagSet_{AddTags,RemoveTags}` | D |
| CkTimer | T4 | `Timer_{Manipulate,Jump,Consume,ChangeDirection}` | D (`OnTimerDone` = timer elapsed, NOT request completion) |
| CkTween | T4 | `Tween_{Stop,Pause,Resume,Restart,SetTimeMultiplier}` | D |
| CkUI | T4 | `WorldSpaceWidget_{SetLocationInfo,SetScalingInfo,SetFadingInfo,SetOcclusionInfo}` | — |
| CkVat | T4 | `VatProxy_{PlayClip,Stop,SetPlayRate}` | D |
| CkVfx | T4 | `VfxCue_{Play,Stop}` | D |
| CkVisibleRange | T4 | `VisibleRange_{ApplyRangeState,SetVisibility}` | D |
| CkChaos/CkPhysics/CkCamera/CkFx/CkIsmRenderer/CkPmg/CkUI | — | (rows above) | — modules with NO completion today |

Modules with **no request structs** (nothing to do beyond verification): CkActorRelay-adjacent
promise surfaces, CkAttribute (mutates via modifier entities), CkDynamic, CkEntityExtension,
CkGameSession, CkInput, CkLabel, CkMessaging, CkRecord, CkRelationship, CkSnapshot, CkSubstep,
CkUnrealComponent, CkVariables, CkPerception, CkTargeting, CkSpline, CkAStar, CkPoi,
CkPoiDisplayDefinition, CkCue, CkWatermark, CkGgraphics-remainder. Re-verify per gate.

Key anchor citations (orchestrator-VERIFIED 2026-07-25):
- `CkEcs/Request/CkRequest_Data.h:19-107` bases; `:111-121` debug-name macro; `:136-168` result guard.
- `CkEcs/Signal/CkSignal_Macros.h:44-49` promise / request-fulfilled binds.
- `CkInventory/Inventory/CkInventory_Utils.cpp:192-216` reference Utils shape.
- `CkInventory/Inventory/CkInventory_RequestHandlers.h:176-242` `DispatchCancel` teardown pattern.
- `CkEqs/Query/CkEqs_Processor.cpp:121-130` IsBound-gated bind (recon-cited, re-verify at Gate 01).
