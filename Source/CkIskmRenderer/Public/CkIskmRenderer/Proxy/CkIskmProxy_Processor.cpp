#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Utils.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"
#include "CkIskmRenderer/CkIskmSubsystem.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"
#include "CkIskmRenderer/CkIskmRenderer_Stats.h"
#include "CkIskmRenderer/Notify/CkIskmNotify_AnimInstance.h"

// ck::Is_NOT_Valid on UPhysicsAsset* needs the full class definition for the __is_base_of intrinsic.
#include "PhysicsEngine/PhysicsAsset.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Iskm::UpdateTransform"),    STAT_CkIskm_UpdateTransform,    STATGROUP_CkIskmRenderer);
DECLARE_CYCLE_STAT(TEXT("Iskm::SocketFollowerSync"), STAT_CkIskm_SocketFollowerSync, STATGROUP_CkIskmRenderer);
DECLARE_CYCLE_STAT(TEXT("Iskm::SocketFollowerSyncDescendants"), STAT_CkIskm_SocketFollowerSyncDescendants, STATGROUP_CkIskmRenderer);
DECLARE_CYCLE_STAT(TEXT("Iskm::EmitFinishedEvents"), STAT_CkIskm_EmitFinishedEvents, STATGROUP_CkIskmRenderer);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_iskmproxy_processor
{
    // Sets an AnimInstance class on the SKMC and re-wires the notify-forwarder owning handle on the
    // resulting instance. Every site that changes the class must go through here.
    auto DoApply_AnimInstanceClass(
        USkeletalMeshComponent* InSKMC,
        TSubclassOf<UAnimInstance> InClass,
        FCk_Handle_IskmProxy InOwningHandle) -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InSKMC),
            TEXT("DoApply_AnimInstanceClass called with null SKMC for proxy [{}]"),
            InOwningHandle)
        { return; }
        InSKMC->SetAnimInstanceClass(InClass);

        if (auto* IskmAI = Cast<::UCk_IskmNotify_AnimInstance>(InSKMC->GetAnimInstance()))
        {
            IskmAI->Set_OwningProxyHandle(InOwningHandle);
        }
        else if (ck::IsValid(InClass))
        {
            ck::iskm::Warning(
                TEXT("AnimInstance class [{}] does NOT derive from UCk_IskmNotify_AnimInstance for proxy [{}]; OnAnimationNotify and OnMontageFinished will not fire."),
                InClass, InOwningHandle);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Resolved once per tick so ForEachEntity never pays the per-entity world lookup.
    auto
        FProcessor_IskmProxy_Setup::
        DoTick(TimeType InDeltaT) -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IskmProxy_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& InCustomData,
            FFragment_IskmProxy_MaterialOverrides& InMaterialOverrides,
            FFragment_IskmProxy_MorphTargets& InMorphTargets) const -> void
    {
        auto RendererHandle = InParams.Get_Renderer();
        CK_ENSURE_IF_NOT(ck::IsValid(RendererHandle),
            TEXT("IskmProxy Setup: invalid renderer for [{}]"), InHandle)
        { return; }

        auto* RendererData = ::UCk_Utils_IskmRenderer_UE::Get_RendererData(RendererHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(RendererData),
            TEXT("IskmProxy Setup: RendererData invalid for [{}]"), InHandle)
        { return; }
        auto* AnimCollection = RendererData->Get_AnimCollection().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(AnimCollection),
            TEXT("IskmProxy Setup: AnimCollection invalid for [{}]"), InHandle)
        { return; }

        // Validated BEFORE Acquire_BaseSKMC so there is nothing to unwind.
        CK_ENSURE_IF_NOT(ck::IsValid(AnimCollection->Get_DefaultMesh()),
            TEXT("IskmProxy Setup: AnimCollection [{}] has no DefaultMesh for [{}]"),
            GetNameSafe(AnimCollection), InHandle)
        { return; }

        auto* World = _World.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("IskmProxy Setup: cached world is invalid for [{}]"), InHandle)
        { return; }

        auto& RendererCurrent = RendererHandle.Get<FFragment_IskmRenderer_Current>();
        auto* RendererActor = RendererCurrent.Get_RendererActor().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(RendererActor),
            TEXT("IskmProxy Setup: renderer actor missing for [{}]"), InHandle)
        { return; }

#if WITH_EDITOR
        // Editor previews acquire from a PER-OWNER renderer actor so a viewport click on the mesh
        // redirects selection to the placed actor. Release stays owner-derived (SKMC->GetOwner()).
        if (World->WorldType == EWorldType::Editor)
        {
            if (auto* SelectionOwner = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionOwner(InHandle);
                ck::IsValid(SelectionOwner))
            {
                if (auto* Subsystem = World->GetSubsystem<UCk_IskmRenderer_Subsystem_UE>();
                    ck::IsValid(Subsystem))
                {
                    if (auto* PerOwnerRendererActor = Subsystem->GetOrCreate_RendererActor_ForEditorSelectionOwner(RendererData, SelectionOwner);
                        ck::IsValid(PerOwnerRendererActor))
                    { RendererActor = PerOwnerRendererActor; }
                }
            }
        }
