#include "CkResolverDataBundle_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

#include "CkResolver/CkResolver_Log.h"

#include "ResolverDataBundle/CkResolverDataBundle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_ResolverPhase_InvalidPhase, TEXT("ResolverPhase.InvalidPhase"));

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ResolverDataBundle_StartNewPhase::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ResolverDataBundle_StartNewPhase::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ResolverDataBundle_Params& InParams,
            FFragment_ResolverDataBundle_Current& InCurrent) const
        -> void
    {
        [[maybe_unused]]
        const auto& PhaseNamePrevious = InCurrent.Get_CurrentPhaseIndex() >= 0 ?
            InParams.Get_Params().Get_Phases()[InCurrent.Get_CurrentPhaseIndex()].Get_PhaseName() :
            TAG_ResolverPhase_InvalidPhase;

        CK_ENSURE_IF_NOT(InCurrent._CurrentPhaseIndex < InParams.Get_Params().Get_Phases().Num() - 1,
            TEXT("Unable to Start a New Phase for the ResolverSource [{}] with ResolverCause [{}] since we are already on the final phase [{}]"),
            InHandle, InParams.Get_Params().Get_ResolverCause(), PhaseNamePrevious)
        { return; }

        InCurrent._CurrentPhaseIndex++;

        const auto& PhaseName = InParams.Get_Params().Get_Phases()[InCurrent.Get_CurrentPhaseIndex()].Get_PhaseName();
        UUtils_Signal_ResolverDataBundle_PhaseStart::Broadcast(InHandle, ck::MakePayload(InHandle, PhaseName));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ResolverDataBundle_HandleRequests::
        DoTick(
            TimeType InDeltaT) -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ResolverDataBundle_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            FFragment_ResolverDataBundle_Requests& InRequestsComp) const
        -> void
    {
        ck::algo::ForEachRequest(InRequestsComp._MutateMetadataRequests,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
        });

        ck::algo::ForEachRequest(InRequestsComp._MutateModifierRequests,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
        });
    }

    auto
        FProcessor_ResolverDataBundle_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            const FRequest_ResolverDataBundle_ModifierOperation& InRequest)
        -> void
    {
        const auto& ModifierOperation = InRequest.Get_ModifierOperation();

        if (const auto& Requirements = ModifierOperation.Get_BundleTagRequirements();
            Requirements.IsEmpty())
        {
            UCk_Utils_ResolverDataBundle_UE::DoAddPendingOperation(InHandle, ModifierOperation.Get_Operation());
            return;
        }

        UCk_Utils_ResolverDataBundle_UE::DoAddPendingOperation(InHandle, ModifierOperation);
    }

    auto
        FProcessor_ResolverDataBundle_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            const FRequest_ResolverDataBundle_MetadataOperation& InRequest)
        -> void
    {
        const auto& MetadataOperation = InRequest.Get_MetadataOperation();

        if (const auto& Requirements = MetadataOperation.Get_BundleTagRequirements();
            Requirements.IsEmpty())
        {
            UCk_Utils_ResolverDataBundle_UE::DoAddPendingOperation(InHandle, MetadataOperation.Get_Operation());
            return;
        }

        UCk_Utils_ResolverDataBundle_UE::DoAddPendingOperation(InHandle, MetadataOperation);
    }

    auto
        FProcessor_ResolverDataBundle_ResolveOperations::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ResolverDataBundle_ResolveOperations::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InCurrent,
            const FFragment_ResolverDataBundle_PendingOperations& InPendingOperations)
        -> void
    {
        const auto& ModifyResolverDataComponentAttribute = [&](const FCk_ResolverDataBundle_ModifierOperation& InModifierOperation)
        {
            const auto AttributeModifierParams = FCk_Fragment_FloatAttributeModifier_ParamsData{
                    InModifierOperation.Get_ModifierDelta(),
                    InModifierOperation.Get_ModifierOperation(),
                    ECk_ModifierOperation_RevocablePolicy::Revocable,
                    ECk_MinMaxCurrent::Current};

            const auto& ResolverDataComponentAttributeName =
                UCk_Utils_ResolverDataBundle_UE::DoGet_ResolverDataComponentAttributeName(InModifierOperation.Get_ResolverComponent());

            auto ResolverDataComponentAttribute = UCk_Utils_FloatAttribute_UE::TryGet(InHandle, ResolverDataComponentAttributeName);

            UCk_Utils_FloatAttributeModifier_UE::Add(ResolverDataComponentAttribute, FGameplayTag::EmptyTag,AttributeModifierParams);
        };

        const auto& ResolvePendingMetadataOperations = [&]() -> void
        {
            for (const auto& MetadataOperation : InPendingOperations.Get_PendingMetadataOperations())
            {
                InCurrent._MetadataTags.AppendTags(MetadataOperation.Get_TagsToAdd());
                InCurrent._MetadataTags.RemoveTags(MetadataOperation.Get_TagsToRemove());
            }

            for (const auto& MetadataOperation : InPendingOperations.Get_PendingMetadataOperations_Conditionals())
            {
                if (NOT MetadataOperation.Get_BundleTagRequirements().RequirementsMet(InCurrent.Get_MetadataTags()))
                { continue; }

                InCurrent._MetadataTags.AppendTags(MetadataOperation.Get_Operation().Get_TagsToAdd());
                InCurrent._MetadataTags.RemoveTags(MetadataOperation.Get_Operation().Get_TagsToRemove());
            }

        }; ResolvePendingMetadataOperations();

        const auto& ResolvePendingModifierOperations = [&]() -> void
        {
            for (const auto& ModifierOperation : InPendingOperations.Get_PendingModifiersOperations())
            {
                ModifyResolverDataComponentAttribute(ModifierOperation);
            }

            for (const auto& ModifierOperation : InPendingOperations.Get_PendingModifiersOperations_Conditionals())
            {
                if (NOT ModifierOperation.Get_BundleTagRequirements().RequirementsMet(InCurrent.Get_MetadataTags()))
                { continue; }

                ModifyResolverDataComponentAttribute(ModifierOperation.Get_Operation());
            }

        }; ResolvePendingModifierOperations();

        UCk_Utils_ResolverDataBundle_UE::DoMarkBundle_AsOperationsResolved(InHandle);

        ck::resolver::Verbose(TEXT("Resolved all Pending Operations of Damage Bundle [{}]"), InHandle);
    }

    auto
        FProcessor_ResolverDataBundle_Calculate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ResolverDataBundle_Params& InParams,
            FFragment_ResolverDataBundle_Current& InCurrent)
        -> void
    {
        const auto& ResolverDataBaseValue_AttributeName =
            UCk_Utils_ResolverDataBundle_UE::DoGet_ResolverDataComponentAttributeName(ECk_ResolverDataBundle_ModifierComponent::BaseValue);
        const auto& ResolverDataBaseValue_Attribute = UCk_Utils_FloatAttribute_UE::TryGet(InHandle, ResolverDataBaseValue_AttributeName);
        const auto& ResolverDataBaseValue = UCk_Utils_FloatAttribute_UE::Get_FinalValue(ResolverDataBaseValue_Attribute);

        const auto& ResolverDataBonusValue_AttributeName =
            UCk_Utils_ResolverDataBundle_UE::DoGet_ResolverDataComponentAttributeName(ECk_ResolverDataBundle_ModifierComponent::BonusValue);
        const auto& ResolverDataBonusValue_Attribute = UCk_Utils_FloatAttribute_UE::TryGet(InHandle, ResolverDataBonusValue_AttributeName);
        const auto& ResolverDataBonusValue = UCk_Utils_FloatAttribute_UE::Get_FinalValue(ResolverDataBonusValue_Attribute);

        const auto& ResolverDataTotalScalarValue_AttributeName =
            UCk_Utils_ResolverDataBundle_UE::DoGet_ResolverDataComponentAttributeName(ECk_ResolverDataBundle_ModifierComponent::TotalScalar);
        const auto& ResolverDataTotalScalarValue_Attribute = UCk_Utils_FloatAttribute_UE::TryGet(InHandle, ResolverDataTotalScalarValue_AttributeName);
        const auto& ResolverDataTotalScalarValue = UCk_Utils_FloatAttribute_UE::Get_FinalValue(ResolverDataTotalScalarValue_Attribute);

        const auto& CalculatedFinalValue = (ResolverDataBaseValue + ResolverDataBonusValue) * ResolverDataTotalScalarValue;
        InCurrent._FinalValue = CalculatedFinalValue;

        resolver::Verbose(TEXT("Calculated Final Value [{}] of ResolverData Bundle [{}]"), CalculatedFinalValue, InHandle);

        UCk_Utils_ResolverDataBundle_UE::DoMarkBundle_AsCalculateDone(InHandle);

        const auto& Instigator = InParams.Get_Params().Get_Instigator();
        const auto& Target = InParams.Get_Params().Get_Target();
        const auto& ResolverCause = InParams.Get_Params().Get_ResolverCause();
        const auto& PhaseName = InParams.Get_Params().Get_Phases()[InCurrent.Get_CurrentPhaseIndex()].Get_PhaseName();

        const auto PostCalculatePayload = FPayload_ResolverDataBundle_OnResolved{}
            .Set_Instigator(Instigator)
            .Set_Target(Target)
            .Set_ResolverCause(ResolverCause)
            .Set_FinalValue(CalculatedFinalValue)
            .Set_Metadata(InCurrent.Get_MetadataTags());

        UUtils_Signal_ResolverDataBundle_PhaseComplete::Broadcast(
            InHandle, ck::MakePayload
            (
                InHandle,
                PhaseName,
                FPayload_ResolverDataBundle_OnResolved{}
                    .Set_Instigator(Instigator)
                    .Set_Target(Target)
                    .Set_ResolverCause(ResolverCause)
                    .Set_FinalValue(CalculatedFinalValue)
                    .Set_Metadata(InCurrent.Get_MetadataTags())
            ));

        UCk_Utils_ResolverDataBundle_UE::DoTryStartNewPhase(InHandle, InParams.Get_Params().Get_Phases().Num(), InCurrent.Get_CurrentPhaseIndex());
    }
}

// --------------------------------------------------------------------------------------------------------------------
