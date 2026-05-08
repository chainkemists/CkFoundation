#include "CkIskmRenderer/Proxy/CkIskmProxy_Processor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"
#include "CkIskmRenderer/CkIskmSubsystem.h"
#include "CkIskmRenderer/CkIskmRenderer_Log.h"

namespace ck
{
    // B7: refresh the world pointer once per tick. ForEachEntity then reads `_World`
    // directly instead of paying the per-entity lookup cost.
    auto
        FProcessor_IskmProxy_Setup::
        DoTick(TimeType /*InDeltaT*/) -> void
    {
        // The transient context exposes the registry's owning world; pull it the same way
        // CkEcsExt processors that need a world ref do.
        if (auto* Registry = TryGet_Registry())
        {
            _World = Registry->ctx().find<TWeakObjectPtr<UWorld>>();
        }
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

        auto* RendererData = UCk_Utils_IskmRenderer_UE::Get_RendererData(RendererHandle);
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

        // Default pose source: Sequence. The AnimInstance class assignment + notify-bridge
        // wiring is added by Phase M (which introduces UCk_IskmNotify_AnimInstance and the
        // DoApply_AnimInstanceClass helper). At Phase E3 we leave the SKMC without an
        // AnimInstance class — single-node playback works without one.
        InPoseSource._PoseSource = ECk_IskmProxy_PoseSource::Sequence;

        InCurrent._BaseSKMC = SKMC;

        const auto NumCustom = RendererData->Get_NumCustomDataFloat();
        InCustomData._Values.Init(0.0f, NumCustom);
        SKMC->SetNumCustomDataFloats(NumCustom);
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
        for (const auto& Default : InParams.Get_CustomInstanceDataDefaults())
        {
            const auto SlotIdx = Default.Get_DataIndex();
            if (NOT InCustomData._Values.IsValidIndex(SlotIdx)) { continue; }
            InCustomData._Values[SlotIdx] = Default.Get_Value();
            SKMC->SetCustomPrimitiveDataFloat(SlotIdx, Default.Get_Value());
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

        // The processor's tag gate (FTag_IskmProxy_Movable + FTag_Transform_Updated) means
        // we only get here when something legitimately needs syncing. Apply unconditionally.
        const auto NewTransform = UCk_Utils_EntityLifetime_UE::Get_EntityTransform(InHandle);
        SKMC->SetWorldTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }

    auto
        FProcessor_IskmProxy_EmitFinishedEvents::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_IskmProxy_Current& InCurrent,
            FFragment_IskmProxy_AnimState& InAnimState) const -> void
    {
        // Filled in during Phase F (via the OnAnimationFinished signal forwarder)
        // and Phase J (montage finish) and Phase M (notify forwarding).
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
}

// Inline processor registration — same pattern as CkIsmRenderer_Processor.cpp and the
// renderer-side CkIskmRenderer_Processor.cpp at the end of D4. The 5 registrations land
// at file scope, outside `namespace ck`.
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EmitFinishedEvents);
CK_REGISTER_PROCESSOR(ck::FProcessor_IskmProxy_EndPlay);