#endif

        auto* SKMC = RendererActor->Acquire_BaseSKMC();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy Setup: failed to acquire SKMC for [{}]"), InHandle)
        { return; }

        SKMC->SetSkeletalMesh(AnimCollection->Get_DefaultMesh());

        // Composed entity-space -> world via the spawn rotation; UpdateTransform re-applies it each frame.
        InCurrent._LocalLocationOffset = InParams.Get_LocalLocationOffset();
        auto SpawnXf = InParams.Get_SpawnTransform();
        SpawnXf.AddToTranslation(SpawnXf.GetRotation().RotateVector(InCurrent._LocalLocationOffset));
        SKMC->SetWorldTransform(SpawnXf);

        // Sync-load: first use of an AnimCollection can hitch.
        const auto SoftClass = RendererData->Get_DefaultAnimInstanceClass();
        auto* AnimClass = SoftClass.IsNull() ? nullptr : SoftClass.LoadSynchronous();
        // An UNSET soft ref is normal flow (sequence mode); a SET ref that fails to load is broken
        // content and must be loud, or the proxy silently T-poses.
        CK_ENSURE_IF_NOT(SoftClass.IsNull() || ck::IsValid(AnimClass),
            TEXT("IskmProxy Setup: _DefaultAnimInstanceClass [{}] failed to load for [{}] — falling back to sequence mode"),
            SoftClass.ToString(), InHandle)
        { /* fall through: the sequence-mode fallback below is the recovery */ }
        const auto IsAnimBpMode = ck::IsValid(AnimClass);
        const auto ClassToApply = IsAnimBpMode
            ? TSubclassOf<UAnimInstance>{AnimClass}
            : TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()};

        ck_iskmproxy_processor::DoApply_AnimInstanceClass(SKMC, ClassToApply, FCk_Handle_IskmProxy{InHandle});
        InPoseSource._PoseSource = IsAnimBpMode
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;

        InCurrent._BaseSKMC = SKMC;

        const auto NumCustom = RendererData->Get_NumCustomDataFloat();
        InCustomData._Values.Init(0.0f, NumCustom);
        // SetNumCustomDataFloats is an ISM-only API; on a plain component the CustomPrimitiveData array
        // grows itself as each slot is written.
        for (auto Idx = 0; Idx < NumCustom; ++Idx)
        {
            SKMC->SetCustomPrimitiveDataFloat(Idx, 0.0f);
        }

        for (auto Idx = 0; Idx < RendererData->Get_Submeshes().Num(); ++Idx)
        {
            const auto& Def = RendererData->Get_Submeshes()[Idx];
            if (NOT Def.Get_AttachByDefault())
            { continue; }
            CK_ENSURE_IF_NOT(ck::IsValid(Def.Get_Mesh()),
                TEXT("IskmProxy [{}]: default-attach submesh [{}] in RendererData [{}] has no Mesh set"),
                InHandle, Def.Get_Name(), GetNameSafe(RendererData))
            { continue; }
            auto* Child = NewObject<USkeletalMeshComponent>(RendererActor, USkeletalMeshComponent::StaticClass(), NAME_None, RF_Transient);
            Child->SetupAttachment(SKMC);
            Child->RegisterComponent();
            Child->SetSkeletalMesh(Def.Get_Mesh());
            Child->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Child->SetLeaderPoseComponent(SKMC);
#if WITH_EDITOR
            // Leader-pose followers still gate their bone refresh on bUpdateAnimationInEditor.
            ACk_IskmRenderer_Actor_UE::EditorOnly_EnableAnimationTicking(Child);
#endif
            for (auto MatIdx = 0; MatIdx < Def.Get_OverrideMaterials().Num(); ++MatIdx)
            {
                if (auto* Mat = Def.Get_OverrideMaterials()[MatIdx].Get())
                {
                    Child->SetMaterial(MatIdx, Mat);
                }
            }
            InCurrent._SubmeshSKMCs.Add(Child);
            InCurrent._AttachedSubmeshIndices.Add(Idx);
        }

        for (const auto& Override : InParams.Get_CustomInstanceDataDefaults())
        {
            const auto& StartIdx = Override.Get_CustomDataIndex();
            const auto& FloatArray = Override.Get_Value().ConvertToFloatArray();
            for (auto Offset = 0; Offset < FloatArray.Num(); ++Offset)
            {
                const auto SlotIdx = StartIdx + Offset;
                CK_ENSURE_IF_NOT(InCustomData._Values.IsValidIndex(SlotIdx),
                    TEXT("IskmProxy [{}]: CustomInstanceDataDefaults writes slot [{}] but only [{}] slots are allocated. RendererData._NumCustomDataFloat must cover the requested offset"),
                    InHandle, SlotIdx, InCustomData._Values.Num())
                { continue; }
                InCustomData._Values[SlotIdx] = FloatArray[Offset];
                SKMC->SetCustomPrimitiveDataFloat(SlotIdx, FloatArray[Offset]);
            }
        }

        // Empty on first Setup (requests only drain post-Setup); load-bearing when the SKMC is
        // (re)acquired after overrides were already recorded on this entity.
        for (const auto& Kvp : InMaterialOverrides._SlotToMaterial)
        {
            SKMC->SetMaterial(Kvp.Key, Kvp.Value.Get());
        }
        InMaterialOverrides._Dirty = false;

        for (const auto& Kvp : InMorphTargets._Values)
        {
            SKMC->SetMorphTarget(Kvp.Key, Kvp.Value);
        }
        InMorphTargets._Dirty = false;

        if (InParams.Get_IsMovable() == ECk_EnableDisable::Enable)
        {
            InHandle.Add<FTag_IskmProxy_Movable>();
        }

        InHandle.Remove<FTag_IskmProxy_NeedsSetup>();
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& InCustomData,
            FFragment_IskmProxy_Requests& InRequests,
            const FFragment_Transform& InTransform) const -> void
    {
        // Drain a COPY: handlers broadcast signals synchronously and a listener may enqueue new requests
        // on this same entity mid-drain, which would dangle the loop's iterators on realloc and be
        // discarded by a trailing Reset. Re-entrant requests survive in the fragment for the next tick.
        auto RequestsCopy = MoveTemp(InRequests._Requests);
        InRequests._Requests.Reset();

        ck::algo::ForEachRequest(RequestsCopy, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                // Every DoHandleRequest overload's internal CK_ENSURE_IF_NOT early-outs guard malformed
                // state (missing SKMC, null asset, etc), not a legitimate request-rejection outcome, so
                // reaching the line after the call IS the success condition.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = ck::MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InParams, InCurrent, InAnimState, InPoseSource, InCustomData, InTransform, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;
            }), ck::policy::DontResetContainer{});
    }

    auto
        FProcessor_IskmProxy_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkIskm_UpdateTransform);

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in UpdateTransform processor"),
            InHandle)
        { return; }

        // The plain fragment read is exact even for actor-backed owners: this runs in FGroup_PostTransform,
        // AFTER FGroup_Transform_SyncFrom mirrored the live root component into the fragment this tick.
        auto NewTransform = InTransform.Get_Transform();
        NewTransform.AddToTranslation(NewTransform.GetRotation().RotateVector(InCurrent.Get_LocalLocationOffset()));
        SKMC->SetWorldTransform(NewTransform);
    }

    auto
        FProcessor_IskmProxy_SocketFollower_SyncTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_IskmProxy_SocketFollower& InFollower) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkIskm_SocketFollowerSync);

        auto Leader = InFollower.Get_Leader();

        // Intentional silent return: teardown ordering can destroy the leader a frame before the
        // follower, and holding the last pose through that window is correct.
        if (ck::Is_NOT_Valid(Leader))
        { return; }

        auto NewTransform = FTransform::Identity;

        if (Leader.Has<FTag_IskmProxy_Ragdolling>())
        {
            // Ragdoll breaks the component-space composition below: physics owns the SKMC world pose and
            // UpdateTransform is excluded, so the leader's ENTITY transform is frozen at the death pose.
            // The world socket already carries ComponentToWorld — do NOT re-add _LocalLocationOffset.
            const auto SocketWorldSpace = ::UCk_Utils_IskmProxy_UE::Get_SocketTransform(
                Leader, InFollower.Get_Socket(), ECk_IskmProxy_TransformSpace::World);

            NewTransform = InFollower.Get_Offset() * SocketWorldSpace;
        }
        else
        {
            // The SKMC's world placement is deliberately NOT consulted — it is one frame stale by
            // construction; the root term comes from the leader's live entity transform instead.
            const auto SocketComponentSpace = ::UCk_Utils_IskmProxy_UE::Get_SocketTransform(
                Leader, InFollower.Get_Socket(), ECk_IskmProxy_TransformSpace::Component);

            auto LeaderTransform = ::UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Leader);
            // Must match FProcessor_IskmProxy_UpdateTransform's SKMC placement, or the follower tracks
            // the entity origin instead of the offset body and floats by the offset.
            const auto LeaderOffset = Leader.Get<FFragment_IskmProxy_Current>().Get_LocalLocationOffset();
            LeaderTransform.AddToTranslation(LeaderTransform.GetRotation().RotateVector(LeaderOffset));

            NewTransform = InFollower.Get_Offset() * SocketComponentSpace * LeaderTransform;
        }

        if (InTransform.Get_Transform().Equals(NewTransform))
        { return; }

        const auto ComponentsModified = ::UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(
            InTransform, InPrevTransform, NewTransform);

        if (EnumHasAnyFlags(ComponentsModified,
            ECk_TransformComponents::Location |
            ECk_TransformComponents::Rotation |
            ECk_TransformComponents::Scale))
        {
            InHandle.AddOrGet<FTag_Transform_Updated>();
        }
    }

    auto
        FProcessor_IskmProxy_SocketFollower_SyncDescendants::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_IskmProxy_SocketFollower& InFollower) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkIskm_SocketFollowerSyncDescendants);

        // FTag_Transform_Updated is deliberately NOT a view requirement: this body ADDS that tag to
        // descendant entities, and mutating a pool the view iterates would invalidate iteration. Gate on
        // it here instead — SyncTransform (same group, RunAfter) sets it the frames the follower moved.
        if (NOT InHandle.Has<FTag_Transform_Updated>())
        { return; }

        DoSync_Descendants(InHandle, InTransform.Get_Transform());
    }

    auto
        FProcessor_IskmProxy_SocketFollower_SyncDescendants::
        DoSync_Descendants(
            FCk_Handle_Transform InParent,
            const FTransform& InParentWorld)
        -> void
    {
        UCk_Utils_SceneNode_UE::ForEach_SceneNode(InParent, [&](FCk_Handle_SceneNode InChild)
        {
            if (InChild.Has<FFragment_SceneNode_UnrealAnchor>())
            { return; }

            // A descendant that is itself a socket follower drives its own pose (and subtree) via this
            // same pass; recomputing it here as relative*parentWorld would contest that.
            if (InChild.Has<FFragment_IskmProxy_SocketFollower>())
            { return; }

            // Same skip contract as TProcessor_SceneNode_Update: anchor-authoritative children
            // (RootComponent/MeshSocket without ExternallyDriven) are driven by their anchors.
            if (InChild.Has_Any<FFragment_Transform_MeshSocket, FFragment_Transform_RootComponent>() &&
                NOT InChild.Has<FTag_Transform_ExternallyDriven>())
            { return; }

            const auto NewTransform = InChild.Get<FFragment_SceneNode_Current>().Get_RelativeTransform() * InParentWorld;

            auto ChildAsTransform = UCk_Utils_Transform_UE::Cast(InChild);
            // Scene nodes always carry FFragment_Transform, so this never structurally inserts into a
            // view-member pool mid-iteration. Descendants are assumed non-replicating.
            auto& ChildTransform = ChildAsTransform.AddOrGet<FFragment_Transform>();

            // Unchanged parent-derived pose means the whole subtree below is unchanged too.
            if (ChildTransform.Get_Transform().Equals(NewTransform))
            { return; }

            auto& ChildPrevTransform = ChildAsTransform.AddOrGet<FFragment_Transform_Previous>();
            UCk_Utils_Transform_UE::Apply_SetTransform_DirectWrite(ChildTransform, ChildPrevTransform, NewTransform);
            ChildAsTransform.AddOrGet<FTag_Transform_Updated>();

            DoSync_Descendants(ChildAsTransform, NewTransform);
        });
    }

    auto
        FProcessor_IskmProxy_EmitFinishedEvents::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState) const -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkIskm_EmitFinishedEvents);

        if (InAnimState._LastFinishedDispatched)
        { return; }
        // Intentional silent return: no current sequence is the normal "nothing playing" state.
        auto* Cur = InAnimState._CurrentSequence.Get();
        if (ck::Is_NOT_Valid(Cur))
        { return; }
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in EmitFinishedEvents processor"),
            InHandle)
        { return; }

        if (NOT SKMC->IsPlaying())
        {
            UUtils_Signal_IskmProxy_OnAnimationFinished::Broadcast(
                InHandle,
                MakePayload(InHandle, FCk_IskmProxy_AnimSequenceRef{Cur}, ECk_IskmProxy_AnimFinishReason::Completed));
            // Reset so a subsequent Request_StopAnimation cannot fire a duplicate Stopped event.
            InAnimState._CurrentSequence.Reset();
            InAnimState._LastFinishedDispatched = true;
        }
    }

    auto
        FProcessor_IskmProxy_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in EndPlay processor — the SKMC was released before EndPlay ran"),
            InHandle)
        { return; }

        for (auto& WeakChild : InCurrent._SubmeshSKMCs)
        {
            if (auto* Child = WeakChild.Get())
            {
                Child->DestroyComponent();
            }
        }
        InCurrent._SubmeshSKMCs.Reset();
        InCurrent._AttachedSubmeshIndices.Reset();

        // Pool hygiene (load-bearing): both arrays are component-level state that survives
        // Release_BaseSKMC, so without these clears the proxy's materials and morphs leak to the next
        // borrower. This proxy owns ALL override slots, so a full clear restores mesh defaults exactly.
        SKMC->EmptyOverrideMaterials();
        SKMC->ClearMorphTargets();

        auto* RendererActor = Cast<ACk_IskmRenderer_Actor_UE>(SKMC->GetOwner());
        CK_ENSURE_IF_NOT(ck::IsValid(RendererActor),
            TEXT("IskmProxy [{}]: BaseSKMC has no ACk_IskmRenderer_Actor_UE owner in EndPlay — pooled SKMC leaked"),
            InHandle)
        {
            InCurrent._BaseSKMC.Reset();
            return;
        }
        RendererActor->Release_BaseSKMC(SKMC);
        InCurrent._BaseSKMC.Reset();
    }

    // ---- DoHandleRequest handlers ----
    //
    // One overload per std::variant alternative in FFragment_IskmProxy_Requests::RequestType.

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& InCustomData,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_PlayAnimation& InRequest) const -> void
    {
        if (InPoseSource._PoseSource == ECk_IskmProxy_PoseSource::AnimBP)
        {
            ck::iskm::Verbose(TEXT("PlayAnimation ignored on AnimBP-mode proxy [{}]"), InHandle);
            return;
        }

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in PlayAnimation handler — Setup did not complete or the SKMC was released early"),
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Sequence()),
            TEXT("IskmProxy [{}]: PlayAnimation request has a null Sequence. Caller must supply a valid UAnimSequenceBase"),
            InHandle)
        { return; }

        if (InRequest.Get_Unique() && InAnimState._CurrentSequence.Get() == InRequest.Get_Sequence())
        {
            return;
        }

        // Guarded: SetAnimInstanceClass(nullptr) on an ALREADY-null AnimClass tears down the live
        // SingleNodeInstance before PlayAnimation recreates one, and that gap visibly A-poses the proxy.
        if (SKMC->AnimClass != nullptr)
        {
            SKMC->SetAnimInstanceClass(nullptr);
        }
        SKMC->PlayAnimation(InRequest.Get_Sequence(), InRequest.Get_Loop());
        SKMC->SetPosition(InRequest.Get_StartAt(), false);
        SKMC->SetPlayRate(InRequest.Get_PlayRate());

        if (auto* Old = InAnimState._CurrentSequence.Get();
            ck::IsValid(Old) && Old != InRequest.Get_Sequence())
        {
            UUtils_Signal_IskmProxy_OnAnimationFinished::Broadcast(
                InHandle,
                MakePayload(InHandle, FCk_IskmProxy_AnimSequenceRef{Old}, ECk_IskmProxy_AnimFinishReason::Replaced));
        }
        InAnimState._CurrentSequence = InRequest.Get_Sequence();
        InAnimState._LastFinishedDispatched = false;
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_StopAnimation& /*InRequest*/) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in StopAnimation handler"),
            InHandle)
        { return; }
        SKMC->Stop();

        if (auto* Old = InAnimState._CurrentSequence.Get(); ck::IsValid(Old))
        {
            UUtils_Signal_IskmProxy_OnAnimationFinished::Broadcast(
                InHandle,
                MakePayload(InHandle, FCk_IskmProxy_AnimSequenceRef{Old}, ECk_IskmProxy_AnimFinishReason::Stopped));
            InAnimState._CurrentSequence.Reset();
            InAnimState._LastFinishedDispatched = true;
        }
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetPlayRate& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetPlayRate handler"),
            InHandle)
        { return; }
        SKMC->SetPlayRate(InRequest.Get_Rate());
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetVisibility& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetVisibility handler"),
            InHandle)
        { return; }

        const auto IsVisible = InRequest.Get_IsVisible();
        SKMC->SetVisibility(IsVisible);

        for (const auto& WeakChild : InCurrent.Get_SubmeshSKMCs())
        {
            if (auto* Child = WeakChild.Get(); ck::IsValid(Child))
            {
                Child->SetVisibility(IsVisible);
            }
        }
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& InCustomData,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetCustomDataFloat& InRequest) const -> void
    {
        CK_ENSURE_IF_NOT(InCustomData._Values.IsValidIndex(InRequest.Get_Offset()),
            TEXT("IskmProxy [{}]: SetCustomDataFloat offset [{}] is out of range (allocated slots: [{}]). RendererData._NumCustomDataFloat must cover the requested offset"),
            InHandle, InRequest.Get_Offset(), InCustomData._Values.Num())
        { return; }

        InCustomData._Values[InRequest.Get_Offset()] = InRequest.Get_Value();

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetCustomDataFloat handler"),
            InHandle)
        { return; }

        SKMC->SetCustomPrimitiveDataFloat(InRequest.Get_Offset(), InRequest.Get_Value());
        for (auto& WeakChild : InCurrent._SubmeshSKMCs)
        {
            if (auto* Child = WeakChild.Get())
            {
                Child->SetCustomPrimitiveDataFloat(InRequest.Get_Offset(), InRequest.Get_Value());
            }
        }
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetMaterialOverride& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetMaterialOverride handler"),
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Material()),
            TEXT("IskmProxy [{}]: SetMaterialOverride request has a null Material for slot [{}]. Use Request_ClearMaterialOverrides to restore mesh-default materials"),
            InHandle, InRequest.Get_SlotIndex())
        { return; }

        CK_ENSURE_IF_NOT(InRequest.Get_SlotIndex() >= 0 && InRequest.Get_SlotIndex() < SKMC->GetNumMaterials(),
            TEXT("IskmProxy [{}]: SetMaterialOverride slot [{}] is out of range (mesh has [{}] material slots)"),
            InHandle, InRequest.Get_SlotIndex(), SKMC->GetNumMaterials())
        { return; }

        // Guaranteed-present Get: Add(...) composes MaterialOverrides unconditionally. It is not threaded
        // through the processor signature — re-threading every overload for two consumers isn't worth it.
        auto& Overrides = InHandle.Get<FFragment_IskmProxy_MaterialOverrides>();
        Overrides._SlotToMaterial.Add(
            InRequest.Get_SlotIndex(),
            TStrongObjectPtr<UMaterialInterface>{InRequest.Get_Material().Get()});
        Overrides._Dirty = true;

        SKMC->SetMaterial(InRequest.Get_SlotIndex(), InRequest.Get_Material());
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_ClearMaterialOverrides& /*InRequest*/) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in ClearMaterialOverrides handler"),
            InHandle)
        { return; }

        auto& Overrides = InHandle.Get<FFragment_IskmProxy_MaterialOverrides>();

        // Intentional silent return: clearing with no recorded overrides is a no-op.
        if (Overrides._SlotToMaterial.IsEmpty())
        { return; }
        Overrides._SlotToMaterial.Reset();
        Overrides._Dirty = true;

        // This proxy owns ALL override slots, so a component-level clear restores mesh defaults exactly.
        SKMC->EmptyOverrideMaterials();
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetMorphTarget& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetMorphTarget handler"),
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(InRequest.Get_MorphName() != NAME_None,
            TEXT("IskmProxy [{}]: SetMorphTarget request has MorphName == None. Caller must supply a morph-target name"),
            InHandle)
        { return; }

        // Guaranteed-present Get — same rationale as MaterialOverrides.
        auto& Morphs = InHandle.Get<FFragment_IskmProxy_MorphTargets>();
        Morphs._Values.Add(InRequest.Get_MorphName(), InRequest.Get_Value());
        Morphs._Dirty = true;

        SKMC->SetMorphTarget(InRequest.Get_MorphName(), InRequest.Get_Value());
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_ClearMorphTargets& /*InRequest*/) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in ClearMorphTargets handler"),
            InHandle)
        { return; }

        auto& Morphs = InHandle.Get<FFragment_IskmProxy_MorphTargets>();

        // Intentional silent return: clearing with no recorded morphs is a no-op.
        if (Morphs._Values.IsEmpty())
        { return; }
        Morphs._Values.Reset();
        Morphs._Dirty = true;

        SKMC->ClearMorphTargets();
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& InCustomData,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetSkeletalMesh& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetSkeletalMesh handler"),
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Mesh()),
            TEXT("IskmProxy [{}]: SetSkeletalMesh request has a null Mesh"),
            InHandle)
        { return; }

        constexpr auto ReinitPose = true;
        SKMC->SetSkeletalMesh(InRequest.Get_Mesh(), ReinitPose);

        // SetSkeletalMesh re-ran InitAnim -> a FRESH AnimInstance of the preserved AnimClass, so the
        // notify bridge's owning handle must be re-established or the signals stop routing.
        if (auto* IskmAI = Cast<::UCk_IskmNotify_AnimInstance>(SKMC->GetAnimInstance()))
        {
            IskmAI->Set_OwningProxyHandle(FCk_Handle_IskmProxy{InHandle});
        }

        // The swap rebuilt the component's material slots / morph curves / custom data.
        auto& Overrides = InHandle.Get<FFragment_IskmProxy_MaterialOverrides>();
        for (const auto& Kvp : Overrides._SlotToMaterial)
        {
            if (Kvp.Key >= 0 && Kvp.Key < SKMC->GetNumMaterials())
            {
                SKMC->SetMaterial(Kvp.Key, Kvp.Value.Get());
            }
        }

        auto& Morphs = InHandle.Get<FFragment_IskmProxy_MorphTargets>();
        for (const auto& Kvp : Morphs._Values)
        {
            SKMC->SetMorphTarget(Kvp.Key, Kvp.Value);
        }

        for (auto Idx = 0; Idx < InCustomData._Values.Num(); ++Idx)
        {
            SKMC->SetCustomPrimitiveDataFloat(Idx, InCustomData._Values[Idx]);
        }
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_AttachSubmesh& InRequest) const -> void
    {
        auto* RendererData = ::UCk_Utils_IskmRenderer_UE::Get_RendererData(InParams.Get_Renderer());
        CK_ENSURE_IF_NOT(ck::IsValid(RendererData),
            TEXT("IskmProxy [{}]: RendererData missing in AttachSubmesh handler — Renderer handle [{}] no longer resolves"),
            InHandle, InParams.Get_Renderer())
        { return; }

        const auto Idx = RendererData->Find_SubmeshIndex_ByName(InRequest.Get_SubmeshName());
        CK_ENSURE_IF_NOT(Idx != INDEX_NONE,
            TEXT("IskmProxy [{}]: AttachSubmesh requested submesh named [{}] but no such entry exists in RendererData [{}]._Submeshes"),
            InHandle, InRequest.Get_SubmeshName(), GetNameSafe(RendererData))
        { return; }

        // Intentional dedup: re-attaching an already-attached submesh is a no-op.
        if (InCurrent._AttachedSubmeshIndices.Contains(Idx))
        { return; }
        // The cap is the Plan-2 GPU custom-data bitmask (mesh presence packs into 4 bits = 15 slots);
        // game code that exceeds it today would silently break under the batched path.
        CK_ENSURE_IF_NOT(InCurrent._AttachedSubmeshIndices.Num() < RendererData->Get_MaxSubmeshPerInstance(),
            TEXT("IskmProxy [{}]: cannot attach submesh [{}] — already at MaxSubmeshPerInstance ({})"),
            InHandle, InRequest.Get_SubmeshName(), RendererData->Get_MaxSubmeshPerInstance())
        { return; }

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in AttachSubmesh handler"),
            InHandle)
        { return; }

        auto* Owner = Cast<ACk_IskmRenderer_Actor_UE>(SKMC->GetOwner());
        CK_ENSURE_IF_NOT(ck::IsValid(Owner),
            TEXT("IskmProxy [{}]: BaseSKMC has no ACk_IskmRenderer_Actor_UE owner in AttachSubmesh handler"),
            InHandle)
        { return; }

        const auto& Def = RendererData->Get_Submeshes()[Idx];
        // Validated BEFORE creating the child SKMC: a null Mesh would produce an invisible submesh that
        // still consumes one of the capped MaxSubmeshPerInstance slots.
        CK_ENSURE_IF_NOT(ck::IsValid(Def.Get_Mesh()),
            TEXT("IskmProxy [{}]: submesh [{}] in RendererData [{}] has no Mesh set"),
            InHandle, InRequest.Get_SubmeshName(), GetNameSafe(RendererData))
        { return; }
        auto* Child = NewObject<USkeletalMeshComponent>(Owner, USkeletalMeshComponent::StaticClass(), NAME_None, RF_Transient);
        Child->SetupAttachment(SKMC);
        Child->RegisterComponent();
        Child->SetSkeletalMesh(Def.Get_Mesh());
        Child->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Child->SetLeaderPoseComponent(SKMC);
