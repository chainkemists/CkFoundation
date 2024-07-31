#include "CkResolverDataBundle_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverDataBundle_UE::
    Create(
        FCk_Handle& InOwner,
        FGameplayTag InName,
        const FCk_Fragment_ResolverDataBundle_ParamsData& InParams)
    -> FCk_Handle_ResolverDataBundle
{
    CK_ENSURE_IF_NOT(NOT InParams.Get_Phases().IsEmpty(),
        TEXT("Unable to create a ResolverDataBundle with Owner [{}]. There must be at least ONE phase in a ResolverDataBundle"), InOwner)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_GameplayLabel_UE::Add(NewEntity, InName);
    UCk_Utils_Handle_UE::Set_DebugName(NewEntity, *ck::Format_UE(TEXT("ResolverDataBundle with Owner [{}]"), InOwner));

    const auto& Params = NewEntity.Add<ck::FFragment_ResolverDataBundle_Params>(InParams);
    auto& Current = NewEntity.Add<ck::FFragment_ResolverDataBundle_Current>();

    // Add the attributes we need for damage calculation
    // Formula = (BaseDamage + BonusDamage) * TotalScalarDamage
    UCk_Utils_FloatAttribute_UE::Add(
        NewEntity,
        FCk_Fragment_FloatAttribute_ParamsData(
            TAG_Label_ResolverDataBundle_BaseValue,
            0.0f), ECk_Replication::DoesNotReplicate);

    UCk_Utils_FloatAttribute_UE::Add(
        NewEntity,
        FCk_Fragment_FloatAttribute_ParamsData(
            TAG_Label_ResolverDataBundle_BonusValue,
            0.0f), ECk_Replication::DoesNotReplicate);

    UCk_Utils_FloatAttribute_UE::Add(
        NewEntity,
        FCk_Fragment_FloatAttribute_ParamsData(
            TAG_Label_ResolverDataBundle_TotalScalarValue,
            1.0f), ECk_Replication::DoesNotReplicate);

    auto DataBundleEntity = Cast(NewEntity);
    std::ignore = DoTryStartNewPhase(DataBundleEntity, Params.Get_Params().Get_Phases().Num(), Current.Get_CurrentPhaseIndex());

    return DataBundleEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ResolverDataBundle, UCk_Utils_ResolverDataBundle_UE, FCk_Handle_ResolverDataBundle,
    ck::FFragment_ResolverDataBundle_Params, ck::FFragment_ResolverDataBundle_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_Name(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InDataBundle);
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_Instigator(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> FCk_Handle_ResolverSource
{
    return InDataBundle.Get<FCk_Fragment_ResolverDataBundle_ParamsData>().Get_Instigator();
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_Target(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> FCk_Handle_ResolverTarget
{
    return InDataBundle.Get<FCk_Fragment_ResolverDataBundle_ParamsData>().Get_Target();
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_Causer(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> FCk_Handle
{
    return InDataBundle.Get<FCk_Fragment_ResolverDataBundle_ParamsData>().Get_Causer();
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_Phases(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> TArray<FCk_Fragment_ResolverDataBundle_PhaseInfo>
{
    return InDataBundle.Get<FCk_Fragment_ResolverDataBundle_ParamsData>().Get_Phases();
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_CurrentPhase(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> FGameplayTag
{
    const auto& Phases = InDataBundle.Get<ck::FFragment_ResolverDataBundle_Params>().Get_Params().Get_Phases();
    const auto& CurrentPhaseIndex = InDataBundle.Get<ck::FFragment_ResolverDataBundle_Current>().Get_CurrentPhaseIndex();

    return Phases.IsValidIndex(CurrentPhaseIndex) ? Phases[CurrentPhaseIndex].Get_PhaseName() : TAG_ResolverDataBundle_InvalidPhase;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_FinalValue(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> float
{
    return InDataBundle.Get<ck::FFragment_ResolverDataBundle_Current>().Get_FinalValue();
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Get_Metadata(
        const FCk_Handle_ResolverDataBundle& InDataBundle)
    -> FGameplayTagContainer
{
    return InDataBundle.Get<ck::FFragment_ResolverDataBundle_Current>().Get_MetadataTags();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverDataBundle_UE::
    Request_AddOperation_Modifier(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FRequest_ResolverDataBundle_ModifierOperation& InRequest)
    -> FCk_Handle_ResolverDataBundle
{
    InDataBundle.AddOrGet<ck::FFragment_ResolverDataBundle_Requests>()._MutateModifierRequests.Emplace(InRequest);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    Request_AddOperation_Metadata(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FRequest_ResolverDataBundle_MetadataOperation& InRequest)
    -> FCk_Handle_ResolverDataBundle
{
    InDataBundle.AddOrGet<ck::FFragment_ResolverDataBundle_Requests>()._MutateMetadataRequests.Emplace(InRequest);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    BindTo_OnPhaseStart(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverDataBundle_OnPhaseStart& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_ResolverDataBundle_PhaseStart, InDataBundle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    UnbindFrom_OnPhaseStart(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnPhaseStart& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_ResolverDataBundle_PhaseStart, InDataBundle, InDelegate);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    BindTo_OnPhaseComplete(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverDataBundle_OnPhaseComplete& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_ResolverDataBundle_PhaseComplete, InDataBundle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    UnbindFrom_OnPhaseComplete(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnPhaseComplete& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_ResolverDataBundle_PhaseComplete, InDataBundle, InDelegate);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    BindTo_OnAllPhasesComplete(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_ResolverDataBundle_AllPhasesComplete, InDataBundle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    UnbindFrom_OnAllPhasesComplete(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_ResolverDataBundle_AllPhasesComplete, InDataBundle, InDelegate);
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoGet_ResolverDataComponentAttributeName(
        ECk_ResolverDataBundle_ModifierComponent InResolverDataComponent)
    -> FGameplayTag
{
    switch (InResolverDataComponent)
    {
        case ECk_ResolverDataBundle_ModifierComponent::BaseValue:
        {
            return TAG_Label_ResolverDataBundle_BaseValue;
        }
        case ECk_ResolverDataBundle_ModifierComponent::BonusValue:
        {
            return TAG_Label_ResolverDataBundle_BonusValue;
        }
        case ECk_ResolverDataBundle_ModifierComponent::TotalScalar:
        {
            return TAG_Label_ResolverDataBundle_TotalScalarValue;
        }
        default:
        {
            CK_INVALID_ENUM(InResolverDataComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_ModifierOperation& InOperation)
    -> FCk_Handle_ResolverDataBundle
{
    InResolverDataBundle.AddOrGet<ck::FFragment_ResolverDataBundle_PendingOperations>()._PendingModifiersOperations.Add(InOperation);
    return InResolverDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_ModifierOperation_Conditional& InOperation)
    -> FCk_Handle_ResolverDataBundle
{
    InResolverDataBundle.AddOrGet<ck::FFragment_ResolverDataBundle_PendingOperations>()._PendingModifiersOperations_Conditionals.Add(InOperation);
    return InResolverDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_MetadataOperation& InOperation)
    -> FCk_Handle_ResolverDataBundle
{
    InResolverDataBundle.AddOrGet<ck::FFragment_ResolverDataBundle_PendingOperations>()._PendingMetadataOperations.Add(InOperation);
    return InResolverDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_MetadataOperation_Conditional& InOperation)
    -> FCk_Handle_ResolverDataBundle
{
    InResolverDataBundle.AddOrGet<ck::FFragment_ResolverDataBundle_PendingOperations>()._PendingMetadataOperations_Conditionals.Add(InOperation);
    return InResolverDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoMarkBundle_AsOperationsResolved(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle)
    -> FCk_Handle_ResolverDataBundle
{
    InResolverDataBundle.Add<ck::FTag_ResolverDataBundle_OperationsResolved>();
    return InResolverDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoMarkBundle_AsCalculateDone(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle)
    -> FCk_Handle_ResolverDataBundle
{
    InResolverDataBundle.Add<ck::FTag_ResolverDataBundle_CalculateDone>();
    return InResolverDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    DoTryStartNewPhase(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        int32 InNumTotalPhases,
        int32 InCurrentPhaseIndex) -> bool
{
    if (InCurrentPhaseIndex + 1 == InNumTotalPhases)
    { return false; }

    InResolverDataBundle.Add<ck::FTag_ResolverDataBundle_StartNewPhase>();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
