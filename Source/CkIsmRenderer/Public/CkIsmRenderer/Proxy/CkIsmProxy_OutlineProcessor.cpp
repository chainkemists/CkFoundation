#include "CkIsmProxy_OutlineProcessor.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIsmRenderer/CkIsmRenderer_Log.h"
#include "CkIsmRenderer/CkIsmSubsystem.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_Outline_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_Outline_TransformSync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_Outline_Suspend);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_Outline_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_Outline_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ism_outline_processor
{
    using ck_ism_proxy::Get_TransformWithLocalOffset;

    // Safe during world/subsystem teardown — both lookups degrade to no-ops.
    auto
        DoTeardownAppliedOutline(
            const FCk_Handle& InHandle,
            const ck::FFragment_IsmProxy_OutlineApplied& InApplied)
        -> void
    {
        if (auto* ShadowIsm = InApplied.Get_ShadowIsm().Get();
            ck::IsValid(ShadowIsm) && ShadowIsm->IsValidId(InApplied.Get_ShadowInstanceId()))
        {
            ShadowIsm->RemoveInstanceById(InApplied.Get_ShadowInstanceId());
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        // An expired preset must not release: an invalid weak ptr compares equal to every other
        // invalid weak ptr, so a nullptr Find inside Release_StencilFor can match an unrelated
        // expired entry (same guard as UCkUsf_OutlineSubsystem::Remove_Outline_From_Component).
        if (auto* OutlineSubsystem = World->GetSubsystem<UCkUsf_OutlineSubsystem>();
            ck::IsValid(OutlineSubsystem) && InApplied.Get_Preset().IsValid())
        {
            OutlineSubsystem->Release_StencilFor(InApplied.Get_Preset().Get());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IsmProxy_Outline_Sync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IsmProxy_Outline_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_OutlineTarget& InTarget,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        using namespace ck_ism_outline_processor;

        auto* Preset = InTarget.Get_Preset().Get();

        if (ck::Is_NOT_Valid(Preset))
        { return; }

        if (InHandle.Has<FFragment_IsmProxy_OutlineApplied>())
        {
            const auto& Applied = InHandle.Get<FFragment_IsmProxy_OutlineApplied>();

            const auto& StillValid = Applied.Get_Preset() == InTarget.Get_Preset() &&
                ck::IsValid(Applied.Get_ShadowIsm().Get()) &&
                Applied.Get_ShadowIsm()->IsValidId(Applied.Get_ShadowInstanceId());

            if (StillValid)
            { return; }

            DoTeardownAppliedOutline(InHandle, Applied);
            InHandle.Remove<FFragment_IsmProxy_OutlineApplied>();
        }

        if (ck::Is_NOT_Valid(_World.Get()))
        { return; }

        auto* OutlineSubsystem = _World->GetSubsystem<UCkUsf_OutlineSubsystem>();
        auto* IsmSubsystem = _World->GetSubsystem<UCk_IsmRenderer_Subsystem_UE>();

        if (ck::Is_NOT_Valid(OutlineSubsystem) ||
            ck::Is_NOT_Valid(IsmSubsystem))
        { return; }

        const auto Stencil = OutlineSubsystem->Get_OrAllocate_StencilFor(Preset);
        if (Stencil == 0)
        { return; } // stencil range exhausted — already warned by the subsystem

        const auto ShadowIsm = IsmSubsystem->FindOrCreate_OutlineIsmComponent(
            InParams.Get_IsmRenderer().Get(), Preset, Stencil
#if WITH_EDITOR
            , UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionOwnerWeak(InHandle)
#endif
            );

        if (ck::Is_NOT_Valid(ShadowIsm))
        {
            OutlineSubsystem->Release_StencilFor(Preset);
            return;
        }

        constexpr auto TransformAsWorldSpace = true;
        const auto& InstanceTransform = Get_TransformWithLocalOffset(InParams, InTransform.Get_Transform());
        const auto& ShadowInstanceId = ShadowIsm->AddInstanceById(InstanceTransform, TransformAsWorldSpace);

        if (const auto& CustomData = InCurrent.Get_CustomInstanceDataValues();
            NOT CustomData.IsEmpty() && ShadowIsm->NumCustomDataFloats == CustomData.Num())
        { ShadowIsm->SetCustomDataById(ShadowInstanceId, CustomData); }

        InHandle.AddOrGet<FFragment_IsmProxy_OutlineApplied>() =
            FFragment_IsmProxy_OutlineApplied{Preset, ShadowIsm.Get(), ShadowInstanceId};

        ck::ismrenderer::VeryVerbose(TEXT("Applied outline (shadow instance) for ISM Proxy [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Outline_TransformSync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _ShadowIsms.Reset();
        TProcessor::DoTick(InDeltaT);

        for (const auto Ism : _ShadowIsms)
        {
            Ism->MarkRenderStateDirty();
        }
    }

    auto
        FProcessor_IsmProxy_Outline_TransformSync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_OutlineApplied& InApplied,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform)
        -> void
    {
        using namespace ck_ism_outline_processor;

        auto* ShadowIsm = InApplied.Get_ShadowIsm().Get();

        if (ck::Is_NOT_Valid(ShadowIsm) || NOT ShadowIsm->IsValidId(InApplied.Get_ShadowInstanceId()))
        { return; }

        _ShadowIsms.Add(ShadowIsm);

        constexpr auto TransformAsWorldSpace = true;
        const auto& NewInstanceTransform = Get_TransformWithLocalOffset(InParams, InTransform.Get_Transform());

        ShadowIsm->UpdateInstanceTransformById(InApplied.Get_ShadowInstanceId(), NewInstanceTransform, TransformAsWorldSpace);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Outline_Suspend::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_OutlineApplied& InApplied)
        -> void
    {
        ck_ism_outline_processor::DoTeardownAppliedOutline(InHandle, InApplied);
        InHandle.Remove<FFragment_IsmProxy_OutlineApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Outline_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_OutlineApplied& InApplied)
        -> void
    {
        ck_ism_outline_processor::DoTeardownAppliedOutline(InHandle, InApplied);
        InHandle.Remove<FFragment_IsmProxy_OutlineApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Outline_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_OutlineApplied& InApplied)
        -> void
    {
        ck_ism_outline_processor::DoTeardownAppliedOutline(InHandle, InApplied);
        InHandle.Remove<FFragment_IsmProxy_OutlineApplied>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
