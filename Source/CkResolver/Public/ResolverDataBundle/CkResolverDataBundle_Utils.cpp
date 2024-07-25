#include "CkResolverDataBundle_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverDataBundle_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_ResolverDataBundle_ParamsData& InParams)
    -> FCk_Handle_ResolverDataBundle
{
    CK_ENSURE_IF_NOT(NOT InParams.Get_Phases().IsEmpty(),
        TEXT("Unable to create a ResolverDataBundle with Owner [{}]. There must be at least ONE phase in a ResolverDataBundle"), InOwner)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(NewEntity, *ck::Format_UE(TEXT("ResolverDataBundle with Owner [{}]"), InOwner));

    const auto& Params = NewEntity.Add<ck::FFragment_ResolverDataBundle_Params>(InParams);
    auto& Current = NewEntity.Add<ck::FFragment_ResolverDataBundle_Current>();

    auto DataBundleEntity = Cast(NewEntity);
    DoTryStartNewPhase(DataBundleEntity, Params.Get_Params().Get_Phases().Num(), Current.Get_CurrentPhaseIndex());

    return DataBundleEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ResolverDataBundle, UCk_Utils_ResolverDataBundle_UE, FCk_Handle_ResolverDataBundle,
    ck::FFragment_ResolverDataBundle_Params, ck::FFragment_ResolverDataBundle_Current)

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
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    UnbindFrom_OnPhaseStart(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnPhaseStart& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
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
    return InDataBundle;
}

auto
    UCk_Utils_ResolverDataBundle_UE::
    UnbindFrom_OnPhaseComplete(
        FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnPhaseComplete& InDelegate)
    -> FCk_Handle_ResolverDataBundle
{
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
        int32 InCurrentPhaseIndex) -> void
{
    if (InCurrentPhaseIndex + 1 == InNumTotalPhases)
    { return; }

    InResolverDataBundle.Add<ck::FTag_Resolver_StartNewPhase>();
}

// --------------------------------------------------------------------------------------------------------------------
