#include "CkIsmProxy_StylizeMaskProcessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIsmRenderer/CkIsmRenderer_Log.h"
#include "CkIsmRenderer/CkIsmSubsystem.h"

#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_Sync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_TransformSync);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_Suspend);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_DropAppliedOnOutline);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_DropAppliedOnCelPattern);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_Remove);
CK_REGISTER_PROCESSOR(ck::FProcessor_IsmProxy_StylizeMask_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ism_stylize_mask_processor
{
    using ck_ism_proxy::Get_TransformWithLocalOffset;

    // No stencil to hand back: the mask is a direct project-configured value rather than the outline's
    // refcounted allocation, so pulling the instance out of the shadow ISM is the whole teardown.
    auto
        DoTeardownAppliedStylizeMask(
            const ck::FFragment_IsmProxy_StylizeMaskApplied& InApplied)
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
        FProcessor_IsmProxy_StylizeMask_Sync::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IsmProxy_StylizeMask_Sync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Usf_StylizeMaskTarget& InTarget,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        using namespace ck_ism_stylize_mask_processor;

        if (ck::Is_NOT_Valid(_World.Get()))
        { return; }

        // Project config rather than a per-world subsystem: the mask claims ONE byte value for the whole
        // project, and no world-local state narrows it. Read every frame so an editor-side change lands
        // without a restart, the way the cel contract's stencil base does.
        const auto StencilValue = UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue();

        // 0 is the engine's "nothing written here" — a shadow carrying it would mark its instances with the
        // value that means untagged and read as a silent success. The project setting clamps at 1, so this
        // only fires on an .ini hand-edited past the clamp.
        if (StencilValue <= 0)
        { return; }

        if (InHandle.Has<FFragment_IsmProxy_StylizeMaskApplied>())
        {
            const auto& Applied = InHandle.Get<FFragment_IsmProxy_StylizeMaskApplied>();

            const auto& StillValid = Applied.Get_StencilValue() == StencilValue &&
                ck::IsValid(Applied.Get_ShadowIsm().Get()) &&
                Applied.Get_ShadowIsm()->IsValidId(Applied.Get_ShadowInstanceId());

            if (StillValid)
            { return; }

            DoTeardownAppliedStylizeMask(Applied);
            InHandle.Remove<FFragment_IsmProxy_StylizeMaskApplied>();
        }

        auto* IsmSubsystem = _World->GetSubsystem<UCk_IsmRenderer_Subsystem_UE>();

        if (ck::Is_NOT_Valid(IsmSubsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto ShadowIsm = IsmSubsystem->FindOrCreate_StylizeMaskIsmComponent(
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

        InHandle.AddOrGet<FFragment_IsmProxy_StylizeMaskApplied>() =
            FFragment_IsmProxy_StylizeMaskApplied{ShadowIsm.Get(), ShadowInstanceId, StencilValue};

        ck::ismrenderer::VeryVerbose(TEXT("Applied stylize mask (shadow instance) for ISM Proxy [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_StylizeMask_TransformSync::
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
        FProcessor_IsmProxy_StylizeMask_TransformSync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_Transform& InTransform)
        -> void
    {
        using namespace ck_ism_stylize_mask_processor;

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
        FProcessor_IsmProxy_StylizeMask_Suspend::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied)
        -> void
    {
        ck_ism_stylize_mask_processor::DoTeardownAppliedStylizeMask(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_StylizeMask_DropAppliedOnOutline::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_OutlineTarget& InOutlineTarget)
        -> void
    {
        ck_ism_stylize_mask_processor::DoTeardownAppliedStylizeMask(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_StylizeMask_DropAppliedOnCelPattern::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied,
            const FFragment_Usf_CelPatternTarget& InCelPatternTarget)
        -> void
    {
        ck_ism_stylize_mask_processor::DoTeardownAppliedStylizeMask(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_StylizeMask_Remove::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied)
        -> void
    {
        ck_ism_stylize_mask_processor::DoTeardownAppliedStylizeMask(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_StylizeMaskApplied>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_StylizeMask_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_StylizeMaskApplied& InApplied)
        -> void
    {
        ck_ism_stylize_mask_processor::DoTeardownAppliedStylizeMask(InApplied);
        InHandle.Try_Remove<FFragment_IsmProxy_StylizeMaskApplied>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
