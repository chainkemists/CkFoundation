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
                FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Succeeded},
                {});
            break;
        }
        case ECk_Interaction_CompletionPolicy::Timed:
        {
            if (UCk_Utils_Interaction_UE::Get_InteractionDuration(InteractionHandle).Get_Seconds() <= 0.0f)
            {
                UCk_Utils_Interaction_UE::Request_EndInteraction(
                    InteractionHandle,
                    FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Succeeded},
                    {});
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
        FCk_Request_Interaction_EndInteraction{ECk_SucceededFailed::Succeeded},
        {});
}

// --------------------------------------------------------------------------------------------------------------------
