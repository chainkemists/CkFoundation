#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Utils.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"
#include "CkIskmRenderer/CkIskmSubsystem.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"
#include "CkIskmRenderer/CkIskmRenderer_Stats.h"
#include "CkIskmRenderer/Notify/CkIskmNotify_AnimInstance.h"

// Phase K: ck::Is_NOT_Valid on UPhysicsAsset* needs the full class definition
// for the validation trait's __is_base_of intrinsic.
#include "PhysicsEngine/PhysicsAsset.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Iskm::UpdateTransform"),    STAT_CkIskm_UpdateTransform,    STATGROUP_CkIskmRenderer);
DECLARE_CYCLE_STAT(TEXT("Iskm::SocketFollowerSync"), STAT_CkIskm_SocketFollowerSync, STATGROUP_CkIskmRenderer);
DECLARE_CYCLE_STAT(TEXT("Iskm::EmitFinishedEvents"), STAT_CkIskm_EmitFinishedEvents, STATGROUP_CkIskmRenderer);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Sets an AnimInstance class on the SKMC and re-wires the notify-forwarder owning
    // handle on the resulting AnimInstance. Called from Setup (this phase), Phase I
    // (SetAnimInstanceClass handler), and Phase J's lazy-AnimInstance branch in
    // PlayMontage. Per Source/CLAUDE.md: no anonymous namespaces — use `static`.
    static auto DoApply_AnimInstanceClass(
        USkeletalMeshComponent* InSKMC,
        TSubclassOf<UAnimInstance> InClass,
        FCk_Handle_IskmProxy InOwningHandle) -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InSKMC),
            TEXT("DoApply_AnimInstanceClass called with null SKMC for proxy [{}]"),
            InOwningHandle)
        { return; }
        InSKMC->SetAnimInstanceClass(InClass);

        // ::-qualified — the friend-class declarations in Fragment.h inject forward
        // decls into ck::, so unqualified lookup inside `namespace ck { }` resolves
        // to the incomplete forward decl instead of the file-scope UCLASS. This only
        // bites when the .cpp falls out of unity-build aggregation (Adaptive Build).
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

    // B7: refresh the world pointer once per tick. ForEachEntity then reads `_World`
    // directly instead of paying the per-entity lookup cost. Sibling pattern from
    // CkIsmProxy_Processor.cpp lines 100-130.
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

        // B7: cached world pointer; resolved once per tick in DoTick(). Asserts cheaply.
        auto* World = _World.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("IskmProxy Setup: cached world is invalid for [{}]"), InHandle)
        { return; }

        auto& RendererCurrent = RendererHandle.Get<FFragment_IskmRenderer_Current>();
        auto* RendererActor = RendererCurrent.Get_RendererActor().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(RendererActor),
            TEXT("IskmProxy Setup: renderer actor missing for [{}]"), InHandle)
        { return; }

        auto* SKMC = RendererActor->Acquire_BaseSKMC();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy Setup: failed to acquire SKMC for [{}]"), InHandle)
        { return; }

        SKMC->SetSkeletalMesh(AnimCollection->Get_DefaultMesh());

        // Cache the per-instance render offset and compose it into the initial pose (entity-space ->
        // world via the spawn rotation). UpdateTransform re-applies it every frame for movable proxies.
        InCurrent._LocalLocationOffset = InParams.Get_LocalLocationOffset();
        auto SpawnXf = InParams.Get_SpawnTransform();
        SpawnXf.AddToTranslation(SpawnXf.GetRotation().RotateVector(InCurrent._LocalLocationOffset));
        SKMC->SetWorldTransform(SpawnXf);

        // Resolve the AnimBP class. Sync-load — see Claude.md note about hitch on first
        // use of an AnimCollection. Fall back to the notify-bridging UAnimInstance subclass
        // so OnAnimationNotify and OnMontageFinished still fire in sequence mode.
        const auto SoftClass = RendererData->Get_DefaultAnimInstanceClass();
        auto* AnimClass = SoftClass.IsNull() ? nullptr : SoftClass.LoadSynchronous();
        const auto IsAnimBpMode = ck::IsValid(AnimClass);
        const auto ClassToApply = IsAnimBpMode
            ? TSubclassOf<UAnimInstance>{AnimClass}
            : TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()};

        DoApply_AnimInstanceClass(SKMC, ClassToApply, FCk_Handle_IskmProxy{InHandle});
        InPoseSource._PoseSource = IsAnimBpMode
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;

        InCurrent._BaseSKMC = SKMC;

        const auto NumCustom = RendererData->Get_NumCustomDataFloat();
        InCustomData._Values.Init(0.0f, NumCustom);
        // USkeletalMeshComponent doesn't have SetNumCustomDataFloats (that's on
        // UInstancedStaticMeshComponent for ISM batching). For non-instanced
        // components we just write per-slot via SetCustomPrimitiveDataFloat;
        // the underlying CustomPrimitiveData array grows automatically.
        for (auto Idx = 0; Idx < NumCustom; ++Idx)
        {
            SKMC->SetCustomPrimitiveDataFloat(Idx, 0.0f);
        }

        for (auto Idx = 0; Idx < RendererData->Get_Submeshes().Num(); ++Idx)
        {
            const auto& Def = RendererData->Get_Submeshes()[Idx];
            if (NOT Def.Get_AttachByDefault())
            { continue; }
            auto* Child = NewObject<USkeletalMeshComponent>(RendererActor, USkeletalMeshComponent::StaticClass(), NAME_None, RF_Transient);
            Child->SetupAttachment(SKMC);
            Child->RegisterComponent();
            Child->SetSkeletalMesh(Def.Get_Mesh());
            Child->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Child->SetLeaderPoseComponent(SKMC);
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

        // B3: seed per-instance custom data from ParamsData defaults.
        // FCk_CustomPrimitiveData uses _CustomDataIndex (not _DataIndex) and a
        // tagged-union FCk_CustomPrimitiveData_Value (Float/Vec2/Vec3/Vec4/Color).
        // Sibling CkIsmProxy_Setup converts via Value.ConvertToFloatArray() and
        // writes consecutive slots. We mirror that exact pattern.
        for (const auto& Override : InParams.Get_CustomInstanceDataDefaults())
        {
            const auto& StartIdx = Override.Get_CustomDataIndex();
            const auto& FloatArray = Override.Get_Value().ConvertToFloatArray();
            for (auto Offset = 0; Offset < FloatArray.Num(); ++Offset)
            {
                const auto SlotIdx = StartIdx + Offset;
                if (NOT InCustomData._Values.IsValidIndex(SlotIdx))
                { continue; }
                InCustomData._Values[SlotIdx] = FloatArray[Offset];
                SKMC->SetCustomPrimitiveDataFloat(SlotIdx, FloatArray[Offset]);
            }
        }

        // Re-apply stored material overrides. Empty on first Setup (requests only
        // drain post-Setup) — load-bearing if the SKMC is ever (re)acquired after
        // overrides were recorded on this entity.
        for (const auto& Kvp : InMaterialOverrides._SlotToMaterial)
        {
            SKMC->SetMaterial(Kvp.Key, Kvp.Value.Get());
        }
        InMaterialOverrides._Dirty = false;

        // Re-apply stored morph targets — same rationale as the material
        // overrides above.
        for (const auto& Kvp : InMorphTargets._Values)
        {
            SKMC->SetMorphTarget(Kvp.Key, Kvp.Value);
        }
        InMorphTargets._Dirty = false;

        // A3: tag the entity as movable if requested. Static proxies (no tag) are skipped
        // by FProcessor_IskmProxy_UpdateTransform every frame.
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
            FFragment_IskmProxy_Requests& InRequests) const -> void
    {
        // Canonical visitor pattern from CkIsmProxy_Processor.cpp:396-411 — single generic
        // lambda dispatching to the overloaded DoHandleRequest member functions per request
        // type. New request types only need a new DoHandleRequest overload (decl in the
        // header, def below) — the visitor body never changes.
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InAnimState, InPoseSource, InCustomData, InRequest);
            }), ck::policy::DontResetContainer{});

        InRequests._Requests.Reset();
    }

    auto
        FProcessor_IskmProxy_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent) const -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkIskm_UpdateTransform);

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in UpdateTransform processor"),
            InHandle)
        { return; }

        // Phase L: read the entity's current transform via the type-unsafe variant
        // (does the FCk_Handle → FCk_Handle_Transform cast internally) and push it
        // onto the SKMC. Gated by FTag_IskmProxy_Movable + FTag_Transform_Updated
        // (CkEcsExt's transform system sets the latter only when the transform
        // changed) and TExclude<FTag_IskmProxy_Ragdolling> (physics drives ragdolls).
        auto NewTransform = ::UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        // Compose the cached per-instance render offset (entity-space) into the world transform so the
        // body renders off the entity origin (e.g. drop a Character-actor proxy by the capsule half-height).
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

        if (ck::Is_NOT_Valid(Leader))
        { return; }

        // Component-space socket = pure animation pose (the SKMC's world placement
        // is deliberately NOT consulted — it is one frame stale by construction);
        // the root term comes from the leader's live entity transform instead.
        const auto SocketComponentSpace = ::UCk_Utils_IskmProxy_UE::Get_SocketTransform(
            Leader, InFollower.Get_Socket(), ECk_IskmProxy_TransformSpace::Component);

        auto LeaderTransform = ::UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Leader);
        // Compose the leader's render offset into the root so followers track the *offset* body
        // rather than the entity origin — must match FProcessor_IskmProxy_UpdateTransform's SKMC
        // placement (line ~258), or cosmetics float by the offset (e.g. a Character's capsule half-height).
        const auto LeaderOffset = Leader.Get<FFragment_IskmProxy_Current>().Get_LocalLocationOffset();
        LeaderTransform.AddToTranslation(LeaderTransform.GetRotation().RotateVector(LeaderOffset));

        const auto NewTransform = InFollower.Get_Offset() * SocketComponentSpace * LeaderTransform;

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
        // Intentional silent return: no current sequence means there's no
        // Completed event to fire. This is the normal "nothing playing" state.
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
            // Reset _CurrentSequence so Get_PlayingAnimation reflects "not playing"
            // and a subsequent Request_StopAnimation doesn't fire a duplicate Stopped
            // event for an already-completed sequence.
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

        // Pool hygiene (load-bearing): OverrideMaterials is a component-level array
        // that survives Release_BaseSKMC's SetSkeletalMesh(nullptr) — without this
        // clear, per-proxy material overrides leak to the next proxy that borrows
        // this SKMC. V1 owns ALL override slots on the base SKMC, so a full clear
        // restores mesh-default materials exactly.
        SKMC->EmptyOverrideMaterials();

        // Same pool hygiene for morph curves: MorphTargetCurves survives
        // Release_BaseSKMC, so per-proxy morphs would leak to the next borrower.
        SKMC->ClearMorphTargets();

        if (auto* Owner = SKMC->GetOwner())
        {
            if (auto* RendererActor = Cast<ACk_IskmRenderer_Actor_UE>(Owner))
            {
                RendererActor->Release_BaseSKMC(SKMC);
            }
        }
        InCurrent._BaseSKMC.Reset();
    }

    // ---- DoHandleRequest stubs ----
    //
    // The HandleRequests visitor instantiates DoHandleRequest(...) for every
    // std::variant alternative in FFragment_IskmProxy_Requests::RequestType. At E3 we
    // declare 5 overloads (matching the variant) but only F/J/K provide real bodies.
    // To keep the linker happy from E3 onwards, we ship empty stubs here. Each later
    // phase replaces its stub with a real implementation:
    //   PlayAnimation / StopAnimation  → Phase F1
    //   PlayMontage / StopMontage      → Phase J1
    //   BeginRagdoll                   → Phase K1
    // Until then, the variant is empty in practice (Add doesn't enqueue anything yet,
    // and FFragment_IskmProxy_Requests has no API to push), so these are no-ops at
    // runtime — they exist purely so the visitor template instantiates cleanly.

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& InCustomData,
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

        // Intentional dedup: if the caller opted into "unique" semantics and the
        // same sequence is already active, skip restarting from frame 0.
        if (InRequest.Get_Unique() && InAnimState._CurrentSequence.Get() == InRequest.Get_Sequence())
        {
            return;
        }

        // Only clear the AnimInstance class if we're not already in single-node
        // mode. Calling SetAnimInstanceClass(nullptr) when AnimClass is already
        // null tears down the existing SingleNodeInstance before PlayAnimation
        // re-creates one; that momentary teardown leaves the render proxy in a
        // half-initialized state and the SKMC visibly snaps to ref pose.
        // Observed via TransitionCycle gym station: re-issuing PlayAnimation
        // from a timer reliably A-poses the proxy.
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

        // MaterialOverrides is added unconditionally in Add(...) but isn't threaded
        // through the processor signature — re-threading every DoHandleRequest
        // overload for two consumers isn't worth the churn.
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
            const FCk_Request_IskmProxy_ClearMaterialOverrides& /*InRequest*/) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in ClearMaterialOverrides handler"),
            InHandle)
        { return; }

        auto& Overrides = InHandle.Get<FFragment_IskmProxy_MaterialOverrides>();

        // Intentional silent return: clearing with no recorded overrides is a
        // no-op (e.g. a randomizer that always clears before re-rolling).
        if (Overrides._SlotToMaterial.IsEmpty())
        { return; }
        Overrides._SlotToMaterial.Reset();
        Overrides._Dirty = true;

        // V1 owns ALL override slots on the base SKMC, so a full component-level
        // clear restores mesh-default materials exactly.
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

        // MorphTargets is added unconditionally in Add(...) but isn't threaded
        // through the processor signature — same rationale as MaterialOverrides.
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

        // SetSkeletalMesh re-runs InitAnim -> a fresh AnimInstance of the preserved AnimClass.
        // Re-establish the notify bridge's owning-proxy handle on the new instance (mirrors
        // DoApply_AnimInstanceClass) so OnAnimationNotify / OnMontageFinished keep routing.
        if (auto* IskmAI = Cast<::UCk_IskmNotify_AnimInstance>(SKMC->GetAnimInstance()))
        {
            IskmAI->Set_OwningProxyHandle(FCk_Handle_IskmProxy{InHandle});
        }

        // The swap rebuilt the component's material slots / morph curves / custom data —
        // re-apply the proxy's recorded state (mirrors the Setup re-apply path).
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
            const FCk_Request_IskmProxy_AttachSubmesh& InRequest) const -> void
    {
        // ::-qualified — see Phase F note re friend-class forward decl injection.
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
        // B4: enforce GPU custom-data bitmask cap (Plan-2 packs mesh presence in 4 bits = 15
        // slots). Plan-1 game code that exceeds this would silently break under Plan-2.
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
        auto* Child = NewObject<USkeletalMeshComponent>(Owner, USkeletalMeshComponent::StaticClass(), NAME_None, RF_Transient);
        Child->SetupAttachment(SKMC);
        Child->RegisterComponent();
        Child->SetSkeletalMesh(Def.Get_Mesh());
        Child->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Child->SetLeaderPoseComponent(SKMC);
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

        // Intentional silent return: detaching a submesh that isn't currently
        // attached is a no-op (e.g. cycle-detach driven by a state machine that
        // doesn't track attach state).
        const auto Slot = InCurrent._AttachedSubmeshIndices.IndexOfByKey(Idx);
        if (Slot == INDEX_NONE)
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
            const FCk_Request_IskmProxy_SetAnimInstanceClass& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in SetAnimInstanceClass handler"),
            InHandle)
        { return; }

        // Intentional: null AnimInstanceClass is the documented sentinel for
        // "fall back to UCk_IskmNotify_AnimInstance (sequence mode)". Callers
        // pass NullClass explicitly to drop out of AnimBP mode.
        const auto IsAnimBpMode = ck::IsValid(InRequest.Get_AnimInstanceClass());
        const auto ClassToApply = IsAnimBpMode
            ? InRequest.Get_AnimInstanceClass()
            : TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()};

        DoApply_AnimInstanceClass(SKMC, ClassToApply, FCk_Handle_IskmProxy{InHandle});
        InPoseSource._PoseSource = IsAnimBpMode
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;

        // Switching AnimInstance class invalidates any sequence playing through
        // the previous single-node instance. Clear the tracked sequence + dispatch
        // flag so EmitFinishedEvents doesn't fire a spurious Completed event.
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

        // Lazy-create the bridging AnimInstance if the SKMC doesn't have one
        // yet — montage playback requires an AnimInstance with a slot. Use the
        // notify-bridging subclass so OnAnimationNotify / OnMontageFinished
        // signals still fire from this entity.
        if (ck::Is_NOT_Valid(SKMC->GetAnimInstance()))
        {
            DoApply_AnimInstanceClass(
                SKMC,
                TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()},
                FCk_Handle_IskmProxy{InHandle});
        }
        auto* AI = SKMC->GetAnimInstance();
        CK_ENSURE_IF_NOT(ck::IsValid(AI),
            TEXT("IskmProxy [{}]: DoApply_AnimInstanceClass failed to set an AnimInstance on the SKMC for PlayMontage"),
            InHandle)
        { return; }

        AI->Montage_Play(InRequest.Get_Montage(), InRequest.Get_PlayRate());
        if (InRequest.Get_StartSection() != NAME_None)
        {
            AI->Montage_JumpToSection(InRequest.Get_StartSection(), InRequest.Get_Montage());
        }
        InAnimState._CurrentMontage = InRequest.Get_Montage();
        // Re-triggering PlayMontage while one is already active is normal (e.g.
        // gym MontageBurst station fires every 3s). Guard the tag add against
        // the "tag already exists" ensure.
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

        // Clear the active-montage tag and current-montage ref so future processors
        // filtering on FTag_IskmProxy_HasActiveMontage see correct state.
        InAnimState._CurrentMontage.Reset();
        InHandle.Remove<FTag_IskmProxy_HasActiveMontage>();
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
        InHandle.Add<FTag_IskmProxy_Ragdolling>();

        // Switching to ragdoll halts SKMC anim playback. If a sequence was active,
        // EmitFinishedEvents would otherwise see IsPlaying() == false next tick and
        // fire a spurious OnAnimationFinished(Completed). Clear the tracked sequence
        // and mark the finish as already-dispatched.
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
            const FCk_Request_IskmProxy_EndRagdoll& /*InRequest*/) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
            TEXT("IskmProxy [{}]: BaseSKMC missing in EndRagdoll handler"),
            InHandle)
        { return; }
        SKMC->SetSimulatePhysics(false);
        SKMC->SetAllBodiesSimulatePhysics(false);
        SKMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        InPoseSource._PoseSource = ck::IsValid(SKMC->GetAnimInstance())
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;
        InHandle.Remove<FTag_IskmProxy_Ragdolling>();
    }
}

// Inline processor registration — same pattern as CkIsmRenderer_Processor.cpp and the
// renderer-side CkIskmRenderer_Processor.cpp at the end of D4. The 5 registrations land
// at file scope, outside `namespace ck`.
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_SocketFollower_SyncTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EmitFinishedEvents);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EndPlay);
