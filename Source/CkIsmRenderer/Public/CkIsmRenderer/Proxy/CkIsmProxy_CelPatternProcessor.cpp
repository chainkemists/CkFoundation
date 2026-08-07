#include "CkIsmProxy_CelPatternProcessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIsmRenderer/CkIsmRenderer_Log.h"
#include "CkIsmRenderer/CkIsmSubsystem.h"

#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_CelPattern_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_CelPattern_TransformSync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_CelPattern_Suspend);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_CelPattern_DropAppliedOnOutline);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_CelPattern_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_CelPattern_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ism_cel_pattern_processor
{
    using ck_ism_proxy::Get_TransformWithLocalOffset;

    // No stencil to hand back: the cel contract is a direct value rather than the outline's refcounted
    // allocation, so pulling the instance out of the shadow ISM is the whole teardown.
    auto
        DoTeardownAppliedCelPattern(
            const ck::FFragment_IsmProxy_CelPatternApplied& InApplied)
        -> void
    {
        if (auto* ShadowIsm = InApplied.Get_ShadowIsm().Get();
            ck::IsValid(ShadowIsm) && ShadowIsm->IsValidId(InApplied.Get_ShadowInstanceId()))
        {
            ShadowIsm->RemoveInstanceById(InApplied.Get_ShadowInstanceId());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IsmProxy_CelPattern_Sync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IsmProxy_CelPattern_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_CelPatternTarget& InTarget,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        using namespace ck_ism_cel_pattern_processor;

        if (ck::Is_NOT_Valid(_World.Get()))
        { return; }

        auto* CelShadeSubsystem = _World->GetSubsystem<UCkUsf_CelShadeSubsystem>();

        if (ck::Is_NOT_Valid(CelShadeSubsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        // 0 means the stencil contract is switched off in this world's settings — a shadow carrying it would
        // write the engine's "nothing here" value and read as a silent success.
        const auto StencilValue = CelShadeSubsystem->Get_StencilValueFor(InTarget.Get_Pattern());

        if (StencilValue == 0)
        { return; }

        if (InHandle.Has<FFragment_IsmProxy_CelPatternApplied>())
        {
            const auto& Applied = InHandle.Get<FFragment_IsmProxy_CelPatternApplied>();

            const auto& StillValid = Applied.Get_Pattern() == InTarget.Get_Pattern() &&
                Applied.Get_StencilValue() == StencilValue &&
                ck::IsValid(Applied.Get_ShadowIsm().Get()) &&
                Applied.Get_ShadowIsm()->IsValidId(Applied.Get_ShadowInstanceId());

            if (StillValid)
            { return; }

            DoTeardownAppliedCelPattern(Applied);
            InHandle.Remove<FFragment_IsmProxy_CelPatternApplied>();
        }

        auto* IsmSubsystem = _World->GetSubsystem<UCk_IsmRenderer_Subsystem_UE>();

        if (ck::Is_NOT_Valid(IsmSubsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto ShadowIsm = IsmSubsystem->FindOrCreate_CelPatternIsmComponent(
            InParams.Get_IsmRenderer().Get(), static_cast<uint8>(StencilValue)
#if WITH_EDITOR
            , UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionOwnerWeak(InHandle)
#endif
            );

        if (ck::Is_NOT_Valid(ShadowIsm))
        { return; }

        constexpr auto TransformAsWorldSpace = true;
        const auto& InstanceTransform = Get_TransformWithLocalOffset(InParams, InTransform.Get_Transform());
        const auto& ShadowInstanceId = ShadowIsm->AddInstanceById(InstanceTransform, TransformAsWorldSpace);

        if (const auto& CustomData = InCurrent.Get_CustomInstanceDataValues();
            NOT CustomData.IsEmpty() && ShadowIsm->NumCustomDataFloats == CustomData.Num())
        { ShadowIsm->SetCustomDataById(ShadowInstanceId, CustomData); }

        InHandle.AddOrGet<FFragment_IsmProxy_CelPatternApplied>() =
            FFragment_IsmProxy_CelPatternApplied{InTarget.Get_Pattern(), ShadowIsm.Get(), ShadowInstanceId, StencilValue};

        ck::ismrenderer::VeryVerbose(TEXT("Applied cel pattern (shadow instance) for ISM Proxy [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_CelPattern_TransformSync::
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
        FProcessor_IsmProxy_CelPattern_TransformSync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_CelPatternApplied& InApplied,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform)
        -> void
    {
        using namespace ck_ism_cel_pattern_processor;

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
        FProcessor_IsmProxy_CelPattern_Suspend::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_CelPatternApplied& InApplied)
        -> void
    {
        ck_ism_cel_pattern_processor::DoTeardownAppliedCelPattern(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_CelPatternApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_CelPattern_DropAppliedOnOutline::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_CelPatternApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget)
        -> void
    {
        ck_ism_cel_pattern_processor::DoTeardownAppliedCelPattern(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_CelPatternApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_CelPattern_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_CelPatternApplied& InApplied)
        -> void
    {
        ck_ism_cel_pattern_processor::DoTeardownAppliedCelPattern(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_CelPatternApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_CelPattern_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_CelPatternApplied& InApplied)
        -> void
    {
        ck_ism_cel_pattern_processor::DoTeardownAppliedCelPattern(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_CelPatternApplied>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
