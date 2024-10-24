#include "CkInteraction_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/CkInteraction_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Interaction_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Interaction_ParamsData& InParams)
    -> FCk_Handle_Interaction
{
	auto NewInteractionEntity = [&]
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
	    NewEntity.Add<ck::FFragment_Interaction_Params>(InParams);
	    NewEntity.Add<ck::FFragment_Interaction_Current>();
        return Cast(NewEntity);
    }();

    UCk_Utils_GameplayLabel_UE::Add(NewInteractionEntity, InParams.Get_InteractionChannel());
	UCk_Utils_Handle_UE::Set_DebugName(NewInteractionEntity, FName(ck::Format_UE(TEXT("Interaction: Source [{}] Target [{}]"), InParams.Get_Source(), InParams.Get_Target())));

	NewInteractionEntity.Add<ck::FTag_Interaction_RequiresSetup>();

	UCk_Entity_ConstructionScript_PDA::Request_Construct(NewInteractionEntity, InParams.Get_ConstructionScript(), {});

	RecordOfInteractions_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
	RecordOfInteractions_Utils::Request_Connect(InHandle, NewInteractionEntity);

	return NewInteractionEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Interaction_UE, FCk_Handle_Interaction,
    ck::FFragment_Interaction_Params, ck::FFragment_Interaction_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Interaction_UE::
	Request_EndInteraction(
		FCk_Handle_Interaction& InInteraction,
		const FCk_Request_Interaction_EndInteraction& InRequest)
	-> FCk_Handle_Interaction
{
	InInteraction.AddOrGet<ck::FFragment_Interaction_Requests>()._Requests.Emplace(InRequest);
    return InInteraction;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Interaction_UE::
	BindTo_OnInteractionFinished(
		FCk_Handle_Interaction& InHandle,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior,
		const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate)
	-> FCk_Handle_Interaction
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_Interaction_OnInteractionFinished, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
	UCk_Utils_Interaction_UE::
	UnbindFrom_OnInteractionFinished(
		FCk_Handle_Interaction& InHandle,
		const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate)
	-> FCk_Handle_Interaction
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_Interaction_OnInteractionFinished, InHandle, InDelegate);
    return InHandle;
}

auto
	UCk_Utils_Interaction_UE::
	TryGet(
		const FCk_Handle& InInteractionOwner,
		const FCk_Handle& InSource,
		const FCk_Handle& InTarget,
		FGameplayTag InInteractionChannel)
	-> FCk_Handle_Interaction
{
	return RecordOfInteractions_Utils::Get_ValidEntry_If(InInteractionOwner,
	[&](const FCk_Handle_Interaction& InHandle) -> bool
	{
		return Get_InteractionSource(InHandle) == InSource &&
			Get_InteractionTarget(InHandle) == InTarget &&
			Get_InteractionChannel(InHandle).MatchesTagExact(InInteractionChannel);
	});
}

auto
	UCk_Utils_Interaction_UE::
	ForEach(
		FCk_Handle& InInteractionOwner,
		const TFunction<void(FCk_Handle_Interaction)>& InFunc)
	-> void
{
	RecordOfInteractions_Utils::ForEach_ValidEntry(InInteractionOwner, InFunc);
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionSource(
		const FCk_Handle_Interaction& InHandle)
	-> FCk_Handle
{
	return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_Source();
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionInstigator(
		const FCk_Handle_Interaction& InHandle)
	-> FCk_Handle
{
	return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_Instigator();
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionTarget(
		const FCk_Handle_Interaction& InHandle)
	-> FCk_Handle
{
	return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_Target();
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionChannel(
		const FCk_Handle_Interaction& InHandle)
	-> const FGameplayTag&
{
	return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_InteractionChannel();
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionCompletionPolicy(
		FCk_Handle_Interaction& InHandle)
	-> ECk_InteractionCompletionPolicy
{
	return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_CompletionPolicy();
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionInteractionDuration(
		FCk_Handle_Interaction& InHandle)
	-> FCk_Time
{
	const auto& DurationAttribute = Get_InteractionInteractionDurationAttribute(InHandle);
	if (ck::IsValid(DurationAttribute))
	{
		return FCk_Time(UCk_Utils_FloatAttribute_UE::Get_FinalValue(DurationAttribute));
	}
	// No attribute if Setup hasn't run yet
	else
	{
		return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_InteractionDuration();
	}
}

auto
	UCk_Utils_Interaction_UE::
	Get_InteractionInteractionDurationAttribute(
		FCk_Handle_Interaction& InHandle)
	-> FCk_Handle_FloatAttribute
{
	return UCk_Utils_FloatAttribute_UE::TryGet(InHandle, TAG_InteractionDuration_FloatAttribute_Name);
}

// --------------------------------------------------------------------------------------------------------------------