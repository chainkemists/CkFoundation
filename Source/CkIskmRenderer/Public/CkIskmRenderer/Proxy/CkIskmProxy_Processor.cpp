#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"
#include "CkIskmRenderer/CkIskmSubsystem.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"
#include "CkIskmRenderer/Notify/CkIskmNotify_AnimInstance.h"

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
        if (ck::Is_NOT_Valid(InSKMC)) { return; }
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
            FFragment_IskmProxy_CustomData& InCustomData) const -> void
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
        SKMC->SetWorldTransform(InParams.Get_SpawnTransform());

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
            if (NOT Def.Get_AttachByDefault()) { continue; }

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
                if (NOT InCustomData._Values.IsValidIndex(SlotIdx)) { continue; }
                InCustomData._Values[SlotIdx] = FloatArray[Offset];
                SKMC->SetCustomPrimitiveDataFloat(SlotIdx, FloatArray[Offset]);
            }
        }

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
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        if (ck::Is_NOT_Valid(SKMC)) { return; }

        // Phase E only registers this processor; transform sourcing is wired up in
        // a later phase (the proxy entity needs a Transform/SceneNode fragment first,
        // and CkEcsExt's UCk_Utils_Transform_UE::Get_EntityCurrentTransform takes a
        // FCk_Handle_Transform handle, not the proxy handle directly). Marking the
        // processor as a no-op for now — the FTag_IskmProxy_Movable + FTag_Transform_Updated
        // gate is correctly registered so when transform integration lands, this body
        // is the only thing that needs filling.
        // TODO(Phase L or transform-integration follow-up): read entity transform via
        // proxy → transform-handle conversion, apply via SetWorldTransform.
    }

    auto
        FProcessor_IskmProxy_EmitFinishedEvents::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState) const -> void
    {
        if (InAnimState._LastFinishedDispatched) { return; }
        auto* Cur = InAnimState._CurrentSequence.Get();
        if (ck::Is_NOT_Valid(Cur)) { return; }

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        if (ck::Is_NOT_Valid(SKMC)) { return; }

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
        if (ck::Is_NOT_Valid(SKMC)) { return; }

        for (auto& WeakChild : InCurrent._SubmeshSKMCs)
        {
            if (auto* Child = WeakChild.Get())
            {
                Child->DestroyComponent();
            }
        }
        InCurrent._SubmeshSKMCs.Reset();
        InCurrent._AttachedSubmeshIndices.Reset();

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
        if (ck::Is_NOT_Valid(SKMC)) { return; }
        if (ck::Is_NOT_Valid(InRequest.Get_Sequence())) { return; }

        if (InRequest.Get_bUnique() && InAnimState._CurrentSequence.Get() == InRequest.Get_Sequence())
        {
            return;
        }

        SKMC->SetAnimInstanceClass(nullptr);
        SKMC->PlayAnimation(InRequest.Get_Sequence(), InRequest.Get_bLoop());
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
        if (ck::Is_NOT_Valid(SKMC)) { return; }
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
            HandleType& /*InHandle*/,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FCk_Request_IskmProxy_SetPlayRate& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        if (ck::Is_NOT_Valid(SKMC)) { return; }
        SKMC->SetPlayRate(InRequest.Get_Rate());
    }

    auto
        FProcessor_IskmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& /*InHandle*/,
            const FFragment_IskmProxy_Params& /*InParams*/,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& InCustomData,
            const FCk_Request_IskmProxy_SetCustomDataFloat& InRequest) const -> void
    {
        if (NOT InCustomData._Values.IsValidIndex(InRequest.Get_Offset())) { return; }
        InCustomData._Values[InRequest.Get_Offset()] = InRequest.Get_Value();
        if (auto* SKMC = InCurrent.Get_BaseSKMC().Get())
        {
            SKMC->SetCustomPrimitiveDataFloat(InRequest.Get_Offset(), InRequest.Get_Value());
            for (auto& WeakChild : InCurrent._SubmeshSKMCs)
            {
                if (auto* Child = WeakChild.Get())
                {
                    Child->SetCustomPrimitiveDataFloat(InRequest.Get_Offset(), InRequest.Get_Value());
                }
            }
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
        if (ck::Is_NOT_Valid(RendererData)) { return; }

        const auto Idx = RendererData->Find_SubmeshIndex_ByName(InRequest.Get_SubmeshName());
        if (Idx == INDEX_NONE) { return; }
        if (InCurrent._AttachedSubmeshIndices.Contains(Idx)) { return; }

        // B4: enforce GPU custom-data bitmask cap (Plan-2 packs mesh presence in 4 bits = 15
        // slots). Plan-1 game code that exceeds this would silently break under Plan-2.
        CK_ENSURE_IF_NOT(InCurrent._AttachedSubmeshIndices.Num() < RendererData->Get_MaxSubmeshPerInstance(),
            TEXT("IskmProxy [{}]: cannot attach submesh [{}] — already at MaxSubmeshPerInstance ({})"),
            InHandle, InRequest.Get_SubmeshName(), RendererData->Get_MaxSubmeshPerInstance())
        { return; }

        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        if (ck::Is_NOT_Valid(SKMC)) { return; }
        auto* Owner = Cast<ACk_IskmRenderer_Actor_UE>(SKMC->GetOwner());
        if (ck::Is_NOT_Valid(Owner)) { return; }

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
            HandleType& /*InHandle*/,
            const FFragment_IskmProxy_Params& InParams,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& /*InPoseSource*/,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FCk_Request_IskmProxy_DetachSubmesh& InRequest) const -> void
    {
        auto* RendererData = ::UCk_Utils_IskmRenderer_UE::Get_RendererData(InParams.Get_Renderer());
        if (ck::Is_NOT_Valid(RendererData)) { return; }
        const auto Idx = RendererData->Find_SubmeshIndex_ByName(InRequest.Get_SubmeshName());
        if (Idx == INDEX_NONE) { return; }

        const auto Slot = InCurrent._AttachedSubmeshIndices.IndexOfByKey(Idx);
        if (Slot == INDEX_NONE) { return; }

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
            if (auto* C = Weak.Get()) { C->DestroyComponent(); }
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
            FFragment_IskmProxy_AnimState& /*InAnimState*/,
            FFragment_IskmProxy_PoseSource& InPoseSource,
            FFragment_IskmProxy_CustomData& /*InCustomData*/,
            const FCk_Request_IskmProxy_SetAnimInstanceClass& InRequest) const -> void
    {
        auto* SKMC = InCurrent.Get_BaseSKMC().Get();
        if (ck::Is_NOT_Valid(SKMC)) { return; }

        // nullptr request falls back to UCk_IskmNotify_AnimInstance so OnAnimationNotify
        // and OnMontageFinished still fire while in sequence mode.
        const auto IsAnimBpMode = ck::IsValid(InRequest.Get_AnimInstanceClass());
        const auto ClassToApply = IsAnimBpMode
            ? InRequest.Get_AnimInstanceClass()
            : TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()};

        DoApply_AnimInstanceClass(SKMC, ClassToApply, FCk_Handle_IskmProxy{InHandle});
        InPoseSource._PoseSource = IsAnimBpMode
            ? ECk_IskmProxy_PoseSource::AnimBP
            : ECk_IskmProxy_PoseSource::Sequence;
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
        if (ck::Is_NOT_Valid(SKMC)) { return; }
        if (ck::Is_NOT_Valid(InRequest.Get_Montage())) { return; }

        // Lazy ensure: an AnimInstance must exist for montage playback. Use the
        // notify-bridging subclass (NOT plain UAnimInstance) so OnAnimationNotify and
        // OnMontageFinished signals still fire from this entity.
        if (ck::Is_NOT_Valid(SKMC->GetAnimInstance()))
        {
            DoApply_AnimInstanceClass(
                SKMC,
                TSubclassOf<UAnimInstance>{::UCk_IskmNotify_AnimInstance::StaticClass()},
                FCk_Handle_IskmProxy{InHandle});
        }
        auto* AI = SKMC->GetAnimInstance();
        if (ck::Is_NOT_Valid(AI)) { return; }

        AI->Montage_Play(InRequest.Get_Montage(), InRequest.Get_PlayRate());
        if (InRequest.Get_StartSection() != NAME_None)
        {
            AI->Montage_JumpToSection(InRequest.Get_StartSection(), InRequest.Get_Montage());
        }
        InAnimState._CurrentMontage = InRequest.Get_Montage();
        InHandle.Add<FTag_IskmProxy_HasActiveMontage>();
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
        if (ck::Is_NOT_Valid(SKMC)) { return; }
        auto* AI = SKMC->GetAnimInstance();
        if (ck::Is_NOT_Valid(AI)) { return; }

        if (auto* Montage = InAnimState._CurrentMontage.Get())
        {
            AI->Montage_Stop(InRequest.Get_BlendOutTime(), Montage);
        }

        // Clear the active-montage tag and current-montage ref so future processors
        // filtering on FTag_IskmProxy_HasActiveMontage see correct state.
        InAnimState._CurrentMontage.Reset();
        InHandle.Remove<FTag_IskmProxy_HasActiveMontage>();
    }

    auto FProcessor_IskmProxy_HandleRequests::DoHandleRequest(
        HandleType&, const FFragment_IskmProxy_Params&, FFragment_IskmProxy_Current&,
        FFragment_IskmProxy_AnimState&, FFragment_IskmProxy_PoseSource&,
        FFragment_IskmProxy_CustomData&,
        const FCk_Request_IskmProxy_BeginRagdoll&) const -> void {}
}

// Inline processor registration — same pattern as CkIsmRenderer_Processor.cpp and the
// renderer-side CkIskmRenderer_Processor.cpp at the end of D4. The 5 registrations land
// at file scope, outside `namespace ck`.
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EmitFinishedEvents);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EndPlay);