#if WITH_EDITOR
        // Leader-pose followers still gate their bone refresh on bUpdateAnimationInEditor.
        ACk_IskmRenderer_Actor_UE::EditorOnly_EnableAnimationTicking(Child);
#endif
        for (auto MatIdx = 0; MatIdx < Def.Get_OverrideMaterials().Num(); ++MatIdx)
        {
            if (auto* Mat = Def.Get_OverrideMaterials()[MatIdx].Get())
            {
                Child->SetMaterial(MatIdx, Mat);
            }
        }
        InCurrent._SubmeshSKMCs.Add(Child);
        InCurrent._AttachedSubmeshIndices.Add(Idx);
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_DetachSubmesh& InRequest) const -> void
    {
        auto* RendererData = ::UCk_Utils_IskmRenderer_UE::Get_RendererData(InParams.Get_Renderer());
        CK_ENSURE_IF_NOT(ck::IsValid(RendererData),
            TEXT("IskmProxy [{}]: RendererData missing in DetachSubmesh handler — Renderer handle [{}] no longer resolves"),
            InHandle, InParams.Get_Renderer())
        { return; }

        const auto Idx = RendererData->Find_SubmeshIndex_ByName(InRequest.Get_SubmeshName());
        CK_ENSURE_IF_NOT(Idx != INDEX_NONE,
            TEXT("IskmProxy [{}]: DetachSubmesh requested submesh named [{}] but no such entry exists in RendererData [{}]._Submeshes"),
            InHandle, InRequest.Get_SubmeshName(), GetNameSafe(RendererData))
        { return; }

        // Intentional silent return: detaching an unattached submesh is a no-op.
        const auto Slot = InCurrent._AttachedSubmeshIndices.IndexOfByKey(Idx);
        if (Slot == INDEX_NONE)
        { return; }
        // Tripwire on the parallel-array invariant maintained by this file's Add/RemoveAt/Reset pairs.
        CK_ENSURE_IF_NOT(InCurrent._SubmeshSKMCs.IsValidIndex(Slot),
            TEXT("IskmProxy [{}]: _AttachedSubmeshIndices/_SubmeshSKMCs desynced (slot [{}] vs [{}] SKMCs)"),
            InHandle, Slot, InCurrent._SubmeshSKMCs.Num())
        { return; }
        if (auto* Child = InCurrent._SubmeshSKMCs[Slot].Get())
        {
            Child->DestroyComponent();
        }
        InCurrent._SubmeshSKMCs.RemoveAt(Slot);
        InCurrent._AttachedSubmeshIndices.RemoveAt(Slot);
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& /*InHandle*/,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_DetachAllSubmeshes& /*InRequest*/) const -> void
    {
        for (auto& Weak : InCurrent._SubmeshSKMCs)
        {
            if (auto* C = Weak.Get())
            { C->DestroyComponent(); }
        }
        InCurrent._SubmeshSKMCs.Reset();
        InCurrent._AttachedSubmeshIndices.Reset();
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_SetAnimInstanceClass& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetAnimInstanceClass handler"),
            InHandle)
        { return; }

        // A null class is the documented sentinel for "drop to sequence mode", not a caller error.
        const auto IsAnimBpMode = ck::IsValid(InRequest.Get_AnimInstanceClass());
        const auto ClassToApply = IsAnimBpMode
            ? InRequest.Get_AnimInstanceClass()
            : TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()};

        ck_iskmproxy_processor::DoApply_AnimInstanceClass(SKMC, ClassToApply, FCk_Handle_IskmProxy{InHandle});
        InPoseSource._PoseSource = IsAnimBpMode
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;

        // The class switch invalidated any sequence playing through the previous single-node instance;
        // clearing both keeps EmitFinishedEvents from firing a spurious Completed event.
        InAnimState._CurrentSequence.Reset();
        InAnimState._LastFinishedDispatched = true;
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_PlayMontage& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in PlayMontage handler"),
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Montage()),
            TEXT("IskmProxy [{}]: PlayMontage request has a null Montage. Caller must supply a valid UAnimMontage"),
            InHandle)
        { return; }

        // Montage playback requires an AnimInstance with a slot; the notify-bridging subclass keeps
        // OnAnimationNotify / OnMontageFinished firing from this entity.
        if (ck::Is_NOT_Valid(SKMC->GetAnimInstance()))
        {
            ck_iskmproxy_processor::DoApply_AnimInstanceClass(
                SKMC,
                TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()},
                FCk_Handle_IskmProxy{InHandle});
        }
        auto* AI = SKMC->GetAnimInstance();
        CK_ENSURE_IF_NOT(ck::IsValid(AI),
            TEXT("IskmProxy [{}]: DoApply_AnimInstanceClass failed to set an AnimInstance on the SKMC for PlayMontage"),
            InHandle)
        { return; }

        // Montage_Play returns 0 on failure. Bail BEFORE mutating state, or the entity is permanently
        // marked montage-active with nothing playing and OnMontageFinished never fires.
        const auto MontageLength = AI->Montage_Play(InRequest.Get_Montage(), InRequest.Get_PlayRate());
        CK_ENSURE_IF_NOT(MontageLength > 0.0f,
            TEXT("IskmProxy [{}]: Montage_Play failed for Montage [{}] — montage/skeleton/slot mismatch with the current SkeletalMesh"),
            InHandle, GetNameSafe(InRequest.Get_Montage()))
        { return; }
        if (InRequest.Get_StartSection() != NAME_None)
        {
            AI->Montage_JumpToSection(InRequest.Get_StartSection(), InRequest.Get_Montage());
        }
        InAnimState._CurrentMontage = InRequest.Get_Montage();
        // Re-triggering while a montage is already active is normal flow, so the add must be guarded
        // against the "tag already exists" ensure.
        if (NOT InHandle.Has<FTag_IskmProxy_HasActiveMontage>())
        {
            InHandle.Add<FTag_IskmProxy_HasActiveMontage>();
        }
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_StopMontage& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in StopMontage handler"),
            InHandle)
        { return; }

        auto* AI = SKMC->GetAnimInstance();
        CK_ENSURE_IF_NOT(ck::IsValid(AI),
            TEXT("IskmProxy [{}]: SKMC has no AnimInstance in StopMontage handler — Setup did not apply UCk_IskmNotify_AnimInstance"),
            InHandle)
        { return; }

        if (auto* Montage = InAnimState._CurrentMontage.Get())
        {
            AI->Montage_Stop(InRequest.Get_BlendOutTime(), Montage);
        }

        // Guarded symmetrically with the PlayMontage add: a redundant StopMontage is normal flow and must
        // not fire the registry's absent-tag ensure.
        InAnimState._CurrentMontage.Reset();
        if (InHandle.Has<FTag_IskmProxy_HasActiveMontage>())
        {
            InHandle.Remove<FTag_IskmProxy_HasActiveMontage>();
        }
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& /*InTransform*/,
            const FCk_Request_IskmProxy_BeginRagdoll& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in BeginRagdoll handler"),
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(SKMC->GetPhysicsAsset()),
            TEXT("IskmProxy [{}]: BeginRagdoll requested but the SkeletalMesh has no PhysicsAsset bound. Set _DefaultMesh on AnimCollection to a mesh with a PhysicsAsset, or set PhysicsAsset on the mesh asset"),
            InHandle)
        { return; }

        SKMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        SKMC->SetAllBodiesSimulatePhysics(true);
        SKMC->SetSimulatePhysics(true);
        SKMC->WakeAllRigidBodies();

        if (NOT InRequest.Get_Impulse().IsNearlyZero())
        {
            constexpr auto VelChange = false;
            SKMC->AddImpulse(InRequest.Get_Impulse(), InRequest.Get_ImpulseBoneName(), VelChange);
        }
        InPoseSource._PoseSource = ECk_IskmProxy_PoseSource::Ragdoll;
        // Re-triggering while already ragdolling is normal flow (overlapping death impulses): the setters
        // above are idempotent and a repeat impulse is meaningful, so guard only the tag add.
        if (NOT InHandle.Has<FTag_IskmProxy_Ragdolling>())
        {
            InHandle.Add<FTag_IskmProxy_Ragdolling>();
        }

        // Ragdoll halts SKMC anim playback, so without this EmitFinishedEvents sees IsPlaying() == false
        // next tick and fires a spurious OnAnimationFinished(Completed).
        InAnimState._CurrentSequence.Reset();
        InAnimState._LastFinishedDispatched = true;
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FFragment_Transform& InTransform,
            const FCk_Request_IskmProxy_EndRagdoll& /*InRequest*/) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in EndRagdoll handler"),
            InHandle)
        { return; }

        // Rejected BEFORE any physics mutation: end-without-begin would silently disable collision and
        // rewrite _PoseSource on a proxy that was never ragdolling.
        CK_ENSURE_IF_NOT(InHandle.Has<FTag_IskmProxy_Ragdolling>(),
            TEXT("IskmProxy [{}]: EndRagdoll without an active ragdoll"),
            InHandle)
        { return; }

        SKMC->SetSimulatePhysics(false);
        SKMC->SetAllBodiesSimulatePhysics(false);
        // Stopping simulation does NOT clear the per-body blend weights: left at 1.0 the frozen simulated
        // pose keeps beating the resumed animation, and the mesh lies flat forever while sockets/anim
        // report standing. The RefreshBoneTransforms below forces the get-up to be visible this frame.
        SKMC->SetAllBodiesPhysicsBlendWeight(0.0f);
        SKMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        // The SKMC is stranded at the fallen root-body pose, and UpdateTransform only re-anchors on the
        // next entity-transform CHANGE — which never comes for an NPC that recovers standing still. The
        // fragment mirror is a tick stale here (handlers run BEFORE Transform_SyncFrom), so an
        // actor-backed owner must be read from its live root component instead.
        auto NewTransform = InHandle.Has<FFragment_Transform_RootComponent>()
            ? ::UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle)
            : InTransform.Get_Transform();
        NewTransform.AddToTranslation(NewTransform.GetRotation().RotateVector(InCurrent.Get_LocalLocationOffset()));
        SKMC->SetWorldTransform(NewTransform);

        SKMC->RefreshBoneTransforms();
        InPoseSource._PoseSource = ck::IsValid(SKMC->GetAnimInstance())
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;
        InHandle.Remove<FTag_IskmProxy_Ragdolling>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IskmProxy_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmProxy_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_SocketFollower_SyncTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_SocketFollower_SyncDescendants);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EmitFinishedEvents);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EndPlay);
