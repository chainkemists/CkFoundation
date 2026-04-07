#include "CkInteraction_EntityScript.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"
#include "CkInteraction/CkInteraction_Log.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_Interaction_EntityScript::
    UCk_Interaction_EntityScript(
        const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    _Replication = ECk_Replication::DoesNotReplicate;
}

auto
    UCk_Interaction_EntityScript::
    ShowReplicationInEditor() const
    -> bool
{
    return false;
}

auto
    UCk_Interaction_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto* Params = InSpawnParams.GetPtr<FCk_Fragment_Interaction_ParamsData>();

    CK_ENSURE_IF_NOT(ck::IsValid(Params, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to Construct Interaction EntityScript on entity [{}]: SpawnParams does not contain FCk_Fragment_Interaction_ParamsData"),
        InHandle)
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    InHandle.Add<ck::FFragment_Interaction_Params>(*Params);
    InHandle.Add<ck::FFragment_Interaction_Current>();
    auto InteractionEntity = UCk_Utils_Interaction_UE::Cast(InHandle);

    if (Params->Get_CompletionPolicy() == ECk_Interaction_CompletionPolicy::Timed)
    {
        const auto& TimeRefillAttributeParams = FCk_Fragment_FloatAttributeRefill_ParamsData{
            TAG_InteractionTimeRefill_FloatAttribute_Name,
            1.0f}
            .Set_StartingState(ECk_Attribute_RefillState::Running)
            .Set_RefillBehavior(ECk_Attribute_Refill_Policy::Variable);

        const auto TimeAttributeParams = FCk_Fragment_FloatAttribute_ParamsData{
            TAG_InteractionTime_FloatAttribute_Name,
            0.0f}
            .Set_MinMax(ECk_MinMax::MinMax)
            .Set_MinValue(0.0f)
            .Set_MaxValue(Params->Get_InteractionDuration().Get_Seconds())
            .Set_EnableRefill(true)
            .Set_RefillParams(TimeRefillAttributeParams);

        UCk_Utils_FloatAttribute_UE::Add(InteractionEntity, TimeAttributeParams, ECk_Replication::DoesNotReplicate);
    }

    auto InteractTarget = Params->Get_Target();
    UCk_Utils_GameplayLabel_UE::Add(InteractionEntity, Params->Get_InteractionChannel());
    UCk_Utils_Handle_UE::Set_DebugName(InteractionEntity, *ck::Format_UE(TEXT("Interaction: Source [{}] Target [{}]"), Params->Get_Source(), InteractTarget));

    UCk_Utils_Interaction_UE::RecordOfInteractions_Utils::Request_Connect(InteractTarget, InteractionEntity);

    return Super::Construct(InteractionEntity, InSpawnParams);
}

auto
    UCk_Interaction_EntityScript::
    BeginPlay()
    -> void
{
    auto InteractionHandle = UCk_Utils_Interaction_UE::CastChecked(_AssociatedEntity);
    const auto CompletionPolicy = UCk_Utils_Interaction_UE::Get_InteractionCompletionPolicy(InteractionHandle);

    switch (CompletionPolicy)
    {
        case ECk_Interaction_CompletionPolicy::Instant:
        {
            UCk_Utils_Interaction_UE::Request_EndInteraction(
                InteractionHandle,
                FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Succeeded});
            break;
        }
        case ECk_Interaction_CompletionPolicy::Timed:
        {
            if (UCk_Utils_Interaction_UE::Get_InteractionDuration(InteractionHandle).Get_Seconds() <= 0.0f)
            {
                UCk_Utils_Interaction_UE::Request_EndInteraction(
                    InteractionHandle,
                    FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Succeeded});
                break;
            }

            auto TimeAttribute = UCk_Utils_Interaction_UE::Get_InteractionTimeAttribute(InteractionHandle);

            auto OnMaxClampedDelegate = FCk_Delegate_FloatAttribute_OnClamped{};
            OnMaxClampedDelegate.BindDynamic(this, &UCk_Interaction_EntityScript::OnInteractionTimeMaxClamped);
            UCk_Utils_FloatAttribute_UE::BindTo_OnMaxClamped(TimeAttribute, OnMaxClampedDelegate);
            break;
        }
        case ECk_Interaction_CompletionPolicy::ManuallyCompleted:
        {
            break;
        }
        default:
        {
            CK_INVALID_ENUM(CompletionPolicy);
            break;
        }
    }

    Super::BeginPlay();
}

auto
    UCk_Interaction_EntityScript::
    OnInteractionTimeMaxClamped(
        FCk_Handle InAttributeOwnerEntity,
        FCk_Payload_FloatAttribute_OnClamped InPayload)
    -> void
{
    auto InteractionHandle = UCk_Utils_Interaction_UE::CastChecked(_AssociatedEntity);
    UCk_Utils_Interaction_UE::Request_EndInteraction(
        InteractionHandle,
        FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Succeeded});
}

// --------------------------------------------------------------------------------------------------------------------
